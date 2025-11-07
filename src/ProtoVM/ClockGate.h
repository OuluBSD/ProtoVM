#ifndef _ProtoVM_ClockGate_h_
#define _ProtoVM_ClockGate_h_

#include "ProtoVM.h"

// ClockGate - Controls whether a clock signal is passed through based on an enable signal
// When enable is high, the input clock is passed to the output
// When enable is low, the output clock remains static (typically low) regardless of input
class ClockGate : public ElcBase {
private:
    // Internal state
    bool input_clock = false;     // Current input clock state
    bool enable_signal = false;   // Enable signal (when high, clock passes through)
    bool output_clock = false;    // Current output clock state
    bool last_enable = false;     // Previous enable state for edge detection

public:
    ClockGate();
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // State methods
    bool GetOutputClock() const { return output_clock; }
    bool GetInputClock() const { return input_clock; }
    bool GetEnableSignal() const { return enable_signal; }
};

#endif