topic "Logic Analyzer Package";
[
	$$0,0#00000000000000000000000000000000:Default
	[{_}%EN-US 
	[ {{10000@(113.42.0) Logic Analyzer}}
	
	This package provides a Logic Analyzer implementation for ProtoVM with:
	- Headless capture model with raw samples, transitions, and decoded blocks
	- UART protocol decoder with configurable settings
	- Custom track support (PC: Bits, PC: Address/Data)
	- GUI with waveform view, zoom/pan, cursor interactions
	- Dockable panels for channels, decoded data, and properties
	- MenuBar, ToolBar, StatusBar
	
	## Classes
	
	- `CaptureModel`: Headless model for capture data
	- `WaveformView`: Custom waveform rendering widget
	- `UartSettingsDlg`: UART configuration dialog
	- `CustomTrackDecoder`: Base class for custom decoders
	
	## Usage
	
	Build with: `umk LogicAnalyzer`
	
	Run headless test: `CaptureModel::RunSelfTest()`
]