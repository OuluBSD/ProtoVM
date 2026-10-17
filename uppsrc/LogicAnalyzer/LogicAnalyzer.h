#ifndef _LogicAnalyzer_LogicAnalyzer_h
#define _LogicAnalyzer_LogicAnalyzer_h

#ifndef flagGUI
#define flagGUI
#endif

#include <CtrlLib/CtrlLib.h>
#include <Docking/Docking.h>

#include "CaptureModel.h"
#include "WaveformView.h"
#include "UartSettingsDlg.h"
#include "CustomTracks.h"
#include "DemoCapture.h"

using namespace Upp;

#define IMAGECLASS LogicAnalyzerImg
#define IMAGEFILE <LogicAnalyzer/LogicAnalyzer.iml>
#include <Draw/iml_header.h>

class LogicAnalyzer : public DockWindow {
public:
	typedef LogicAnalyzer CLASSNAME;

	LogicAnalyzer();
	virtual ~LogicAnalyzer();

	void Run();

private:
	void SetupMenu();
	void SetupToolbar();
	void DockInit() override;
	void SetupStatusBar();

	void OnNewCapture();
	void OnDemoCapture();
	void OnRunStop();
	void OnZoomIn();
	void OnZoomOut();
	void OnZoomFit();
	void OnUartSettings();
	void OnAddCustomTrack();
	void OnHelpAbout();
	void OnClose();
	
	void ToggleChannelsDock();
	void ToggleDecodedDock();
	void TogglePropertiesDock();
	void GenSpiDemo();
	void GenI2cDemo();
	void RunSelfTest();
	
	void UpdateTitle();
	void UpdateStatusBar();
	
	String TimeToString(int64 us);

	void MenuCallback(Bar& bar);
	void FileMenu(Bar& bar);
	void ViewMenu(Bar& bar);
	void CaptureMenu(Bar& bar);
	void ToolsMenu(Bar& bar);
	void HelpMenu(Bar& bar);
	void ToolbarCallback(Bar& bar);
	void AddCustomTrack(TrackKind kind);

	CaptureModel model;
	WaveformView waveformView;

	MenuBar menuBar;
	ToolBar toolBar;
	StatusBar statusBar;

	DockableCtrl* channelsDock = nullptr;
	DockableCtrl* decodedDock = nullptr;
	DockableCtrl* propertiesDock = nullptr;

	bool running = false;
	int sampleRate = 1000000;
};

#endif