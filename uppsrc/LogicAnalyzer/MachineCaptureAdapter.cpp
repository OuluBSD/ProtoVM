#include "MachineCaptureAdapter.h"

#include <ProtoVM/ProtoVM.h>

MachineCaptureAdapter::MachineCaptureAdapter(Machine& mach, Pcb& pcb)
	: mach(mach), pcb(pcb) {}

int MachineCaptureAdapter::AddProbe(ElectricNodeBase& component, const String& pin_name) {
	String pname = component.GetName() + "." + pin_name;
	TraceProbe& probe = pcb.Add<TraceProbe>(pname);
	component[pin_name] >> probe["IN"];
	probes.Add(&probe);
	probe_names.Add(pname);
	return probes.GetCount() - 1;
}

bool MachineCaptureAdapter::Run(int ticks) {
	for(int i = 0; i < ticks; i++) {
		if(!mach.Tick())
			return false;
		for(TraceProbe* p : probes)
			p->Sample(mach.current_tick);
	}
	return true;
}

void MachineCaptureAdapter::BuildModel(CaptureModel& model, int64 tick_us) const {
	int64 end = 0;
	for(int i = 0; i < probes.GetCount(); i++) {
		Track& tr = model.GetTracks().Add();
		tr.name = probe_names[i];
		tr.kind = TK_RAW;
		tr.color = Color(60 + (i * 47) % 180, 110, 190);

		const Vector<int>& ticks = probes[i]->GetTicks();
		const Vector<byte>& values = probes[i]->GetValues();
		for(int j = 0; j < ticks.GetCount(); j++) {
			int64 t = (int64)ticks[j] * tick_us;
			bool v = (values[j] & 1) != 0;

			Sample s;
			s.time = t;
			s.value = v;
			tr.samples.Add(s);

			Transition tg;
			tg.time = t;
			tg.value = v;
			tr.transitions.Add(tg);

			if(t > end)
				end = t;
		}
	}
	model.SetTimeRange(0, end > 0 ? end : 1);
}
