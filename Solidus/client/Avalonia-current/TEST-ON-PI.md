# Test on Pi — layout shell (0.4.0)

## Copy

Overwrite `~/mscc-avalonia` with:

`Avalonia Migration\publish\linux-arm64`

## Run

```bash
mscc start
cd ~/mscc-avalonia
chmod +x MSCC.Avalonia
./MSCC.Avalonia
```

## What should look like Windows

- **Left:** SERVER Connect, audio sliders (gray), filters/step, RIT, temps  
- **Center top:** S-meter / VFO A / VFO B / ALC boxes  
- **Band bar** + MHz set / + −  
- **Tabs:** MAIN (spectrum), CW, RX/TX, FAVORITES, cal tabs, SETTINGS  
- **Right:** PTT/TUN/… gray, NB/NR/AN gray, versions, activity log  

## Live (gold / working)

- **Connect** / Disconnect  
- Spectrum + waterfall on **MAIN**  
- Band buttons, mode (USB/LSB/CW/AM/DIG-U), Set / + / − / Step  
- Click spectrum to tune  

## Not live (gray)

- Audio, RIT, PTT/TUN/AMP, NB/NR/AN, zoom, VFO B, most tabs  

## Pass

Window looks like Multus face; spectrum still works; no crash when clicking gray controls.
