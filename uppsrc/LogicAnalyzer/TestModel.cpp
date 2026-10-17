#include "CaptureModel.h"
#include "DemoCapture.h"
#include <Core/Core.h>

using namespace Upp;

CONSOLE_APP_MAIN {
	CaptureModel model;
	GenerateDemoCapture(model, 8, 1000000);
	model.DecodeAllUart();
	
	Cout() << "Demo capture generated with " << model.GetTrackCount() << " tracks\n";
	Cout() << "Time range: " << model.GetTimeStart() << " to " << model.GetTimeEnd() << " us\n";
	
	for(int i = 0; i < model.GetTrackCount(); i++) {
		const Track& t = model.GetTrack(i);
		Cout() << "Track " << i << ": " << t.name << ", " << t.samples.GetCount() << " samples, " << t.transitions.GetCount() << " transitions, " << t.decodedBlocks.GetCount() << " decoded blocks\n";
	}
	
	if(!model.RunSelfTest()) {
		Cout() << "SELF-TEST FAILED\n";
	}
	
	Cout() << "SELF-TEST PASSED\n";
}