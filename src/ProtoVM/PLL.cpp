#include "ProtoVM.h"
#include "PLL.h"

PLL::PLL(int mult_factor) {
    // Set the multiplication factor (at least 2 to actually multiply)
    multiplication_factor = (mult_factor >= 2) ? mult_factor : 2;
    
    // Calculate output frequency parameters
    // In this simplified model, we'll assume the input period is 10 ticks (arbitrary)
    // So if we want to multiply by N, we need to generate N edges in the same time
    // So output period = input period / multiplication_factor
    max_output_count = (10 / multiplication_factor > 0) ? (10 / multiplication_factor) : 1;
    
    // Add input and output connections
    AddSink("CLK_IN");    // Input reference clock
    AddSink("RST");       // Reset input
    AddSource("CLK_OUT").SetMultiConn();  // Output multiplied clock
    AddSource("LOCKED").SetMultiConn();   // Lock status indicator
    
    // Initialize state
    input_clock = false;
    output_clock = false;
    output_counter = 0;
    locked = false;
    
    SetName(String().Cat() << "PLL_X" << multiplication_factor);
}

void PLL::SetMultiplicationFactor(int factor) {
    multiplication_factor = (factor >= 2) ? factor : 2;
    max_output_count = (10 / multiplication_factor > 0) ? (10 / multiplication_factor) : 1;
    // Reset state when factor changes
    output_counter = 0;
    locked = false;
}

bool PLL::Tick() {
    // Simple PLL simulation:
    // When input clock rises, update the output clock counter
    // The output clock toggles at a rate that's a multiple of the input
    
    // For simplicity in this simulation, we'll increment the counter each tick
    // and toggle the output when we've counted enough ticks to achieve the 
    // desired multiplication factor
    
    output_counter++;
    
    // If we've counted to our max count, toggle the output
    if (output_counter >= max_output_count) {
        output_clock = !output_clock;
        output_counter = 0;
        
        // After some cycles, consider the PLL locked
        if (!locked && output_counter > lock_threshold) {
            locked = true;
        }
    }
    
    // Set changed flag based on state changes
    bool changed = HasChanged();
    SetChanged(changed);
    
    return true;
}

bool PLL::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id == 2) {  // CLK_OUT output
            return dest.PutRaw(dest_conn_id, (byte*)&output_clock, 0, 1);
        } else if (conn_id == 3) {  // LOCKED output
            byte lock_status = locked ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &lock_status, 0, 1);
        }
    }
    return true;
}

bool PLL::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    ASSERT(data_bytes == 0 && data_bits == 1);

    if (conn_id == 0) {  // CLK_IN input
        input_clock = (*data & 1) ? true : false;
    }
    else if (conn_id == 1) {  // RST input
        bool reset = (*data & 1) ? true : false;
        if (reset) {
            // Reset the PLL state
            output_counter = 0;
            output_clock = false;
            locked = false;
        }
    }

    return true;
}