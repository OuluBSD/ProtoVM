#ifndef _LogicAnalyzer_LiveCaptureTest_h
#define _LogicAnalyzer_LiveCaptureTest_h

// Headless self-test for live capture: builds a small ProtoVM circuit, probes a
// signal with MachineCaptureAdapter, and checks the CaptureModel gets a track
// with transitions. Returns true on success.
bool RunLiveCaptureSelfTest();

#endif
