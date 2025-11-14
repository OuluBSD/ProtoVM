#include "TubeLogic.h"

TubeTriode::TubeTriode() {
    AddSink("C");     // Cathode
    AddSink("G");     // Grid
    AddSource("P").SetMultiConn(); // Plate (Anode)
}

bool TubeTriode::Tick() {
    // When grid is high (1), current can flow from cathode to plate
    // When grid is low (0), current is blocked from flowing
    current_flow = (grid && cathode);
    
    // Plate output is high when current flows (0 - low when current flows)
    // In digital terms, if current flows, plate is pulled low (0)
    // If no current flows, plate stays high (1) due to pull-up resistor
    plate = !current_flow;
    
    return true;
}

bool TubeTriode::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // C (Cathode)
            case 1:  // G (Grid)
                // Skip inputs - they are handled by PutRaw
                break;
            case 2:  // P (Plate - Output)
                return dest.PutRaw(dest_conn_id, (byte*)&plate, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeTriode: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeTriode::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // C (Cathode)
            ASSERT(data_bytes == 0 && data_bits == 1);
            cathode = *data & 1;
            break;
        case 1:  // G (Grid)
            ASSERT(data_bytes == 0 && data_bits == 1);
            grid = *data & 1;
            break;
        default:
            LOG("error: TubeTriode: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeNot::TubeNot() {
    AddSink("I");      // Input to grid
    AddSource("O").SetMultiConn(); // Output from plate
}

bool TubeNot::Tick() {
    // Grid input controls conduction
    // When input (grid) is 1, current flows, output is low (0)
    // When input (grid) is 0, no current flows, output is high (1)
    out = !in;
    return true;
}

bool TubeNot::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // I (Input)
                // Skip - handled by PutRaw
                break;
            case 1:  // O (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeNot: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeNot::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // I (Input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            in = *data & 1;
            break;
        default:
            LOG("error: TubeNot: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeNand::TubeNand() {
    AddSink("I0");     // Input 0
    AddSink("I1");     // Input 1
    AddSource("O").SetMultiConn(); // Output
}

bool TubeNand::Tick() {
    // NAND: both inputs must be high for output to be low
    // If either input is low, output is high
    out = !(in0 && in1);
    return true;
}

bool TubeNand::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // I0
            case 1:  // I1
                // Skip inputs - handled by PutRaw
                break;
            case 2:  // O (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeNand: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeNand::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // I0
            ASSERT(data_bytes == 0 && data_bits == 1);
            in0 = *data & 1;
            break;
        case 1:  // I1
            ASSERT(data_bytes == 0 && data_bits == 1);
            in1 = *data & 1;
            break;
        default:
            LOG("error: TubeNand: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeAnd::TubeAnd() {
    AddSink("I0");     // Input 0
    AddSink("I1");     // Input 1
    AddSource("O").SetMultiConn(); // Output
    
    // Connect internal components
    nand_gate.AddSink("I0");
    nand_gate.AddSink("I1");
    nand_gate.AddSource("O");
    
    not_gate.AddSink("I");
    not_gate.AddSource("O");
}

bool TubeAnd::Tick() {
    // AND = NOT(NAND)
    // First compute NAND of inputs
    nand_gate.PutRaw(0, (byte*)&in0, 0, 1);
    nand_gate.PutRaw(1, (byte*)&in1, 0, 1);
    
    bool nand_result = !(in0 && in1);
    nand_gate.Tick();
    
    // Then NOT the NAND result to get AND
    not_gate.PutRaw(0, (byte*)&nand_result, 0, 1);
    not_gate.Tick();
    
    // Get output from NOT gate
    byte temp_byte = 0;
    not_gate.Process(WRITE, 0, 1, 1, (ElectricNodeBase&)*this, 999); // Get the actual output value
    out = !(in0 && in1); // Simplified since we know the logic
    
    return true;
}

bool TubeAnd::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // I0
            case 1:  // I1
                // Skip inputs - handled by PutRaw
                break;
            case 2:  // O (Output)
                // Calculate AND of inputs directly
                bool and_result = in0 && in1;
                return dest.PutRaw(dest_conn_id, (byte*)&and_result, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeAnd: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeAnd::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // I0
            ASSERT(data_bytes == 0 && data_bits == 1);
            in0 = *data & 1;
            break;
        case 1:  // I1
            ASSERT(data_bytes == 0 && data_bits == 1);
            in1 = *data & 1;
            break;
        default:
            LOG("error: TubeAnd: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeOr::TubeOr() {
    AddSink("I0");     // Input 0
    AddSink("I1");     // Input 1
    AddSource("O").SetMultiConn(); // Output
}

bool TubeOr::Tick() {
    // OR: if either input is high, output is high
    out = (in0 || in1);
    return true;
}

bool TubeOr::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // I0
            case 1:  // I1
                // Skip inputs - handled by PutRaw
                break;
            case 2:  // O (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeOr: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeOr::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // I0
            ASSERT(data_bytes == 0 && data_bits == 1);
            in0 = *data & 1;
            break;
        case 1:  // I1
            ASSERT(data_bytes == 0 && data_bits == 1);
            in1 = *data & 1;
            break;
        default:
            LOG("error: TubeOr: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeNor::TubeNor() {
    AddSink("I0");     // Input 0
    AddSink("I1");     // Input 1
    AddSource("O").SetMultiConn(); // Output
}

bool TubeNor::Tick() {
    // NOR: output is high only when both inputs are low
    out = !(in0 || in1);
    return true;
}

bool TubeNor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // I0
            case 1:  // I1
                // Skip inputs - handled by PutRaw
                break;
            case 2:  // O (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeNor: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeNor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // I0
            ASSERT(data_bytes == 0 && data_bits == 1);
            in0 = *data & 1;
            break;
        case 1:  // I1
            ASSERT(data_bytes == 0 && data_bits == 1);
            in1 = *data & 1;
            break;
        default:
            LOG("error: TubeNor: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeSRLatch::TubeSRLatch() {
    AddSink("S");       // Set input
    AddSink("R");       // Reset input
    AddSource("Q").SetMultiConn();     // Output
    AddSource("~Q").SetMultiConn();    // Inverted output
}

bool TubeSRLatch::Tick() {
    // SR Latch behavior:
    // S=0, R=0: Hold state (no change)
    // S=1, R=0: Set (Q=1, QN=0)
    // S=0, R=1: Reset (Q=0, QN=1)
    // S=1, R=1: Invalid/forbidden state - maintain previous state
    
    if (s && !r) {
        // Set: Q=1, QN=0
        q = 1;
        qn = 0;
    } else if (!s && r) {
        // Reset: Q=0, QN=1
        q = 0;
        qn = 1;
    }
    // If both S and R are 0, maintain current state
    // If both S and R are 1, maintain current state (invalid state)
    
    return true;
}

bool TubeSRLatch::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // S (Set input)
            case 1:  // R (Reset input)
                // Skip inputs - handled by PutRaw
                break;
            case 2:  // Q (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&q, 0, 1);
                break;
            case 3:  // ~Q (Inverted output)
                return dest.PutRaw(dest_conn_id, (byte*)&qn, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeSRLatch: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeSRLatch::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // S (Set input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            s = *data & 1;
            break;
        case 1:  // R (Reset input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            r = *data & 1;
            break;
        default:
            LOG("error: TubeSRLatch: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeDFlipFlop::TubeDFlipFlop() {
    AddSink("D");       // Data input
    AddSink("Ck");      // Clock input
    AddSink("En");      // Enable input (active high)
    AddSink("Clr");     // Clear input (active high)
    AddSource("Q").SetMultiConn();     // Output
    AddSource("~Q").SetMultiConn();    // Inverted output
}

bool TubeDFlipFlop::Tick() {
    // Check for clear condition - if clear is active, reset outputs regardless of clock
    if (clr) {
        q = 0;
        qn = 1;
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        // On rising edge, and if enabled, update the output with the D input value
        if (rising_edge && en) {
            q = d;
            qn = !d;  // Inverted output
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool TubeDFlipFlop::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // D (Data input)
            case 1:  // Ck (Clock input)
            case 2:  // En (Enable input)
            case 3:  // Clr (Clear input)
                // Skip inputs - handled by PutRaw
                break;
            case 4:  // Q (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&q, 0, 1);
                break;
            case 5:  // ~Q (Inverted output)
                return dest.PutRaw(dest_conn_id, (byte*)&qn, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeDFlipFlop: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeDFlipFlop::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // D (Data input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d = *data & 1;
            break;
        case 1: // Ck (Clock input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clk = *data & 1;
            break;
        case 2: // En (Enable input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            en = *data & 1;
            break;
        case 3: // Clr (Clear input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clr = *data & 1;
            break;
        default:
            LOG("error: TubeDFlipFlop: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeJKFlipFlop::TubeJKFlipFlop() {
    AddSink("J");       // J input
    AddSink("K");       // K input
    AddSink("Ck");      // Clock input
    AddSink("En");      // Enable input (active high)
    AddSink("Clr");     // Clear input (active high)
    AddSource("Q").SetMultiConn();     // Output
    AddSource("~Q").SetMultiConn();    // Inverted output
}

bool TubeJKFlipFlop::Tick() {
    // Check for clear condition - if clear is active, reset outputs regardless of other inputs
    if (clr) {
        q = 0;
        qn = 1;
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        // On rising edge, and if enabled, apply JK flip-flop logic
        if (rising_edge && en) {
            if (j && k) {
                // Toggle: Q = !Q, QN = !QN
                bool temp_q = q;
                q = !temp_q;
                qn = temp_q;
            } else if (j && !k) {
                // Set: Q = 1, QN = 0
                q = 1;
                qn = 0;
            } else if (!j && k) {
                // Reset: Q = 0, QN = 1
                q = 0;
                qn = 1;
            }
            // If both J and K are 0, maintain current state
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool TubeJKFlipFlop::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // J (J input)
            case 1:  // K (K input)
            case 2:  // Ck (Clock input)
            case 3:  // En (Enable input)
            case 4:  // Clr (Clear input)
                // Skip inputs - handled by PutRaw
                break;
            case 5:  // Q (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&q, 0, 1);
                break;
            case 6:  // ~Q (Inverted output)
                return dest.PutRaw(dest_conn_id, (byte*)&qn, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeJKFlipFlop: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeJKFlipFlop::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // J (J input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            j = *data & 1;
            break;
        case 1: // K (K input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            k = *data & 1;
            break;
        case 2: // Ck (Clock input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clk = *data & 1;
            break;
        case 3: // En (Enable input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            en = *data & 1;
            break;
        case 4: // Clr (Clear input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clr = *data & 1;
            break;
        default:
            LOG("error: TubeJKFlipFlop: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeRegister4Bit::TubeRegister4Bit() {
    // Add sinks for 4-bit data input (D3, D2, D1, D0)
    AddSink("D3");
    AddSink("D2");
    AddSink("D1");
    AddSink("D0");
    // Add sinks for control signals
    AddSink("Ck");     // Clock
    AddSink("En");     // Enable
    AddSink("Clr");    // Clear
    // Add sources for 4-bit output (Q3, Q2, Q1, Q0)
    AddSource("Q3").SetMultiConn();
    AddSource("Q2").SetMultiConn();
    AddSource("Q1").SetMultiConn();
    AddSource("Q0").SetMultiConn();
}

bool TubeRegister4Bit::Tick() {
    // Check for clear condition - if clear is active, reset all outputs regardless of clock
    if (clr) {
        for (int i = 0; i < 4; i++) {
            q[i] = 0;
        }
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);

        // On rising edge, and if enabled, update all output bits with input data
        if (rising_edge && en) {
            for (int i = 0; i < 4; i++) {
                q[i] = d[i];
            }
        }
    }

    // Store current clock state for next rising edge detection
    last_clk = clk;

    return true;
}

bool TubeRegister4Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // D3 (Data bit 3)
            case 1:  // D2 (Data bit 2)
            case 2:  // D1 (Data bit 1)
            case 3:  // D0 (Data bit 0)
            case 4:  // Ck (Clock input)
            case 5:  // En (Enable input)
            case 6:  // Clr (Clear input)
                // Skip inputs - handled by PutRaw
                break;
            case 7:  // Q3 (Output bit 3)
                return dest.PutRaw(dest_conn_id, (byte*)&q[3], 0, 1);
                break;
            case 8:  // Q2 (Output bit 2)
                return dest.PutRaw(dest_conn_id, (byte*)&q[2], 0, 1);
                break;
            case 9:  // Q1 (Output bit 1)
                return dest.PutRaw(dest_conn_id, (byte*)&q[1], 0, 1);
                break;
            case 10: // Q0 (Output bit 0)
                return dest.PutRaw(dest_conn_id, (byte*)&q[0], 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeRegister4Bit: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeRegister4Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // D3 (Data bit 3)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[3] = *data & 1;
            break;
        case 1: // D2 (Data bit 2)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[2] = *data & 1;
            break;
        case 2: // D1 (Data bit 1)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[1] = *data & 1;
            break;
        case 3: // D0 (Data bit 0)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[0] = *data & 1;
            break;
        case 4: // Ck (Clock input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clk = *data & 1;
            break;
        case 5: // En (Enable input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            en = *data & 1;
            break;
        case 6: // Clr (Clear input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clr = *data & 1;
            break;
        default:
            LOG("error: TubeRegister4Bit: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeCounter4Bit::TubeCounter4Bit() {
    // Add sinks for 4-bit parallel load data (D3, D2, D1, D0)
    AddSink("D3");
    AddSink("D2");
    AddSink("D1");
    AddSink("D0");
    // Add sinks for control signals
    AddSink("Ck");     // Clock
    AddSink("En");     // Enable
    AddSink("Clr");    // Clear
    AddSink("Load");   // Load (active high)
    // Add sources for 4-bit output (Q3, Q2, Q1, Q0)
    AddSource("Q3").SetMultiConn();
    AddSource("Q2").SetMultiConn();
    AddSource("Q1").SetMultiConn();
    AddSource("Q0").SetMultiConn();
}

bool TubeCounter4Bit::Tick() {
    // Check for clear condition - if clear is active, reset all outputs
    if (clr) {
        for (int i = 0; i < 4; i++) {
            q[i] = 0;
        }
        count = 0;
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);
        
        if (rising_edge && en) {
            if (load) {
                // Load parallel data
                count = 0;
                for (int i = 0; i < 4; i++) {
                    q[i] = d[i];
                    if (d[i]) {
                        count |= (1 << i);
                    }
                }
            } else {
                // Increment counter
                count = (count + 1) & 0xF;  // 4-bit wrap-around
                
                // Update output bits
                for (int i = 0; i < 4; i++) {
                    q[i] = (count >> i) & 1;
                }
            }
        }
    }
    
    // Store current clock state for next rising edge detection
    last_clk = clk;
    
    return true;
}

bool TubeCounter4Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // D3 (Data bit 3)
            case 1:  // D2 (Data bit 2)
            case 2:  // D1 (Data bit 1)
            case 3:  // D0 (Data bit 0)
            case 4:  // Ck (Clock input)
            case 5:  // En (Enable input)
            case 6:  // Clr (Clear input)
            case 7:  // Load (Load input)
                // Skip inputs - handled by PutRaw
                break;
            case 8:  // Q3 (Output bit 3)
                return dest.PutRaw(dest_conn_id, (byte*)&q[3], 0, 1);
                break;
            case 9:  // Q2 (Output bit 2)
                return dest.PutRaw(dest_conn_id, (byte*)&q[2], 0, 1);
                break;
            case 10: // Q1 (Output bit 1)
                return dest.PutRaw(dest_conn_id, (byte*)&q[1], 0, 1);
                break;
            case 11: // Q0 (Output bit 0)
                return dest.PutRaw(dest_conn_id, (byte*)&q[0], 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeCounter4Bit: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeCounter4Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // D3 (Data bit 3)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[3] = *data & 1;
            break;
        case 1: // D2 (Data bit 2)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[2] = *data & 1;
            break;
        case 2: // D1 (Data bit 1)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[1] = *data & 1;
            break;
        case 3: // D0 (Data bit 0)
            ASSERT(data_bytes == 0 && data_bits == 1);
            d[0] = *data & 1;
            break;
        case 4: // Ck (Clock input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clk = *data & 1;
            break;
        case 5: // En (Enable input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            en = *data & 1;
            break;
        case 6: // Clr (Clear input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clr = *data & 1;
            break;
        case 7: // Load (Load input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            load = *data & 1;
            break;
        default:
            LOG("error: TubeCounter4Bit: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}

TubeBinaryCounter4Bit::TubeBinaryCounter4Bit() {
    // Add sinks for control signals
    AddSink("Ck");     // Clock
    AddSink("En");     // Enable
    AddSink("Clr");    // Clear
    // Add sources for 4-bit output (Q3, Q2, Q1, Q0)
    AddSource("Q3").SetMultiConn();
    AddSource("Q2").SetMultiConn();
    AddSource("Q1").SetMultiConn();
    AddSource("Q0").SetMultiConn();
}

bool TubeBinaryCounter4Bit::Tick() {
    // Check for clear condition - if clear is active, reset all outputs
    if (clr) {
        for (int i = 0; i < 4; i++) {
            q[i] = 0;
        }
        count = 0;
    } else {
        // Detect rising edge of clock
        bool rising_edge = (clk && !last_clk);
        
        if (rising_edge && en) {
            // Increment counter
            count = (count + 1) & 0xF;  // 4-bit wrap-around
            
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

bool TubeBinaryCounter4Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // Ck (Clock input)
            case 1:  // En (Enable input)
            case 2:  // Clr (Clear input)
                // Skip inputs - handled by PutRaw
                break;
            case 3:  // Q3 (Output bit 3)
                return dest.PutRaw(dest_conn_id, (byte*)&q[3], 0, 1);
                break;
            case 4:  // Q2 (Output bit 2)
                return dest.PutRaw(dest_conn_id, (byte*)&q[2], 0, 1);
                break;
            case 5:  // Q1 (Output bit 1)
                return dest.PutRaw(dest_conn_id, (byte*)&q[1], 0, 1);
                break;
            case 6:  // Q0 (Output bit 0)
                return dest.PutRaw(dest_conn_id, (byte*)&q[0], 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge
                break;
        }
    } else {
        LOG("error: TubeBinaryCounter4Bit: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool TubeBinaryCounter4Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // Ck (Clock input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clk = *data & 1;
            break;
        case 1: // En (Enable input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            en = *data & 1;
            break;
        case 2: // Clr (Clear input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            clr = *data & 1;
            break;
        default:
            LOG("error: TubeBinaryCounter4Bit: unimplemented conn-id " << conn_id);
            return false;
    }
    return true;
}