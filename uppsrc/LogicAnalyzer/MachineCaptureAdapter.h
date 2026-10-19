#ifndef _LogicAnalyzer_MachineCaptureAdapter_h
#define _LogicAnalyzer_MachineCaptureAdapter_h

#include "CaptureModel.h"
#include "TraceProbe.h"

class Machine;
class Pcb;
class ElectricNodeBase;

// Bridges a ProtoVM Machine to the Logic Analyzer CaptureModel: adds passive
// probes to selected component pins, advances the simulation sampling every
// tick, and converts the recorded transitions into raw capture tracks.
class MachineCaptureAdapter {
public:
	MachineCaptureAdapter(Machine& mach, Pcb& pcb);

	// Probe a component's source pin. Call before Machine::Init().
	int AddProbe(ElectricNodeBase& component, const String& pin_name);

	// Advance the simulation, sampling every probe each tick.
	bool Run(int ticks);

	int GetProbeCount() const { return probes.GetCount(); }
	String GetProbeName(int i) const { return i >= 0 && i < probe_names.GetCount() ? probe_names[i] : String(); }

	// Convert recorded probe data into one raw track per probe.
	// tick_us is the wall-clock duration (us) of one simulation tick.
	void BuildModel(CaptureModel& model, int64 tick_us = 1) const;

private:
	Machine& mach;
	Pcb& pcb;
	Vector<TraceProbe*> probes;
	Vector<String> probe_names;
};

#endif
