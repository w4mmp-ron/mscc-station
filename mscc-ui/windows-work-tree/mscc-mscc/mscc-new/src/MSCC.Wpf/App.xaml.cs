using System;
using System.IO;
using System.Threading;
using System.Windows;
using System.Windows.Threading;
using MSCC.Core.Logging;

namespace MSCC.Wpf;

public partial class App : Application
{
    // Used to enforce single instance. The mutex lives for the lifetime of the first process.
    private static Mutex? _singleInstanceMutex;

    protected override void OnStartup(StartupEventArgs e)
    {
        // Enforce single instance using a named Mutex.
        // "Local\" prefix = per-user session (safer, no elevation issues).
        const string mutexName = @"Local\MSCC-NET9";

        bool createdNew;
        _singleInstanceMutex = new Mutex(true, mutexName, out createdNew);

        if (!createdNew)
        {
            // Another instance is already running.
            MessageBox.Show(
                "MSCC is already running.\n\nOnly one instance of the application is allowed.",
                "MSCC - Already Running",
                MessageBoxButton.OK,
                MessageBoxImage.Information);

            // Clean up the mutex we just created (we don't own it).
            _singleInstanceMutex.Dispose();
            _singleInstanceMutex = null;

            Shutdown();
            return;
        }

        base.OnStartup(e);

        // Hook unhandled exceptions early (for debug phase - logs everything to mscc.log + crash.log)
        this.DispatcherUnhandledException += OnDispatcherUnhandledException;
        AppDomain.CurrentDomain.UnhandledException += OnAppDomainUnhandledException;

        DebugMonitor.MonitorTextBoxText(" App.OnStartup: creating MainWindow explicitly");

        try
        {
            var mainWindow = new MainWindow();
            this.MainWindow = mainWindow;

            // Window position/size/state is restored inside MainWindow ctor from persisted settings
            // (no forced CenterScreen so saved location can take effect).
            // Force visible + foreground still helps with multi-monitor quirks.
            mainWindow.Show();
            mainWindow.Activate();
            mainWindow.Focus();

            DebugMonitor.MonitorTextBoxText(" App.OnStartup: MainWindow.Show() + Activate() called");
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" App.OnStartup EXCEPTION while creating/showing window: {ex}");
            WriteCrashLog(ex);
            // Still try to show a minimal error window so something appears
            var err = new Window
            {
                Title = "MSCC - Startup Error",
                Width = 700,
                Height = 400,
                Content = new System.Windows.Controls.TextBlock
                {
                    Text = "Failed to create main window.\n\nSee crash.log and mscc.log in %LocalAppData%\\MSCC-NET9\\logs\n\n" + ex,
                    Foreground = System.Windows.Media.Brushes.OrangeRed,
                    FontFamily = new System.Windows.Media.FontFamily("Consolas"),
                    FontSize = 11,
                    TextWrapping = System.Windows.TextWrapping.Wrap,
                    Padding = new Thickness(10)
                }
            };
            err.Show();
        }
    }

    private void OnDispatcherUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
    {
        DebugMonitor.MonitorTextBoxText($" UNHANDLED DISPATCHER EXCEPTION: {e.Exception}");
        WriteCrashLog(e.Exception);
        // Do not set e.Handled = true here in debug phase; let it surface if needed.
        // For production we could recover, but we want full info now.
    }

    private void OnAppDomainUnhandledException(object sender, UnhandledExceptionEventArgs e)
    {
        var ex = e.ExceptionObject as Exception;
        DebugMonitor.MonitorTextBoxText($" UNHANDLED APPDOMAIN EXCEPTION: {ex}");
        if (ex != null) WriteCrashLog(ex);
    }

    private static void WriteCrashLog(Exception ex)
    {
        try
        {
            string logDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "MSCC-NET9", "logs");
            Directory.CreateDirectory(logDir);
            string path = Path.Combine(logDir, "crash.log");
            File.AppendAllText(path, $"{DateTime.Now:yyyy-MM-dd HH:mm:ss} APP CRASH / UNHANDLED:\n{ex}\n\n");
        }
        catch { /* last resort */ }
    }

    protected override void OnExit(ExitEventArgs e)
    {
        DebugMonitor.MonitorTextBoxText(" === App.OnExit: last-chance cleanup (in case close path bypassed window Closing; stop already sent) ===");
        try
        {
            if (MainWindow?.DataContext is IDisposable d)
            {
                d.Dispose();
            }
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" App.OnExit cleanup error: {ex.Message}");
        }

        // Release the single-instance mutex so a new instance can start after we exit.
        try
        {
            _singleInstanceMutex?.ReleaseMutex();
            _singleInstanceMutex?.Dispose();
        }
        catch { /* ignore if we don't own it or already released */ }

        base.OnExit(e);
    }
}
