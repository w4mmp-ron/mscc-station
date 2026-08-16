import socket
import tkinter as tk
import customtkinter as ctk
import threading
import time
from collections import deque
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from datetime import datetime
import os
import json
from tkinter import simpledialog
import requests

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")


class SWRMeterApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("SWR Meter • Remote")
        self.geometry("860x860")
        self.resizable(True, True)
        self.minsize(810, 780)

        # UDP
        self.UDP_IP = "0.0.0.0"
        self.UDP_PORT = 6999
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.UDP_IP, self.UDP_PORT))
        self.sock.settimeout(0.05)

        # Data variables
        self.fwd_power = 0.0
        self.ref_power = 0.0
        self.swr = 1.0
        self.voltage = 0.0
        self.fault = 0
        self.last_update = time.time()

        # Peaks & Session tracking
        self.local_peak_fwd = 0.0
        self.local_peak_swr = 1.0
        self.best_swr = 1.0
        self.best_swr_time = 0
        self.time_above_2 = 0.0

        # Auto-reset configuration - persistent
        self.low_power_threshold = 1.0
        self.reset_delay = 5.0
        self.low_power_since = None
        self.auto_reset_pending = False

        # Graph history
        self.history_time = deque(maxlen=2400)
        self.history_fwd = deque(maxlen=2400)
        self.history_swr = deque(maxlen=2400)
        self.start_time = time.time()
        self.swr_history = deque(maxlen=60)

        self.running = True
        self.graph_paused = False
        self.show_full_history = False
        self.last_graph_update = 0

        # IP handling
        self.meter_ip = None
        self.ip_source = "None"

        self.load_config()

        self.create_widgets()
        threading.Thread(target=self.udp_listener, daemon=True).start()
        self.after(50, self.update_gui)

    def load_config(self):
        config_file = os.path.join(os.path.expanduser("~"), ".swr_meter_config.json")
        try:
            if os.path.exists(config_file):
                with open(config_file, "r") as f:
                    data = json.load(f)
                    saved_ip = data.get("meter_ip")
                    if saved_ip:
                        self.meter_ip = saved_ip
                        self.ip_source = "Manual"
                    self.low_power_threshold = data.get("low_power_threshold", 1.0)
                    self.reset_delay = data.get("reset_delay", 5.0)
        except Exception as e:
            print(f"Config load warning: {e}")

    def save_config(self):
        config_file = os.path.join(os.path.expanduser("~"), ".swr_meter_config.json")
        try:
            data = {
                "meter_ip": self.meter_ip,
                "low_power_threshold": self.low_power_threshold,
                "reset_delay": self.reset_delay
            }
            with open(config_file, "w") as f:
                json.dump(data, f, indent=2)
        except Exception as e:
            print(f"Config save failed: {e}")

    def create_metric_card(self, parent, title, value_text="0.0", has_reset=False, cmd=None):
        card = ctk.CTkFrame(parent, fg_color="#252526", corner_radius=8, height=68)
        card.grid_propagate(False)

        ctk.CTkLabel(card, text=title, font=("Arial", 10, "bold"), 
                    text_color="#aaaaaa").pack(pady=(5, 1))
        
        lbl_value = ctk.CTkLabel(card, text=value_text, 
                                font=("Arial", 15, "bold"))
        lbl_value.pack(pady=(0, 3))
        
        if has_reset and cmd:
            btn = ctk.CTkButton(card, text="Reset", width=58, height=20,
                               font=("Arial", 9), fg_color="#444444",
                               hover_color="#555555", command=cmd)
            btn.pack(pady=(0, 5))
        
        return card, lbl_value

    def reset_peak_fwd(self):
        self.local_peak_fwd = 0.0
        self.low_power_since = None
        self.auto_reset_pending = False

    def reset_peak_swr(self):
        self.local_peak_swr = 1.0
        self.low_power_since = None
        self.auto_reset_pending = False

    def reset_graph(self):
        self.history_time.clear()
        self.history_fwd.clear()
        self.history_swr.clear()
        self.start_time = time.time()
        self.last_graph_update = 0
        print("Graph history has been reset")
        self.update_graph()

    def create_widgets(self):
        self.grid_rowconfigure((0,1,6,7), weight=0)
        self.grid_rowconfigure(2, weight=1)
        self.grid_columnconfigure(0, weight=1)

        # Header
        header = ctk.CTkFrame(self, fg_color="#0a0a0a", height=52, corner_radius=0)
        header.grid(row=0, column=0, sticky="ew")
        ctk.CTkLabel(header, text="SWR METER — REMOTE", 
                     font=("Montserrat", 24, "bold"), text_color="#00ddff").pack(pady=10)

        self.main = ctk.CTkFrame(self, fg_color="#1a1a1a")
        self.main.grid(row=2, column=0, sticky="nsew", padx=16, pady=10)
        self.main.grid_rowconfigure(4, weight=1)
        self.main.grid_columnconfigure(0, weight=1)

        # Alert Banner
        self.alert_banner = ctk.CTkLabel(self.main, text="", font=("Arial", 18, "bold"),
                                         text_color="white", fg_color="#9f0000", height=36)
        self.alert_banner.grid(row=0, column=0, sticky="ew", padx=16, pady=(8,4))
        self.alert_banner.grid_remove()

        # Big SWR + Sparkline
        swr_section = ctk.CTkFrame(self.main, fg_color="#1f1f1f", corner_radius=16)
        swr_section.grid(row=1, column=0, sticky="nsew", padx=20, pady=12)
        swr_section.grid_rowconfigure(1, weight=1)
        swr_section.grid_columnconfigure(0, weight=1)

        self.lbl_swr = ctk.CTkLabel(swr_section, text="1.00",
                                    font=("Montserrat", 102, "bold"), text_color="#00ff99")
        self.lbl_swr.grid(row=0, column=0, pady=(14,6))

        self.spark_canvas = tk.Canvas(swr_section, height=34, bg="#1f1f1f", highlightthickness=0)
        self.spark_canvas.grid(row=1, column=0, sticky="ew", padx=28, pady=(0,14))

        self.lbl_trend = ctk.CTkLabel(swr_section, text="→", font=("Arial", 44, "bold"), width=70)
        self.lbl_trend.place(relx=0.87, rely=0.23)

        # Power Bars
        power_section = ctk.CTkFrame(self.main, fg_color="transparent")
        power_section.grid(row=2, column=0, sticky="ew", padx=24, pady=8)
        power_section.grid_columnconfigure((0,1), weight=1)

        fwd_f = ctk.CTkFrame(power_section, fg_color="#252526", corner_radius=12)
        fwd_f.grid(row=0, column=0, sticky="nsew", padx=(0,8))
        ctk.CTkLabel(fwd_f, text="FORWARD POWER", font=("Arial", 15, "bold")).pack(pady=(14,6))
        self.bar_fwd = ctk.CTkProgressBar(fwd_f, height=42, corner_radius=12, progress_color="#00ddff")
        self.bar_fwd.pack(fill="x", padx=18, pady=(8,10))
        self.lbl_fwd = ctk.CTkLabel(fwd_f, text="0.0 W", font=("Arial", 21, "bold"))
        self.lbl_fwd.pack(pady=(0,12))

        ref_f = ctk.CTkFrame(power_section, fg_color="#252526", corner_radius=12)
        ref_f.grid(row=0, column=1, sticky="nsew", padx=(8,0))
        ctk.CTkLabel(ref_f, text="REFLECTED POWER", font=("Arial", 15, "bold")).pack(pady=(14,6))
        self.bar_ref = ctk.CTkProgressBar(ref_f, height=42, corner_radius=12, progress_color="#ff8800")
        self.bar_ref.pack(fill="x", padx=18, pady=(8,10))
        self.lbl_ref = ctk.CTkLabel(ref_f, text="0.00 W", font=("Arial", 21, "bold"))
        self.lbl_ref.pack(pady=(0,12))

        # Metric Cards - 3 columns (temperature card removed)
        metrics = ctk.CTkFrame(self.main, fg_color="transparent")
        metrics.grid(row=3, column=0, sticky="ew", padx=20, pady=(6, 8))
        metrics.grid_columnconfigure((0, 1, 2), weight=1)

        card_fwd, self.lbl_peak_fwd = self.create_metric_card(
            metrics, "PEAK FWD", "0.0 W", has_reset=True, cmd=self.reset_peak_fwd)
        card_volt, self.lbl_volt = self.create_metric_card(
            metrics, "FWD VOLTAGE", "0.0 V")
        card_swr, self.lbl_peak_swr = self.create_metric_card(
            metrics, "PEAK SWR", "1.00", has_reset=True, cmd=self.reset_peak_swr)

        card_fwd.grid(row=0, column=0, padx=4, sticky="nsew")
        card_volt.grid(row=0, column=1, padx=4, sticky="nsew")
        card_swr.grid(row=0, column=2, padx=4, sticky="nsew")

        # Graph
        graph_frame = ctk.CTkFrame(self.main, fg_color="#1a1a1a")
        graph_frame.grid(row=4, column=0, sticky="nsew", padx=16, pady=(8, 12))

        self.fig = Figure(figsize=(9, 5.8), dpi=100, facecolor="#1a1a1a")
        self.ax1 = self.fig.add_subplot(111)
        self.ax2 = self.ax1.twinx()

        self.ax1.set_facecolor("#1f1f1f")
        for ax in (self.ax1, self.ax2):
            ax.tick_params(colors="#aaaaaa")
            ax.spines['bottom'].set_color("#555555")

        self.line_fwd, = self.ax1.plot([], [], color="#00ddff", linewidth=2)
        self.line_swr, = self.ax2.plot([], [], color="#ffaa00", linewidth=2)
        self.best_marker, = self.ax2.plot([], [], 'ro', markersize=8)

        self.ax1.set_ylabel("Power (W)", color="#00ddff")
        self.ax2.set_ylabel("SWR", color="#ffaa00")

        self.canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True, padx=10, pady=8)

        # Buttons
        btn_frame = ctk.CTkFrame(self.main, fg_color="transparent")
        btn_frame.grid(row=5, column=0, sticky="ew", pady=8)

        self.btn_view = ctk.CTkButton(btn_frame, text="Live (Last 60s)", width=155,
                                      fg_color="#0066cc", command=self.toggle_view_mode)
        self.btn_view.pack(side="left", padx=6)

        self.btn_pause = ctk.CTkButton(btn_frame, text="Pause Graph", width=115, command=self.toggle_pause)
        self.btn_pause.pack(side="left", padx=6)

        self.btn_log = ctk.CTkButton(btn_frame, text="Log Session", width=115,
                                     fg_color="#00aa00", command=self.log_session)
        self.btn_log.pack(side="left", padx=6)

        self.btn_settings = ctk.CTkButton(btn_frame, text="IP Auto", width=85, height=32,
                                          fg_color="#555555", hover_color="#666666",
                                          font=("Arial", 14), command=self.open_settings)
        self.btn_settings.pack(side="left", padx=6)

        self.btn_peaks = ctk.CTkButton(btn_frame, text="Peaks", width=80, height=32,
                                       fg_color="#555555", hover_color="#666666",
                                       font=("Arial", 14), command=self.open_peak_settings)
        self.btn_peaks.pack(side="left", padx=6)

        self.btn_reset_graph = ctk.CTkButton(btn_frame, text="Clear Graph", width=115, height=32,
                                             fg_color="#555555", hover_color="#666666",
                                             font=("Arial", 14), command=self.reset_graph)
        self.btn_reset_graph.pack(side="left", padx=6)

        self.btn_reset = ctk.CTkButton(btn_frame, text="Reset", width=80, height=32,
                                       fg_color="#8B0000", hover_color="#A00000",
                                       font=("Arial", 14, "bold"), command=self.reset_swr_fault)
        self.btn_reset.pack(side="left", padx=6)

        # Bottom status
        status_frame = ctk.CTkFrame(self.main, fg_color="transparent")
        status_frame.grid(row=6, column=0, sticky="ew", pady=8)
        status_frame.grid_columnconfigure((0,1,2,3), weight=1)

        self.lbl_best_swr = ctk.CTkLabel(status_frame, text="Best SWR: 1.00", 
                                        font=("Arial", 15), text_color="#00ff99")
        self.lbl_best_swr.grid(row=0, column=0, padx=12)

        self.lbl_time_above = ctk.CTkLabel(status_frame, text="Time >2:1: 0s", 
                                          font=("Arial", 15))
        self.lbl_time_above.grid(row=0, column=1, padx=12)

        self.lbl_status = ctk.CTkLabel(status_frame, text="Connected", 
                                      text_color="#00ff99", font=("Arial", 15))
        self.lbl_status.grid(row=0, column=3, padx=12)

    def open_settings(self):
        current = self.meter_ip if self.meter_ip else "Not detected"
        source_text = f" ({self.ip_source})" if self.ip_source else ""

        new_ip = simpledialog.askstring(
            "Meter IP Settings", 
            f"Current: {current}{source_text}\n\nEnter new Meter IP Address:",
            initialvalue=self.meter_ip
        )
        
        if new_ip and new_ip.strip():
            self.meter_ip = new_ip.strip()
            self.ip_source = "Manual"
            self.save_config()
            self.btn_settings.configure(text="IP Man")
            print(f"Manual IP override set: {self.meter_ip}")

    def open_peak_settings(self):
        dialog = ctk.CTkToplevel(self)
        dialog.title("Peak Auto-Reset Settings")
        dialog.geometry("360x280")
        dialog.resizable(False, False)
        dialog.grab_set()

        ctk.CTkLabel(dialog, text="Peak Auto-Reset Settings", 
                    font=("Arial", 16, "bold")).pack(pady=(20, 10))

        ctk.CTkLabel(dialog, text="Low Power Threshold (W):", 
                    font=("Arial", 13)).pack(anchor="w", padx=30, pady=(10, 2))
        thresh_entry = ctk.CTkEntry(dialog, width=140, placeholder_text="1.0")
        thresh_entry.insert(0, f"{self.low_power_threshold:.1f}")
        thresh_entry.pack(pady=(0, 10), padx=30)

        ctk.CTkLabel(dialog, text="Reset Delay after RX (seconds):", 
                    font=("Arial", 13)).pack(anchor="w", padx=30, pady=(10, 2))
        delay_entry = ctk.CTkEntry(dialog, width=140, placeholder_text="5.0")
        delay_entry.insert(0, f"{self.reset_delay:.1f}")
        delay_entry.pack(pady=(0, 20), padx=30)

        btn_frame = ctk.CTkFrame(dialog, fg_color="transparent")
        btn_frame.pack(pady=15)

        def save_and_close():
            try:
                new_thresh = float(thresh_entry.get().strip())
                new_delay = float(delay_entry.get().strip())
                if new_thresh > 0.1 and new_delay >= 1.0:
                    self.low_power_threshold = new_thresh
                    self.reset_delay = new_delay
                    self.save_config()
                    print(f"Peak settings saved → Threshold: {new_thresh:.1f} W, Delay: {new_delay:.1f} s")
                else:
                    print("Warning: threshold must be > 0.1 W and delay >= 1 s")
            except ValueError:
                print("Error: Please enter valid numbers")
            dialog.destroy()

        ctk.CTkButton(btn_frame, text="Save", width=110, height=32, 
                     fg_color="#00aa00", command=save_and_close).pack(side="left", padx=12)
        
        ctk.CTkButton(btn_frame, text="Cancel", width=110, height=32, 
                     fg_color="#555555", command=dialog.destroy).pack(side="left", padx=12)

    def reset_swr_fault(self):
        if self.fwd_power > 1.0:
            original_text = self.alert_banner.cget("text")
            original_color = self.alert_banner.cget("fg_color")
            self.alert_banner.configure(text="STOP TX - Do Not Transmit!", fg_color="#ff8800")
            self.after(2500, lambda: self.alert_banner.configure(
                text=original_text, fg_color=original_color))
            return

        self.fault = 0
        if self.meter_ip:
            try:
                url = f"http://{self.meter_ip}/reset"
                response = requests.get(url, timeout=1.5)
                if response.status_code == 200:
                    print(f"✓ Web reset successful: {url}")
            except Exception as e:
                print(f"✗ Web reset failed: {e}")

        self.lbl_status.configure(text="CLEARED", text_color="#00ff00", font=("Arial", 16, "bold"))
        self.after(1000, lambda: self.lbl_status.configure(
            text="Connected", text_color="#00ff99", font=("Arial", 16)))

    def udp_listener(self):
        while self.running:
            try:
                data, addr = self.sock.recvfrom(2048)
                packet = data.decode('utf-8', errors='ignore').strip()
                if not packet:
                    continue

                # New JSON parsing (strict JSON only - no pipe fallback)
                json_data = json.loads(packet)

                new_ip = addr[0]
                if self.meter_ip != new_ip:
                    self.meter_ip = new_ip
                    self.ip_source = "Auto"
                    self.save_config()
                    print(f"Auto-detected meter IP: {self.meter_ip}")
                    self.after(0, lambda: self.btn_settings.configure(text="IP Auto"))

                # Map required fields only (all new/optional fields ignored as per decisions)
                self.voltage = float(json_data.get("v", 0.0))
                self.fwd_power = float(json_data.get("fwd", 0.0))
                self.ref_power = float(json_data.get("ref", 0.0))
                self.swr = max(1.0, float(json_data.get("swr", 1.0)))
                self.fault = int(json_data.get("fault", 0))
                self.last_update = time.time()
                self.swr_history.append(self.swr)

                if self.fwd_power > 0.05 and self.swr > self.local_peak_swr:
                    self.local_peak_swr = self.swr

            except json.JSONDecodeError:
                print("JSON decode error - invalid packet")
            except Exception:
                pass  # Silent for transient issues

    def toggle_view_mode(self):
        self.show_full_history = not self.show_full_history
        if self.show_full_history:
            self.btn_view.configure(text="History (Last 5 min)", fg_color="#cc6600")
        else:
            self.btn_view.configure(text="Live (Last 60s)", fg_color="#0066cc")

    def toggle_pause(self):
        self.graph_paused = not self.graph_paused
        self.btn_pause.configure(text="Resume Graph" if self.graph_paused else "Pause Graph")

    def log_session(self):
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        folder = "SWR_Logs"
        os.makedirs(folder, exist_ok=True)

        with open(f"{folder}/swr_log_{timestamp}.csv", "w") as f:
            f.write("Time,FWD_Power,SWR\n")
            for t, p, s in zip(self.history_time, self.history_fwd, self.history_swr):
                f.write(f"{t:.1f},{p:.1f},{s:.2f}\n")

        self.fig.savefig(f"{folder}/swr_graph_{timestamp}.png", dpi=200, facecolor="#1a1a1a")

        self.lbl_status.configure(text=f"Logged {timestamp}", text_color="#ffff00")
        self.after(3000, lambda: self.lbl_status.configure(text="Connected", text_color="#00ff99"))

    def draw_sparkline(self):
        self.spark_canvas.delete("all")
        if len(self.swr_history) < 5:
            return

        w = self.spark_canvas.winfo_width()
        h = self.spark_canvas.winfo_height()
        if w < 50 or h < 20:
            return

        vals = list(self.swr_history)
        min_s = min(vals)
        max_s = max(vals)

        if max_s - min_s < 0.001:
            y = h // 2
            self.spark_canvas.create_line(8, y, w-8, y, fill="#00ff99", width=2.5)
            return

        scale = (h - 10) / (max_s - min_s)

        points = []
        for i, v in enumerate(vals):
            x = 8 + i * (w - 16) / (len(vals) - 1)
            y = h - 6 - (v - min_s) * scale
            points.extend([x, y])

        self.spark_canvas.create_line(points, fill="#004422", width=5, smooth=True)
        self.spark_canvas.create_line(points, fill="#00ff99", width=2.5, smooth=True)

    def check_auto_peak_reset(self):
        now = time.time()
        
        if now - self.last_update > 3.0:
            self.low_power_since = None
            self.auto_reset_pending = False
            return

        is_low_power = self.fwd_power <= self.low_power_threshold

        if is_low_power:
            if self.low_power_since is None:
                self.low_power_since = now
                self.auto_reset_pending = True
            elif self.auto_reset_pending and (now - self.low_power_since >= self.reset_delay):
                self.local_peak_fwd = 0.0
                self.local_peak_swr = 1.0
                self.low_power_since = None
                self.auto_reset_pending = False
        else:
            self.low_power_since = None
            self.auto_reset_pending = False

    def update_graph(self):
        if self.graph_paused: return
        now = time.time() - self.start_time
        fwd_plot = self.fwd_power if self.fwd_power >= 2.0 else 0.0
        swr_plot = self.swr

        self.history_time.append(now)
        self.history_fwd.append(fwd_plot)
        self.history_swr.append(swr_plot)

        if swr_plot < self.best_swr and self.fwd_power > 2.0:
            self.best_swr = swr_plot
            self.best_swr_time = now

        if self.show_full_history:
            cutoff = now - 300
            idx = len(self.history_time) - len([t for t in self.history_time if t >= cutoff])
        else:
            cutoff = now - 60
            idx = len(self.history_time) - len([t for t in self.history_time if t >= cutoff])

        t_data = list(self.history_time)[idx:]
        f_data = list(self.history_fwd)[idx:]
        s_data = list(self.history_swr)[idx:]

        self.line_fwd.set_data(t_data, f_data)
        self.line_swr.set_data(t_data, s_data)

        visible_cutoff = now - 300 if self.show_full_history else now - 60
        if self.best_swr < 1.5 and self.best_swr_time >= visible_cutoff:
            self.best_marker.set_data([self.best_swr_time], [self.best_swr])
        else:
            self.best_marker.set_data([], [])

        self.ax1.relim()
        self.ax1.autoscale_view()
        self.ax2.relim()
        self.ax2.autoscale_view()
        self.canvas.draw_idle()

    def update_gui(self):
        now = time.time()
        age = now - self.last_update
        stale = age > 3.0

        self.check_auto_peak_reset()

        if len(self.swr_history) == 0:
            self.after(65, self.update_gui)
            return

        if not stale:
            if self.fwd_power > self.local_peak_fwd + 0.1:
                self.local_peak_fwd = self.fwd_power

            if self.swr > 2.0:
                self.time_above_2 += 0.065

        if stale:
            self.lbl_swr.configure(text="OFFLINE", text_color="#ffdd00", font=("Montserrat", 58, "bold"))
            self.lbl_status.configure(text="OFFLINE", text_color="#ff6666")
        else:
            color = "#ff3333" if (self.fault or self.swr > 4.0) else "#ffaa00" if self.swr > 2.0 else "#ffee66" if self.swr > 1.5 else "#00ff99"
            self.lbl_swr.configure(text=f"{self.swr:.2f}", text_color=color)

            if len(self.swr_history) >= 5:
                delta = self.swr_history[-1] - list(self.swr_history)[-5]
                arrow = "↑" if delta > 0.08 else "↓" if delta < -0.08 else "→"
                self.lbl_trend.configure(text=arrow, text_color="#ff6666" if delta > 0 else "#66ff99" if delta < 0 else "#777777")

            fmax = 20 if self.fwd_power < 15 else 200 if self.fwd_power < 150 else 700
            rmax = 4 if self.fwd_power < 15 else 35 if self.fwd_power < 150 else 100
            self.bar_fwd.set(min(self.fwd_power / fmax, 1.0))
            self.bar_ref.set(min(self.ref_power / rmax, 1.0))

            self.lbl_fwd.configure(text=f"{self.fwd_power:.1f} W")
            self.lbl_ref.configure(text=f"{self.ref_power:.2f} W")

            self.lbl_peak_fwd.configure(text=f"{self.local_peak_fwd:.1f} W", text_color="#ffffff")
            self.lbl_peak_swr.configure(text=f"{self.local_peak_swr:.2f}", text_color="#ffffff")
            self.lbl_volt.configure(text=f"{self.voltage:.1f} V")

            self.draw_sparkline()

            if self.fault or self.swr > 4.0:
                self.alert_banner.configure(text="  ⚠  HIGH SWR / FAULT — CHECK ANTENNA  ⚠  ")
                self.alert_banner.grid()
            else:
                self.alert_banner.grid_remove()

        if time.time() - self.last_graph_update > 0.08:
            self.update_graph()
            self.last_graph_update = time.time()

        self.after(65, self.update_gui)

    def on_closing(self):
        self.running = False
        time.sleep(0.1)
        try: 
            self.sock.close()
        except:
            pass
        self.save_config()
        self.destroy()


if __name__ == "__main__":
    app = SWRMeterApp()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()