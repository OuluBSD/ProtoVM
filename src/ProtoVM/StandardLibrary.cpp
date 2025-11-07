#include "StandardLibrary.h"

// Implementation of 3-input AND gate
AndGate3::AndGate3() {
    AddSink("I0");
    AddSink("I1");
    AddSink("I2");
    AddSource("O").SetMultiConn();
}

bool AndGate3::Tick() {
    out = in[0] && in[1] && in[2];
    return true;
}

bool AndGate3::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id >= 0 && conn_id <= 2) {
            // Skip inputs - handled by PutRaw
        } else if (conn_id == 3) {  // Output
            return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
        }
    }
    return true;
}

bool AndGate3::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id >= 0 && conn_id <= 2) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        in[conn_id] = *data & 1;
    } else {
        LOG("error: AndGate3: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 4-input AND gate
AndGate4::AndGate4() {
    AddSink("I0");
    AddSink("I1");
    AddSink("I2");
    AddSink("I3");
    AddSource("O").SetMultiConn();
}

bool AndGate4::Tick() {
    out = in[0] && in[1] && in[2] && in[3];
    return true;
}

bool AndGate4::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id >= 0 && conn_id <= 3) {
            // Skip inputs - handled by PutRaw
        } else if (conn_id == 4) {  // Output
            return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
        }
    }
    return true;
}

bool AndGate4::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id >= 0 && conn_id <= 3) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        in[conn_id] = *data & 1;
    } else {
        LOG("error: AndGate4: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 3-input OR gate
OrGate3::OrGate3() {
    AddSink("I0");
    AddSink("I1");
    AddSink("I2");
    AddSource("O").SetMultiConn();
}

bool OrGate3::Tick() {
    out = in[0] || in[1] || in[2];
    return true;
}

bool OrGate3::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id >= 0 && conn_id <= 2) {
            // Skip inputs - handled by PutRaw
        } else if (conn_id == 3) {  // Output
            return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
        }
    }
    return true;
}

bool OrGate3::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id >= 0 && conn_id <= 2) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        in[conn_id] = *data & 1;
    } else {
        LOG("error: OrGate3: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 4-input OR gate
OrGate4::OrGate4() {
    AddSink("I0");
    AddSink("I1");
    AddSink("I2");
    AddSink("I3");
    AddSource("O").SetMultiConn();
}

bool OrGate4::Tick() {
    out = in[0] || in[1] || in[2] || in[3];
    return true;
}

bool OrGate4::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id >= 0 && conn_id <= 3) {
            // Skip inputs - handled by PutRaw
        } else if (conn_id == 4) {  // Output
            return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
        }
    }
    return true;
}

bool OrGate4::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id >= 0 && conn_id <= 3) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        in[conn_id] = *data & 1;
    } else {
        LOG("error: OrGate4: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of Buffer
Buffer::Buffer() {
    AddSink("I");
    AddSource("O").SetMultiConn();
}

bool Buffer::Tick() {
    out = in;
    return true;
}

bool Buffer::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id == 0) {  // Input
            // Skip - handled by PutRaw
        } else if (conn_id == 1) {  // Output
            return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
        }
    }
    return true;
}

bool Buffer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0) {  // Input
        ASSERT(data_bytes == 0 && data_bits == 1);
        in = *data & 1;
    } else {
        LOG("error: Buffer: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of Tri-State Buffer
TriStateBuffer::TriStateBuffer() {
    AddSink("I");    // Input
    AddSink("OE");   // Output Enable
    AddSource("O").SetMultiConn(); // Output
}

bool TriStateBuffer::Tick() {
    // If output enable is high (active), pass input to output
    // Otherwise, output is effectively disconnected (high-impedance)
    output = enable ? input : output;  // Keep previous state when disabled
    return true;
}

bool TriStateBuffer::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        if (conn_id == 0) {  // Input
            // Skip - handled by PutRaw
        } else if (conn_id == 1) {  // Output Enable
            // Skip - handled by PutRaw
        } else if (conn_id == 2) {  // Output
            // Only pass output if enabled
            if (enable) {
                return dest.PutRaw(dest_conn_id, (byte*)&output, 0, 1);
            }
            // If not enabled, don't pass anything (high-impedance state)
            return true;
        }
    }
    return true;
}

bool TriStateBuffer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0) {  // Input
        ASSERT(data_bytes == 0 && data_bits == 1);
        input = *data & 1;
    } else if (conn_id == 1) {  // Output Enable
        ASSERT(data_bytes == 0 && data_bits == 1);
        enable = *data & 1;
        // When enable is activated, update output immediately
        if (enable) {
            output = input;
        }
    } else {
        LOG("error: TriStateBuffer: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 8-bit register
Register8Bit::Register8Bit() {
    // Add sinks for 8-bit data input (D7 to D0)
    for (int i = 7; i >= 0; i--) {
        AddSink(String().Cat() << "D" << i);
    }
    // Add sinks for control signals
    AddSink("Ck");     // Clock
    AddSink("En");     // Enable
    AddSink("Clr");    // Clear
    // Add sources for 8-bit output (Q7 to Q0)
    for (int i = 7; i >= 0; i--) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();
    }
}

bool Register8Bit::Tick() {
    // Check for clear condition - if clear is active, reset all outputs regardless of clock
    if (clr) {
        for (int i = 0; i < 8; i++) {
            q[i] = 0;
        }
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        // On rising edge, and if enabled, update all output bits with input data
        if (rising_edge && en) {
            for (int i = 0; i < 8; i++) {
                q[i] = d[i];
            }
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool Register8Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle data input connections (D7 to D0: conn_id 0-7)
        if (conn_id >= 0 && conn_id < 8) {
            // Skip data inputs - handled by PutRaw
        }
        // Handle control signal connections
        else if (conn_id >= 8 && conn_id <= 10) {
            // Skip control signals - handled by PutRaw
        }
        // Handle output connections (Q7 to Q0: conn_id 11-18)
        else if (conn_id >= 11 && conn_id < 19) {
            int output_idx = conn_id - 11;
            if (output_idx < 8) {
                return dest.PutRaw(dest_conn_id, (byte*)&q[output_idx], 0, 1);
            }
        }
    }
    return true;
}

bool Register8Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data input connections (D7 to D0: conn_id 0-7)
    if (conn_id >= 0 && conn_id < 8) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        int input_idx = 7 - conn_id;  // D7 is conn_id 0, D6 is conn_id 1, etc.
        d[input_idx] = *data & 1;
    }
    // Handle control signals
    else if (conn_id == 8) {   // Ck (Clock)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clk = *data & 1;
    }
    else if (conn_id == 9) {   // En (Enable)
        ASSERT(data_bytes == 0 && data_bits == 1);
        en = *data & 1;
    }
    else if (conn_id == 10) {  // Clr (Clear)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clr = *data & 1;
    }
    else {
        LOG("error: Register8Bit: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 8-bit D flip-flop
FlipFlopD8Bit::FlipFlopD8Bit() {
    // Add sinks for 8-bit data input (D7 to D0)
    for (int i = 7; i >= 0; i--) {
        AddSink(String().Cat() << "D" << i);
    }
    // Add sinks for control signals
    AddSink("Ck");     // Clock
    AddSink("En");     // Enable
    AddSink("Clr");    // Clear
    // Add sources for 8-bit output and inverted output (Q7 to Q0, QN7 to QN0)
    for (int i = 7; i >= 0; i--) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();
    }
    for (int i = 7; i >= 0; i--) {
        AddSource(String().Cat() << "QN" << i).SetMultiConn();
    }
}

bool FlipFlopD8Bit::Tick() {
    // Check for clear condition - if clear is active, reset all outputs regardless of clock
    if (clr) {
        for (int i = 0; i < 8; i++) {
            q[i] = 0;
            qn[i] = 1;
        }
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        // On rising edge, and if enabled, update the outputs with the D input values
        if (rising_edge && en) {
            for (int i = 0; i < 8; i++) {
                q[i] = d[i];
                qn[i] = !d[i];  // Inverted output
            }
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool FlipFlopD8Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle data input connections (D7 to D0: conn_id 0-7)
        if (conn_id >= 0 && conn_id < 8) {
            // Skip - handled by PutRaw
        }
        // Handle control signals (Ck, En, Clr: conn_id 8-10)
        else if (conn_id >= 8 && conn_id <= 10) {
            // Skip - handled by PutRaw
        }
        // Handle positive output connections (Q7 to Q0: conn_id 11-18)
        else if (conn_id >= 11 && conn_id < 19) {
            int output_idx = conn_id - 11;
            if (output_idx < 8) {
                return dest.PutRaw(dest_conn_id, (byte*)&q[output_idx], 0, 1);
            }
        }
        // Handle inverted output connections (QN7 to QN0: conn_id 19-26)
        else if (conn_id >= 19 && conn_id < 27) {
            int output_idx = conn_id - 19;
            if (output_idx < 8) {
                return dest.PutRaw(dest_conn_id, (byte*)&qn[output_idx], 0, 1);
            }
        }
    }
    return true;
}

bool FlipFlopD8Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data input connections (D7 to D0: conn_id 0-7)
    if (conn_id >= 0 && conn_id < 8) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        int input_idx = 7 - conn_id;  // D7 is conn_id 0, D6 is conn_id 1, etc.
        d[input_idx] = *data & 1;
    }
    // Handle control signals
    else if (conn_id == 8) {   // Ck (Clock)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clk = *data & 1;
    }
    else if (conn_id == 9) {   // En (Enable)
        ASSERT(data_bytes == 0 && data_bits == 1);
        en = *data & 1;
    }
    else if (conn_id == 10) {  // Clr (Clear)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clr = *data & 1;
    }
    else {
        LOG("error: FlipFlopD8Bit: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 4-bit counter
Counter4Bit::Counter4Bit() {
    // Add sinks for 4-bit data input (D3 to D0)
    for (int i = 3; i >= 0; i--) {
        AddSink(String().Cat() << "D" << i);
    }
    // Add sinks for control signals
    AddSink("Ck");     // Clock
    AddSink("En");     // Enable
    AddSink("Clr");    // Clear
    AddSink("Load");   // Load
    // Add sources for 4-bit output (Q3 to Q0)
    for (int i = 3; i >= 0; i--) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();
    }
    // Add carry out
    AddSource("CO").SetMultiConn();
}

bool Counter4Bit::Tick() {
    // Check for clear condition
    if (clr) {
        count = 0;
        carry_out = 0;
        for (int i = 0; i < 4; i++) {
            q[i] = 0;
        }
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        if (rising_edge) {
            if (load && en) {
                // Load parallel data
                count = 0;
                for (int i = 0; i < 4; i++) {
                    if (d[i]) {
                        count |= (1 << i);
                    }
                }
            } else if (en) {
                // Increment counter
                count = (count + 1) & 0x0F;  // 4-bit wraparound
                carry_out = (count == 0) ? 1 : 0;  // Carry out when wrapping to 0
            }
            
            // Update output bits
            for (int i = 0; i < 4; i++) {
                q[i] = (count >> i) & 1;
            }
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool Counter4Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle data input connections (D3 to D0: conn_id 0-3)
        if (conn_id >= 0 && conn_id < 4) {
            // Skip data inputs - handled by PutRaw
        }
        // Handle control signal connections (Ck, En, Clr, Load: conn_id 4-7)
        else if (conn_id >= 4 && conn_id <= 7) {
            // Skip control signals - handled by PutRaw
        }
        // Handle output connections (Q3 to Q0: conn_id 8-11)
        else if (conn_id >= 8 && conn_id < 12) {
            int output_idx = conn_id - 8;
            if (output_idx < 4) {
                return dest.PutRaw(dest_conn_id, (byte*)&q[output_idx], 0, 1);
            }
        }
        // Handle carry out (conn_id 12)
        else if (conn_id == 12) {
            return dest.PutRaw(dest_conn_id, (byte*)&carry_out, 0, 1);
        }
    }
    return true;
}

bool Counter4Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data input connections (D3 to D0: conn_id 0-3)
    if (conn_id >= 0 && conn_id < 4) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        int input_idx = 3 - conn_id;  // D3 is conn_id 0, D2 is conn_id 1, etc.
        d[input_idx] = *data & 1;
    }
    // Handle control signals
    else if (conn_id == 4) {   // Ck (Clock)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clk = *data & 1;
    }
    else if (conn_id == 5) {   // En (Enable)
        ASSERT(data_bytes == 0 && data_bits == 1);
        en = *data & 1;
    }
    else if (conn_id == 6) {   // Clr (Clear)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clr = *data & 1;
    }
    else if (conn_id == 7) {   // Load
        ASSERT(data_bytes == 0 && data_bits == 1);
        load = *data & 1;
    }
    else {
        LOG("error: Counter4Bit: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 8-bit counter
Counter8Bit::Counter8Bit() {
    // Add sinks for 8-bit data input (D7 to D0)
    for (int i = 7; i >= 0; i--) {
        AddSink(String().Cat() << "D" << i);
    }
    // Add sinks for control signals
    AddSink("Ck");     // Clock
    AddSink("En");     // Enable
    AddSink("Clr");    // Clear
    AddSink("Load");   // Load
    // Add sources for 8-bit output (Q7 to Q0)
    for (int i = 7; i >= 0; i--) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();
    }
    // Add carry out
    AddSource("CO").SetMultiConn();
}

bool Counter8Bit::Tick() {
    // Check for clear condition
    if (clr) {
        count = 0;
        carry_out = 0;
        for (int i = 0; i < 8; i++) {
            q[i] = 0;
        }
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        if (rising_edge) {
            if (load && en) {
                // Load parallel data
                count = 0;
                for (int i = 0; i < 8; i++) {
                    if (d[i]) {
                        count |= (1 << i);
                    }
                }
            } else if (en) {
                // Increment counter
                count = (count + 1) & 0xFF;  // 8-bit wraparound
                carry_out = (count == 0) ? 1 : 0;  // Carry out when wrapping to 0
            }
            
            // Update output bits
            for (int i = 0; i < 8; i++) {
                q[i] = (count >> i) & 1;
            }
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool Counter8Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle data input connections (D7 to D0: conn_id 0-7)
        if (conn_id >= 0 && conn_id < 8) {
            // Skip data inputs - handled by PutRaw
        }
        // Handle control signal connections (Ck, En, Clr, Load: conn_id 8-11)
        else if (conn_id >= 8 && conn_id <= 11) {
            // Skip control signals - handled by PutRaw
        }
        // Handle output connections (Q7 to Q0: conn_id 12-19)
        else if (conn_id >= 12 && conn_id < 20) {
            int output_idx = conn_id - 12;
            if (output_idx < 8) {
                return dest.PutRaw(dest_conn_id, (byte*)&q[output_idx], 0, 1);
            }
        }
        // Handle carry out (conn_id 20)
        else if (conn_id == 20) {
            return dest.PutRaw(dest_conn_id, (byte*)&carry_out, 0, 1);
        }
    }
    return true;
}

bool Counter8Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle data input connections (D7 to D0: conn_id 0-7)
    if (conn_id >= 0 && conn_id < 8) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        int input_idx = 7 - conn_id;  // D7 is conn_id 0, D6 is conn_id 1, etc.
        d[input_idx] = *data & 1;
    }
    // Handle control signals
    else if (conn_id == 8) {   // Ck (Clock)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clk = *data & 1;
    }
    else if (conn_id == 9) {   // En (Enable)
        ASSERT(data_bytes == 0 && data_bits == 1);
        en = *data & 1;
    }
    else if (conn_id == 10) {  // Clr (Clear)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clr = *data & 1;
    }
    else if (conn_id == 11) {  // Load
        ASSERT(data_bytes == 0 && data_bits == 1);
        load = *data & 1;
    }
    else {
        LOG("error: Counter8Bit: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 16x8 RAM
RAM16x8::RAM16x8() {
    // Add sinks for 4-bit address (A3 to A0)
    for (int i = 3; i >= 0; i--) {
        AddSink(String().Cat() << "A" << i);
    }
    // Add sinks for 8-bit data input (D7 to D0)
    for (int i = 7; i >= 0; i--) {
        AddSink(String().Cat() << "D" << i);
    }
    // Add sinks for control signals
    AddSink("WE");     // Write Enable
    AddSink("OE");     // Output Enable
    AddSink("CS");     // Chip Select
    // Add sources for 8-bit data output (Q7 to Q0)
    for (int i = 7; i >= 0; i--) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();
    }
}

bool RAM16x8::Tick() {
    // Get address value
    int addr_val = 0;
    for (int i = 0; i < 4; i++) {
        if (addr[i]) {
            addr_val |= (1 << i);
        }
    }
    
    // Only perform operations if chip is selected
    if (cs) {
        // Write operation (active high)
        if (we) {
            // Write data to memory location
            byte data_val = 0;
            for (int i = 0; i < 8; i++) {
                if (din[i]) {
                    data_val |= (1 << i);
                }
            }
            memory[addr_val] = data_val;
        }
        
        // Read operation (output enable active high)
        if (oe) {
            byte data_val = memory[addr_val];
            for (int i = 0; i < 8; i++) {
                dout[i] = (data_val >> i) & 1;
            }
        } else {
            // If output is disabled, set outputs to high-impedance equivalent
            for (int i = 0; i < 8; i++) {
                dout[i] = 0; // For simulation purposes, set to 0 when disabled
            }
        }
    } else {
        // If chip is not selected, set outputs to high-impedance equivalent
        for (int i = 0; i < 8; i++) {
            dout[i] = 0; // For simulation purposes, set to 0 when disabled
        }
    }
    
    return true;
}

bool RAM16x8::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle address input connections (A3 to A0: conn_id 0-3)
        if (conn_id >= 0 && conn_id < 4) {
            // Skip - handled by PutRaw
        }
        // Handle data input connections (D7 to D0: conn_id 4-11)
        else if (conn_id >= 4 && conn_id < 12) {
            // Skip - handled by PutRaw
        }
        // Handle control signals (WE, OE, CS: conn_id 12-14)
        else if (conn_id >= 12 && conn_id <= 14) {
            // Skip - handled by PutRaw
        }
        // Handle data output connections (Q7 to Q0: conn_id 15-22)
        else if (conn_id >= 15 && conn_id < 23) {
            int output_idx = conn_id - 15;
            if (output_idx < 8) {
                return dest.PutRaw(dest_conn_id, (byte*)&dout[output_idx], 0, 1);
            }
        }
    }
    return true;
}

bool RAM16x8::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle address input connections (A3 to A0: conn_id 0-3)
    if (conn_id >= 0 && conn_id < 4) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        int addr_idx = conn_id;  // A0 is conn_id 0, A1 is conn_id 1, etc.
        addr[addr_idx] = *data & 1;
    }
    // Handle data input connections (D7 to D0: conn_id 4-11)
    else if (conn_id >= 4 && conn_id < 12) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        int data_idx = 11 - conn_id;  // D7 is conn_id 4, D6 is conn_id 5, etc.
        din[data_idx] = *data & 1;
    }
    // Handle control signals
    else if (conn_id == 12) {  // WE (Write Enable)
        ASSERT(data_bytes == 0 && data_bits == 1);
        we = *data & 1;
    }
    else if (conn_id == 13) {  // OE (Output Enable)
        ASSERT(data_bytes == 0 && data_bits == 1);
        oe = *data & 1;
    }
    else if (conn_id == 14) {  // CS (Chip Select)
        ASSERT(data_bytes == 0 && data_bits == 1);
        cs = *data & 1;
    }
    else {
        LOG("error: RAM16x8: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 16x8 ROM
ROM16x8::ROM16x8(const byte init_data[16]) {
    // Initialize memory with provided data or zeros
    if (init_data) {
        for (int i = 0; i < 16; i++) {
            memory[i] = init_data[i];
        }
    } else {
        for (int i = 0; i < 16; i++) {
            memory[i] = 0x00;  // Initialize to zeros
        }
    }

    // Add sinks for 4-bit address (A3 to A0)
    for (int i = 3; i >= 0; i--) {
        AddSink(String().Cat() << "A" << i);
    }
    // Add sinks for control signals
    AddSink("OE");     // Output Enable
    AddSink("CS");     // Chip Select
    // Add sources for 8-bit data output (Q7 to Q0)
    for (int i = 7; i >= 0; i--) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();
    }
}

bool ROM16x8::Tick() {
    // Get address value
    int addr_val = 0;
    for (int i = 0; i < 4; i++) {
        if (addr[i]) {
            addr_val |= (1 << i);
        }
    }
    
    // Only perform read if chip is selected and output is enabled
    if (cs && oe) {
        byte data_val = memory[addr_val];
        for (int i = 0; i < 8; i++) {
            dout[i] = (data_val >> i) & 1;
        }
    } else {
        // If chip is not selected or output is disabled, set outputs to high-impedance equivalent
        for (int i = 0; i < 8; i++) {
            dout[i] = 0; // For simulation purposes, set to 0 when disabled
        }
    }
    
    return true;
}

bool ROM16x8::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle address input connections (A3 to A0: conn_id 0-3)
        if (conn_id >= 0 && conn_id < 4) {
            // Skip - handled by PutRaw
        }
        // Handle control signals (OE, CS: conn_id 4-5)
        else if (conn_id >= 4 && conn_id <= 5) {
            // Skip - handled by PutRaw
        }
        // Handle data output connections (Q7 to Q0: conn_id 6-13)
        else if (conn_id >= 6 && conn_id < 14) {
            int output_idx = conn_id - 6;
            if (output_idx < 8) {
                return dest.PutRaw(dest_conn_id, (byte*)&dout[output_idx], 0, 1);
            }
        }
    }
    return true;
}

bool ROM16x8::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle address input connections (A3 to A0: conn_id 0-3)
    if (conn_id >= 0 && conn_id < 4) {
        ASSERT(data_bytes == 0 && data_bits == 1);
        int addr_idx = conn_id;  // A0 is conn_id 0, A1 is conn_id 1, etc.
        addr[addr_idx] = *data & 1;
    }
    // Handle control signals
    else if (conn_id == 4) {   // OE (Output Enable)
        ASSERT(data_bytes == 0 && data_bits == 1);
        oe = *data & 1;
    }
    else if (conn_id == 5) {   // CS (Chip Select)
        ASSERT(data_bytes == 0 && data_bits == 1);
        cs = *data & 1;
    }
    else {
        LOG("error: ROM16x8: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}

// Implementation of 8-bit shift register
ShiftRegister8Bit::ShiftRegister8Bit() {
    // Add sinks for control signals
    AddSink("DS");     // Data input (serial)
    AddSink("Ck");     // Clock
    AddSink("Clr");    // Clear
    AddSink("M");      // Mode: 0=shift, 1=load
    AddSink("SL");     // Serial/Parallel load control
    // Add sinks for 8-bit parallel data input (D7 to D0)
    for (int i = 7; i >= 0; i--) {
        AddSink(String().Cat() << "D" << i);
    }
    // Add sources for 8-bit parallel output (Q7 to Q0)
    for (int i = 7; i >= 0; i--) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();
    }
    // Add source for serial output
    AddSource("Q7S").SetMultiConn();
}

bool ShiftRegister8Bit::Tick() {
    // Check for clear condition
    if (clr) {
        for (int i = 0; i < 8; i++) {
            q[i] = 0;
        }
        q7_serial = 0;
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        if (rising_edge) {
            if (mode) {
                // Parallel load mode
                for (int i = 0; i < 8; i++) {
                    q[i] = d[i];
                }
            } else {
                // Shift mode
                if (ser_load) {
                    // Serial load: shift in serial data from DS
                    // Shift all bits except the first one to the right
                    for (int i = 7; i > 0; i--) {
                        q[i] = q[i-1];
                    }
                    // Load the serial input into position 0
                    q[0] = data_in;
                } else {
                    // Shift right: shift all bits to the right
                    for (int i = 7; i > 0; i--) {
                        q[i] = q[i-1];
                    }
                    // The bit that 'falls off' the end becomes the serial output
                    q7_serial = q[0];
                    // The new position 0 is typically set to 0 when shifting
                    q[0] = 0;
                }
            }
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool ShiftRegister8Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle simple input connections (DS, Ck, Clr, M, SL: conn_id 0-4)
        if (conn_id >= 0 && conn_id <= 4) {
            // Skip control inputs - handled by PutRaw
        }
        // Handle parallel data input connections (D7 to D0: conn_id 5-12)
        else if (conn_id >= 5 && conn_id < 13) {
            // Skip - handled by PutRaw
        }
        // Handle parallel output connections (Q7 to Q0: conn_id 13-20)
        else if (conn_id >= 13 && conn_id < 21) {
            int output_idx = conn_id - 13;
            if (output_idx < 8) {
                return dest.PutRaw(dest_conn_id, (byte*)&q[output_idx], 0, 1);
            }
        }
        // Handle serial output (Q7S: conn_id 21)
        else if (conn_id == 21) {
            return dest.PutRaw(dest_conn_id, (byte*)&q7_serial, 0, 1);
        }
    }
    return true;
}

bool ShiftRegister8Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    if (conn_id == 0) {      // DS (Data input serial)
        ASSERT(data_bytes == 0 && data_bits == 1);
        data_in = *data & 1;
    } else if (conn_id == 1) { // Ck (Clock)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clk = *data & 1;
    } else if (conn_id == 2) { // Clr (Clear)
        ASSERT(data_bytes == 0 && data_bits == 1);
        clr = *data & 1;
    } else if (conn_id == 3) { // M (Mode)
        ASSERT(data_bytes == 0 && data_bits == 1);
        mode = *data & 1;
    } else if (conn_id == 4) { // SL (Serial/Parallel load)
        ASSERT(data_bytes == 0 && data_bits == 1);
        ser_load = *data & 1;
    } else if (conn_id >= 5 && conn_id < 13) { // D7 to D0 (Parallel data input)
        ASSERT(data_bytes == 0 && data_bits == 1);
        int data_idx = 12 - conn_id;  // D7 is conn_id 5, D6 is conn_id 6, etc.
        d[data_idx] = *data & 1;
    } else {
        LOG("error: ShiftRegister8Bit: unimplemented conn-id " << conn_id);
        return false;
    }
    return true;
}