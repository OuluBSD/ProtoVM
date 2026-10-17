# LogicAnalyzer Package

Logic Analyzer implementation for ProtoVM with headless capture model and U++ GUI.

## Structure

- `CaptureModel` - Headless model/controller for capture tracks, raw samples, decoded blocks, UART decoder
- `WaveformView` - Custom waveform rendering with channel headers, time ruler, zoom/pan
- `UartSettingsDlg` - UART configuration dialog
- `CustomTracks` - PC:Bits and PC:Address/Data custom track implementations
- `DemoCapture` - Demo capture data generator

## Conventions

- Follows U++ BLITZ/header conventions: main header first in each .cpp
- No comments unless necessary
- Platform-neutral paths
- LOG only if needed