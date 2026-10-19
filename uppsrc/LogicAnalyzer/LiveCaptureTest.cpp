#include "LiveCaptureTest.h"

#include "MachineCaptureAdapter.h"

#include <ProtoVM/ProtoVM.h>

using namespace Upp;

namespace {

// A source that toggles its output every two ticks (like a slow clock).
struct ToggleClock : public ElcBase {
	int t = 0;

	ToggleClock() { AddSource("OUT"); }

	bool Tick() override { t++; SetChanged(true); return true; }

	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
		if(type == WRITE && conn_id == 0) {
			byte v = (t / 2) & 1;
			return dest.PutRaw(dest_conn_id, &v, 0, 1);
		}
		return true;
	}

	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override { return true; }
};

}

bool RunLiveCaptureSelfTest() {
	Machine mach;
	Pcb& pcb = mach.AddPcb();

	ToggleClock& clk = pcb.Add<ToggleClock>("clk");

	MachineCaptureAdapter adapter(mach, pcb);
	adapter.AddProbe(clk, "OUT");

	if(!mach.Init())
		return false;

	if(!adapter.Run(16))
		return false;

	CaptureModel model;
	adapter.BuildModel(model, 1);

	if(model.GetTrackCount() != 1)
		return false;

	const Track& tr = model.GetTrack(0);
	if(tr.transitions.GetCount() == 0)
		return false;

	Cout() << "Live capture: " << tr.name << " -> " << tr.transitions.GetCount()
	       << " transitions, " << model.GetTimeEnd() << " us\n";
	return true;
}
