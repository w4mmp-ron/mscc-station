# MSCC WinForms → WPF Migration Strategy

## Guiding Principles

1. **Do not attempt a 1:1 port.** The original application has heavy coupling, global state, and custom GDI+ rendering. A direct port will be painful and result in bad WPF code.

2. **Strong separation of concerns.** MSCC.Core must remain completely free of WPF dependencies.

3. **MVVM is mandatory.** We use CommunityToolkit.Mvvm.

4. **Isolate the hardest part early.** The spectrum / panadapter / waterfall rendering is the most difficult component. It should be developed as an independent custom control.

5. **Incremental delivery.** Get a usable shell running quickly, then replace sections one by one while keeping the old WinForms app as a reference.

## Recommended Phases

### Phase 1: Foundation (Current)
- Clean .NET 9 solution structure
- Basic MVVM setup
- Core domain models (VfoState, RadioState, etc.)
- Shell MainWindow with placeholder regions

### Phase 2: Core Domain & State Management
- Model the full radio state
- Create services for hardware communication (start with mock)

### Phase 3: Spectrum / Panadapter Control
- Build a high-performance custom control using WriteableBitmap or SkiaSharp
- Define clean data contracts between Core and UI

### Phase 4: Main Controls & Meters
- Rebuild VFO controls, band buttons, filters, volume, etc.
- Replace placeholder meters with real ones

### Phase 5: Hardware Integration
- Implement real UDP (and optionally FTDI) communication
- Gradually replace mock with real hardware link

### Phase 6: Polish & Feature Parity
- Theming, keyboard shortcuts, settings, etc.
- Performance tuning

## Current Status
- Phase 1 in progress (basic shell + ViewModel created)
