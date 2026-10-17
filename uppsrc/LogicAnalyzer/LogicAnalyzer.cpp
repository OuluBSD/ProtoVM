#include "LogicAnalyzer.h"
#include "DemoCapture.h"
#include <Core/Core.h>

LogicAnalyzer::LogicAnalyzer() {
	Title("Logic Analyzer - ProtoVM");
	Sizeable().Zoomable().CenterScreen();
	SetRect(0, 0, 1200, 800);
	
	SetupMenu();
	SetupToolbar();
	SetupStatusBar();
	
	Add(waveformView.SizePos());
	waveformView.SetModel(&model);
	
	GenerateDemoCapture(model, 8, 1000000);
	model.DecodeAllUart();
	waveformView.SetTimeRange(model.GetTimeStart(), model.GetTimeEnd());
	waveformView.ZoomFit();
	
	UpdateTitle();
	UpdateStatusBar();
}

LogicAnalyzer::~LogicAnalyzer() {
}

void LogicAnalyzer::Run() {
	OpenMain();
}

void LogicAnalyzer::SetupMenu() {
	menuBar.Set(THISBACK(MenuCallback));
	AddFrame(menuBar);
}

void LogicAnalyzer::MenuCallback(Bar& bar) {
	bar.Add("File", THISBACK(FileMenu));
	bar.Add("View", THISBACK(ViewMenu));
	bar.Add("Capture", THISBACK(CaptureMenu));
	bar.Add("Tools", THISBACK(ToolsMenu));
	bar.Add("Help", THISBACK(HelpMenu));
}

void LogicAnalyzer::FileMenu(Bar& bar) {
	bar.Add("New Capture", CtrlImg::new_doc(), THISBACK(OnNewCapture)).Key(K_CTRL_N);
	bar.Add("Demo Capture", CtrlImg::File(), THISBACK(OnDemoCapture)).Key(K_CTRL_D);
	bar.Separator();
	bar.Add("Exit", THISBACK(OnClose)).Key(K_ALT_F4);
}

void LogicAnalyzer::ViewMenu(Bar& bar) {
	bar.Add("Zoom In", THISBACK(OnZoomIn)).Key(K_CTRL | K_UP);
	bar.Add("Zoom Out", THISBACK(OnZoomOut)).Key(K_CTRL | K_DOWN);
	bar.Add("Zoom Fit", THISBACK(OnZoomFit)).Key(K_CTRL | K_0);
	bar.Separator();
	bar.Add("Channels Dock", THISBACK(ToggleChannelsDock)).Key(K_F2);
	bar.Add("Decoded Dock", THISBACK(ToggleDecodedDock)).Key(K_F3);
	bar.Add("Properties Dock", THISBACK(TogglePropertiesDock)).Key(K_F4);
}

void LogicAnalyzer::CaptureMenu(Bar& bar) {
	bar.Add("Run/Stop", THISBACK(OnRunStop)).Key(K_F5);
	bar.Separator();
	bar.Add("UART Settings...", THISBACK(OnUartSettings));
	bar.Add("Add Custom Track", THISBACK(OnAddCustomTrack));
	bar.Separator();
	bar.Add("Generate SPI Demo", THISBACK(GenSpiDemo));
	bar.Add("Generate I2C Demo", THISBACK(GenI2cDemo));
}

void LogicAnalyzer::ToolsMenu(Bar& bar) {
	bar.Add("Self Test", THISBACK(RunSelfTest));
}

void LogicAnalyzer::HelpMenu(Bar& bar) {
	bar.Add("About", THISBACK(OnHelpAbout));
}

void LogicAnalyzer::SetupToolbar() {
	toolBar.Set(THISBACK(ToolbarCallback));
	AddFrame(toolBar);
}

void LogicAnalyzer::ToolbarCallback(Bar& bar) {
	bar.Add("New", CtrlImg::new_doc(), THISBACK(OnNewCapture));
	bar.Add("Demo", CtrlImg::File(), THISBACK(OnDemoCapture));
	bar.Separator();
	bar.Add("Run/Stop", THISBACK(OnRunStop));
	bar.Separator();
	bar.Add("Zoom In", THISBACK(OnZoomIn));
	bar.Add("Zoom Out", THISBACK(OnZoomOut));
	bar.Add("Zoom Fit", THISBACK(OnZoomFit));
	bar.Separator();
	bar.Add("UART Settings", THISBACK(OnUartSettings));
	bar.Add("Add Custom Track", THISBACK(OnAddCustomTrack));
}

void LogicAnalyzer::SetupStatusBar() {
	statusBar.Left(100);  // Cursor position
	statusBar.Left(150);  // Selection
	statusBar.Left(100);  // Sample rate
	statusBar.Left(200);  // Time range
	AddFrame(statusBar);
}

void LogicAnalyzer::DockInit() {
	AllowDockAll();
	
	channelsDock = &CreateDockable<DockableCtrl>("Channels");
	DockLeft(*channelsDock);
	
	decodedDock = &CreateDockable<DockableCtrl>("Decoded");
	DockBottom(*decodedDock);
	
	propertiesDock = &CreateDockable<DockableCtrl>("Properties");
	DockRight(*propertiesDock);
}

void LogicAnalyzer::UpdateTitle() {
	String title = "Logic Analyzer - ProtoVM";
	if(running) title += " [RUNNING]";
	Title(title);
}

void LogicAnalyzer::UpdateStatusBar() {
	statusBar.Set(0, Format("Cursor: %s", waveformView.GetCursorPosition() >= 0 ? 
		TimeToString(waveformView.GetCursorPosition()) : "---"), 100);
	
	if(waveformView.GetModel()) {
		statusBar.Set(2, Format("Rate: %d Hz", waveformView.GetModel()->GetSampleRate()), 100);
		statusBar.Set(3, Format("Range: %s - %s", 
			TimeToString(waveformView.GetModel()->GetTimeStart()),
			TimeToString(waveformView.GetModel()->GetTimeEnd())), 200);
	}
}

String LogicAnalyzer::TimeToString(int64 us) {
	if(us >= 1000000) return Format("%.3f s", us / 1000000.0);
	if(us >= 1000) return Format("%.2f ms", us / 1000.0);
	return Format("%d us", (int)us);
}

void LogicAnalyzer::OnNewCapture() {
	model.Clear();
	waveformView.Refresh();
	UpdateTitle();
	UpdateStatusBar();
}

void LogicAnalyzer::OnDemoCapture() {
	GenerateDemoCapture(model, 8, 1000000);
	model.DecodeAllUart();
	waveformView.SetTimeRange(model.GetTimeStart(), model.GetTimeEnd());
	waveformView.ZoomFit();
	UpdateTitle();
	UpdateStatusBar();
}

void LogicAnalyzer::OnClose() {
	Break();
}

void LogicAnalyzer::OnRunStop() {
	running = !running;
	UpdateTitle();
	toolBar.Refresh();
	
	if(running) {
		statusBar.Set(0, "Running...", 100);
	} else {
		statusBar.Set(0, "Stopped", 100);
	}
}

void LogicAnalyzer::OnZoomIn() {
	waveformView.ZoomIn();
	UpdateStatusBar();
}

void LogicAnalyzer::OnZoomOut() {
	waveformView.ZoomOut();
	UpdateStatusBar();
}

void LogicAnalyzer::OnZoomFit() {
	waveformView.ZoomFit();
	UpdateStatusBar();
}

void LogicAnalyzer::OnUartSettings() {
	if(model.GetUartConfigs().IsEmpty()) {
		UartConfig cfg;
		model.GetUartConfigs().Add(cfg);
	}
	
	UartConfig& cfg = model.GetUartConfigs()[0];
	Array<String> channels;
	for(int i = 0; i < model.GetTrackCount(); i++) {
		channels.Add(model.GetTrack(i).name);
	}
	
	UartSettingsDlg dlg(cfg, channels);
	if(dlg.Execute() == IDOK) {
		model.DecodeAllUart();
		waveformView.Refresh();
	}
}

void LogicAnalyzer::OnAddCustomTrack() {
	MenuBar menu;
	menu.Add("PC: Bits", THISBACK1(AddCustomTrack, TK_CUSTOM_BITS));
	menu.Add("PC: Address/Data", THISBACK1(AddCustomTrack, TK_CUSTOM_ADDR_DATA));
	menu.Execute();
}

void LogicAnalyzer::AddCustomTrack(TrackKind kind) {
	model.AddCustomTrack(kind == TK_CUSTOM_BITS ? "PC: Bits" : "PC: Address/Data", kind);
	waveformView.Refresh();
	UpdateStatusBar();
}

void LogicAnalyzer::OnHelpAbout() {
	PromptOK("Logic Analyzer for ProtoVM\n\n"
	         "Features:\n"
	         "- Multi-channel waveform capture\n"
	         "- UART decode with configurable settings\n"
	         "- Custom track decoders (PC: Bits, PC: Address/Data)\n"
	         "- Zoom/Pan/Cursor interactions\n"
	         "- Dockable channel/decoded/properties panels\n\n"
	         "Part of ProtoVM project");
}

void LogicAnalyzer::ToggleChannelsDock() {
	if(channelsDock) channelsDock->Show(!channelsDock->IsVisible());
}

void LogicAnalyzer::ToggleDecodedDock() {
	if(decodedDock) decodedDock->Show(!decodedDock->IsVisible());
}

void LogicAnalyzer::TogglePropertiesDock() {
	if(propertiesDock) propertiesDock->Show(!propertiesDock->IsVisible());
}

void LogicAnalyzer::GenSpiDemo() {
	GenerateSpiDemo(model, 16);
	waveformView.SetTimeRange(model.GetTimeStart(), model.GetTimeEnd());
	waveformView.ZoomFit();
	UpdateStatusBar();
}

void LogicAnalyzer::GenI2cDemo() {
	GenerateI2cDemo(model);
	waveformView.SetTimeRange(model.GetTimeStart(), model.GetTimeEnd());
	waveformView.ZoomFit();
	UpdateStatusBar();
}

void LogicAnalyzer::RunSelfTest() {
	if(CaptureModel::RunSelfTest()) {
		PromptOK("Self-test PASSED");
	} else {
		PromptOK("Self-test FAILED");
	}
}

GUI_APP_MAIN {
	for(String arg : CommandLine())
		if(arg == "--self-test") {
			Cout() << (CaptureModel::RunSelfTest() ? "Self-test PASSED\n" : "Self-test FAILED\n");
			return;
		}
	
	LogicAnalyzer app;
	app.Run();
}