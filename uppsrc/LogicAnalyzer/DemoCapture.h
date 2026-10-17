#ifndef _LogicAnalyzer_DemoCapture_h
#define _LogicAnalyzer_DemoCapture_h

#include <Core/Core.h>
#include "CaptureModel.h"

using namespace Upp;

void GenerateDemoCapture(CaptureModel& model, int numChannels = 8, int64 durationUs = 1000000);
void GenerateUartDemo(CaptureModel& model, const String& rxData, const String& txData, int baudRate = 9600);
void GenerateSpiDemo(CaptureModel& model, int numBytes = 16);
void GenerateI2cDemo(CaptureModel& model);

#endif