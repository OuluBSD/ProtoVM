#include "ProtoVM.h"
#include "ClockDivider.h"

ClockDivider::ClockDivider(int factor) {
    // Ensure the division factor is at least 2 (dividing by 1 would just pass through)
    division_factor = (factor >= 2) ? factor : 2;
    
    // Add input and output connections
    AddSink("CLK_IN");   // Input clock
    AddSource("CLK_OUT").SetMultiConn();  // Output clock (divided)
    AddSource("COUNT").SetMultiConn();    // Current counter value for debugging
    
    // Initialize state
    counter = 0;
    output_clock = false;
    input_clock = false;
    last_input_clock = false;
    
    SetName(String().Cat() << "ClockDivider_DIV" << division_factor);
}

void ClockDivider::SetDivisionFactor(int factor) {
    division_factor = (factor >= 2) ? factor : 2;
    // Reset counter when factor changes to ensure consistent behavior
    counter = 0;
}

bool ClockDivider::Tick() {
    // Detect rising edge on input clock
    bool rising_edge = (input_clock && !last_input_clock);
    
    if (rising_edge) {
        // Increment the counter on each input clock rising edge
        counter++;
        
        // When counter reaches the division factor, toggle the output
        if (counter >= division_factor) {
            output_clock = !output_clock;  // Toggle output clock
            counter = 0;  // Reset counter
        }
    }
    
    // Update last input state for next tick
    last_input_clock = input_clock;
    
    // Set changed flag if output clock changed
    bool changed = HasChanged();
    SetChanged(output_clock != (counter == 0));  // Simplified change detection
    
    return true;
}

bool ClockDivider::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id == 1) {  // CLK_OUT output
            return dest.PutRaw(dest_conn_id, (byte*)&output_clock, 0, 1);
        } else if (conn_id == 2) {  // COUNT output (for debugging)
            // Output the counter value as a byte
            byte count_byte = counter & 0xFF;  // Use only the lower 8 bits
            return dest.PutRaw(dest_conn_id, &count_byte, 0, 1);
        }
    }
    return true;
}

bool ClockDivider::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    ASSERT(data_bytes == 0 && data_bits == 1);

    if (conn_id == 0) {  // CLK_IN input
        input_clock = (*data & 1) ? true : false;
    }

    return true;
}