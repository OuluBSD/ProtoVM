#ifndef _LogicAnalyzer_WaveformView_h
#define _LogicAnalyzer_WaveformView_h

#include <CtrlLib/CtrlLib.h>
#include "CaptureModel.h"

using namespace Upp;

class WaveformView : public Ctrl {
public:
	typedef WaveformView CLASSNAME;
	
	WaveformView();
	
	void SetModel(CaptureModel* m) { model = m; Refresh(); }
	CaptureModel* GetModel() const { return model; }
	
	void SetTimeRange(int64 start, int64 end) { timeStart = start; timeEnd = end; Refresh(); }
	void SetPixelsPerUs(double p) { pixelsPerUs = p; Refresh(); }
	double GetPixelsPerUs() const { return pixelsPerUs; }
	
	void ZoomIn(double factor = 2.0) { pixelsPerUs *= factor; Refresh(); }
	void ZoomOut(double factor = 2.0) { pixelsPerUs /= factor; Refresh(); }
	void ZoomFit();
	
	void SetCursorPosition(int64 time) { cursorTime = time; Refresh(); }
	int64 GetCursorPosition() const { return cursorTime; }
	
	virtual void Paint(Draw& w) override;
	virtual void LeftDown(Point p, dword keyflags) override;
	virtual void LeftUp(Point p, dword keyflags) override;
	virtual void MouseMove(Point p, dword keyflags) override;
	virtual void MouseWheel(Point p, int zdelta, dword keyflags) override;
	virtual Image CursorImage(Point p, dword keyflags) override;
	virtual void Layout() override;
	
	Callback WhenCursorChange;
	Callback WhenSelectionChange;
	
	void SetChannelHeight(int h) { channelHeight = h; Refresh(); }
	int GetChannelHeight() const { return channelHeight; }
	
	void SetShowDecoded(bool b) { showDecoded = b; Refresh(); }
	bool GetShowDecoded() const { return showDecoded; }

private:
	CaptureModel* model = nullptr;
	
	int64 timeStart = 0;
	int64 timeEnd = 1000000;
	double pixelsPerUs = 1.0;
	
	int64 cursorTime = -1;
	int64 selectionStart = -1;
	int64 selectionEnd = -1;
	
	int channelHeight = 40;
	int headerWidth = 180;
	int rulerHeight = 30;
	bool showDecoded = true;
	
	bool panning = false;
	Point panStart;
	int64 panTimeStart;
	
	Rect GetWaveformArea() const;
	Rect GetHeaderArea() const;
	Rect GetRulerArea() const;
	
	void DrawHeader(Draw& w, const Rect& r);
	void DrawRuler(Draw& w, const Rect& r);
	void DrawWaveform(Draw& w, const Rect& r);
	void DrawChannel(Draw& w, const Rect& r, int trackIndex, int64 visStart, int64 visEnd);
	void DrawRawWaveform(Draw& w, const Rect& r, const Track& track, int64 visStart, int64 visEnd);
	void DrawDecodedBlocks(Draw& w, const Rect& r, const Track& track, int64 visStart, int64 visEnd);
	
	int TimeToX(int64 t) const;
	int64 XToTime(int x) const;
	int TrackToY(int trackIndex) const;
	int TrackIndexFromY(int y) const;
};

#endif