#ifndef _LogicAnalyzer_TraceProbe_h
#define _LogicAnalyzer_TraceProbe_h

#include <ProtoVM/Common.h>

// Passive sink that records the values written to it by a driver. Attach one to
// a signal's source connector (before Machine::Init) to observe that signal
// without altering the circuit.
class TraceProbe : public ElectricNodeBase {
public:
	TraceProbe() { AddSink("IN").SetRequired(false); }

	bool Tick() override { return true; }

	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
		return true;
	}

	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
		if(data && (data_bytes > 0 || data_bits > 0)) {
			last_value = data[0];
			has_value = true;
		}
		return true;
	}

	// Record the last written value for the given tick, if it changed.
	void Sample(int tick) {
		if(!has_value)
			return;
		if(values.IsEmpty() || values.Top() != last_value) {
			ticks.Add(tick);
			values.Add(last_value);
		}
	}

	byte GetLastValue() const { return last_value; }
	bool HasValue() const { return has_value; }
	int GetTransitionCount() const { return ticks.GetCount(); }
	const Vector<int>& GetTicks() const { return ticks; }
	const Vector<byte>& GetValues() const { return values; }

	void Clear() {
		ticks.Clear();
		values.Clear();
		has_value = false;
		last_value = 0;
	}

private:
	byte last_value = 0;
	bool has_value = false;
	Vector<int> ticks;
	Vector<byte> values;
};

#endif
