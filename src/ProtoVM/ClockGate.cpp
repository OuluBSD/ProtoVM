#include "ProtoVM.h"
#include "ClockGate.h"

ClockGate::ClockGate() {
    // Add input and output connections
    AddSink("CLK_IN");    // Input clock
    AddSink("EN");        // Enable signal
    AddSource("CLK_OUT").SetMultiConn();  // Output clock (gated)
    
    // Initialize state
    input_clock = false;
    enable_signal = false;
    output_clock = false;
    last_enable = false;
    
    SetName("ClockGate");
}

bool ClockGate::Tick() {
    // When enable signal is high, pass the input clock to output
    // When enable signal is low, output remains at its previous state or low
    if (enable_signal) {
        output_clock = input_clock;
    }
    // If enable is low, keep the same output (no change)
    
    // Set changed flag if output changed
    bool changed = (output_clock != input_clock || enable_signal); // Simplified
    SetChanged(changed);
    
    return true;
}

bool ClockGate::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id == 2) {  // CLK_OUT output
            return dest.PutRaw(dest_conn_id, (byte*)&output_clock, 0, 1);
        }
    }
    return true;
}

bool ClockGate::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    ASSERT(data_bytes == 0 && data_bits == 1);

    if (conn_id == 0) {  // CLK_IN input
        input_clock = (*data & 1) ? true : false;
    }
    else if (conn_id == 1) {  // EN input
        enable_signal = (*data & 1) ? true : false;
    }

    return true;
}