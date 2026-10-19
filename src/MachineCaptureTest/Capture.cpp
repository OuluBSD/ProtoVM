#include "../ProtoVM/ProtoVM.h"

#include <Core/Core.h>

using namespace Upp;

// A trivial multi-bit source that emits a changing value every 2 ticks.
struct PatternSource : public ElcBase {
	int tick = 0;
	PatternSource() { AddSource("OUT").SetMultiConn(); }

	bool Tick() override { tick++; SetChanged(true); return true; }

	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
		if(type == WRITE && conn_id == 0) {
			byte value = byte((tick / 2) & 0x0F);   // 4-bit ramp, changes every 2 ticks
			return dest.PutRaw(dest_conn_id, &value, 0, 1);
		}
		return true;
	}

	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override { return true; }
};

// The candidate probe: a passive sink that records whatever is written to it.
struct CaptureProbe : public ElcBase {
	byte last_value = 0;
	bool has_value = false;
	int transitions = 0;

	CaptureProbe() { AddSink("IN").SetRequired(false); }

	bool Tick() override { return true; }

	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
		return true;
	}

	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
		if(data && (data_bytes > 0 || data_bits > 0)) {
			byte v = data[0];
			if(!has_value || v != last_value)
				transitions++;
			last_value = v;
			has_value = true;
		}
		return true;
	}
};

CONSOLE_APP_MAIN {
	Machine mach;
	Pcb& pcb = mach.AddPcb();

	PatternSource& src = pcb.Add<PatternSource>("src");
	CaptureProbe& probe = pcb.Add<CaptureProbe>("probe");

	src["OUT"] >> probe["IN"];

	if(!mach.Init()) {
		Cout() << "Init failed\n";
		return;
	}

	String wave;
	for(int i = 0; i < 16; i++) {
		mach.Tick();
		wave << (int)probe.last_value;
	}

	Cout() << "Samples:    " << wave << "\n";
	Cout() << "Transitions recorded by probe: " << probe.transitions << "\n";
	Cout() << "Machine transitions (engine tracing, still 0): " << mach.GetSignalTransitionCount() << "\n";

	if(probe.transitions > 0)
		Cout() << "PROBE-SPIKE PASSED\n";
	else
		Cout() << "PROBE-SPIKE FAILED\n";
}
