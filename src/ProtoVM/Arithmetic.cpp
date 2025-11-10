#include "ProtoVM.h"
#include "Arithmetic.h"

FullAdder::FullAdder() {
    AddSink("A");       // First input bit
    AddSink("B");       // Second input bit
    AddSink("Cin");     // Carry input
    AddSource("Sum").SetMultiConn();    // Sum output
    AddSource("Cout").SetMultiConn();   // Carry output
}

bool FullAdder::Tick() {
    // Calculate sum: A XOR B XOR Cin
    sum = a ^ b ^ carry_in;
    
    // Calculate carry out: (A AND B) OR (Cin AND (A OR B))
    carry_out = (a && b) || (carry_in && (a || b));
    
    return true;
}

bool FullAdder::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            case 0:  // A (Input)
            case 1:  // B (Input)
            case 2:  // Cin (Carry input)
                // Skip inputs - they are handled by PutRaw
                break;
            case 3:  // Sum (Output)
                return dest.PutRaw(dest_conn_id, (byte*)&sum, 0, 1);
                break;
            case 4:  // Cout (Carry output)
                return dest.PutRaw(dest_conn_id, (byte*)&carry_out, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge (for dummy pins or similar)
                break;
        }
    }
    else {
        LOG("error: FullAdder: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool FullAdder::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // A (Input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            a = *data & 1;
            break;
        case 1: // B (Input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            b = *data & 1;
            break;
        case 2: // Cin (Carry input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            carry_in = *data & 1;
            break;
        default:
            LOG("error: FullAdder: unimplemented conn-id " + AsString((int)conn_id));
            return false;
    }
    return true;
}

Adder4Bit::Adder4Bit() {
    // Add sinks for 4-bit inputs A and B
    AddSink("A3");
    AddSink("A2");
    AddSink("A1");
    AddSink("A0");
    AddSink("B3");
    AddSink("B2");
    AddSink("B1");
    AddSink("B0");
    AddSink("Cin");     // Carry input
    
    // Add sources for 4-bit sum output and carry output
    AddSource("S3").SetMultiConn();
    AddSource("S2").SetMultiConn();
    AddSource("S1").SetMultiConn();
    AddSource("S0").SetMultiConn();
    AddSource("Cout").SetMultiConn();
}

bool Adder4Bit::Tick() {
    // Calculate using full adders in a ripple carry fashion
    // FA0: A0, B0, and Cin -> S0, carry1
    bool carry1 = (a[0] && b[0]) || (carry_in && (a[0] || b[0]));
    sum[0] = a[0] ^ b[0] ^ carry_in;
    
    // FA1: A1, B1, and carry1 -> S1, carry2
    bool carry2 = (a[1] && b[1]) || (carry1 && (a[1] || b[1]));
    sum[1] = a[1] ^ b[1] ^ carry1;
    
    // FA2: A2, B2, and carry2 -> S2, carry3
    bool carry3 = (a[2] && b[2]) || (carry2 && (a[2] || b[2]));
    sum[2] = a[2] ^ b[2] ^ carry2;
    
    // FA3: A3, B3, and carry3 -> S3, Cout
    carry_out = (a[3] && b[3]) || (carry3 && (a[3] || b[3]));
    sum[3] = a[3] ^ b[3] ^ carry3;
    
    return true;
}

bool Adder4Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            // Input connections
            case 0:  // A3
            case 1:  // A2
            case 2:  // A1
            case 3:  // A0
            case 4:  // B3
            case 5:  // B2
            case 6:  // B1
            case 7:  // B0
            case 8:  // Cin (Carry input)
                // Skip inputs - they are handled by PutRaw
                break;
            // Output connections
            case 9:  // S3 (Sum bit 3)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[3], 0, 1);
                break;
            case 10: // S2 (Sum bit 2)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[2], 0, 1);
                break;
            case 11: // S1 (Sum bit 1)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[1], 0, 1);
                break;
            case 12: // S0 (Sum bit 0)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[0], 0, 1);
                break;
            case 13: // Cout (Carry output)
                return dest.PutRaw(dest_conn_id, (byte*)&carry_out, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge (for dummy pins or similar)
                break;
        }
    }
    else {
        LOG("error: Adder4Bit: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool Adder4Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // A3
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[3] = *data & 1;
            break;
        case 1: // A2
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[2] = *data & 1;
            break;
        case 2: // A1
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[1] = *data & 1;
            break;
        case 3: // A0
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[0] = *data & 1;
            break;
        case 4: // B3
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[3] = *data & 1;
            break;
        case 5: // B2
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[2] = *data & 1;
            break;
        case 6: // B1
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[1] = *data & 1;
            break;
        case 7: // B0
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[0] = *data & 1;
            break;
        case 8: // Cin (Carry input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            carry_in = *data & 1;
            break;
        default:
            LOG("error: Adder4Bit: unimplemented conn-id " + AsString((int)conn_id));
            return false;
    }
    return true;
}

AdderSubtractor4Bit::AdderSubtractor4Bit() {
    // Add sinks for 4-bit inputs A and B
    AddSink("A3");
    AddSink("A2");
    AddSink("A1");
    AddSink("A0");
    AddSink("B3");
    AddSink("B2");
    AddSink("B1");
    AddSink("B0");
    AddSink("Sub");     // Subtraction control (0=add, 1=subtract)
    AddSink("Cin");     // Carry input (for chaining adders)
    
    // Add sources for 4-bit sum/output and carry output
    AddSource("S3").SetMultiConn();
    AddSource("S2").SetMultiConn();
    AddSource("S1").SetMultiConn();
    AddSource("S0").SetMultiConn();
    AddSource("Cout").SetMultiConn();
}

bool AdderSubtractor4Bit::Tick() {
    // XOR B inputs with the sub control signal to perform subtraction
    // When sub=0: b_xor[i] = b[i] (for addition)
    // When sub=1: b_xor[i] = !b[i] (for subtraction - two's complement)
    for (int i = 0; i < 4; i++) {
        b_xor[i] = b[i] ^ sub;
    }
    
    // For subtraction, we need to add 1 to the inverted B (two's complement)
    // This is done by setting carry_in to 'sub' (1 for subtraction, 0 for addition)
    bool effective_carry_in = sub ? 1 : carry_in;
    
    // Calculate using full adders in a ripple carry fashion
    // FA0: A0, b_xor[0], and effective_carry_in -> S0, carry1
    bool carry1 = (a[0] && b_xor[0]) || (effective_carry_in && (a[0] || b_xor[0]));
    sum[0] = a[0] ^ b_xor[0] ^ effective_carry_in;
    
    // FA1: A1, b_xor[1], and carry1 -> S1, carry2
    bool carry2 = (a[1] && b_xor[1]) || (carry1 && (a[1] || b_xor[1]));
    sum[1] = a[1] ^ b_xor[1] ^ carry1;
    
    // FA2: A2, b_xor[2], and carry2 -> S2, carry3
    bool carry3 = (a[2] && b_xor[2]) || (carry2 && (a[2] || b_xor[2]));
    sum[2] = a[2] ^ b_xor[2] ^ carry2;
    
    // FA3: A3, b_xor[3], and carry3 -> S3, Cout
    carry_out = (a[3] && b_xor[3]) || (carry3 && (a[3] || b_xor[3]));
    sum[3] = a[3] ^ b_xor[3] ^ carry3;
    
    return true;
}

bool AdderSubtractor4Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        switch (conn_id) {
            // Input connections
            case 0:  // A3
            case 1:  // A2
            case 2:  // A1
            case 3:  // A0
            case 4:  // B3
            case 5:  // B2
            case 6:  // B1
            case 7:  // B0
            case 8:  // Sub (Subtraction control)
            case 9:  // Cin (Carry input)
                // Skip inputs - they are handled by PutRaw
                break;
            // Output connections
            case 10: // S3 (Sum bit 3)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[3], 0, 1);
                break;
            case 11: // S2 (Sum bit 2)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[2], 0, 1);
                break;
            case 12: // S1 (Sum bit 1)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[1], 0, 1);
                break;
            case 13: // S0 (Sum bit 0)
                return dest.PutRaw(dest_conn_id, (byte*)&sum[0], 0, 1);
                break;
            case 14: // Cout (Carry output)
                return dest.PutRaw(dest_conn_id, (byte*)&carry_out, 0, 1);
                break;
            default:
                // For any other connection IDs, just acknowledge (for dummy pins or similar)
                break;
        }
    }
    else {
        LOG("error: AdderSubtractor4Bit: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool AdderSubtractor4Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0: // A3
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[3] = *data & 1;
            break;
        case 1: // A2
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[2] = *data & 1;
            break;
        case 2: // A1
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[1] = *data & 1;
            break;
        case 3: // A0
            ASSERT(data_bytes == 0 && data_bits == 1);
            a[0] = *data & 1;
            break;
        case 4: // B3
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[3] = *data & 1;
            break;
        case 5: // B2
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[2] = *data & 1;
            break;
        case 6: // B1
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[1] = *data & 1;
            break;
        case 7: // B0
            ASSERT(data_bytes == 0 && data_bits == 1);
            b[0] = *data & 1;
            break;
        case 8: // Sub (Subtraction control)
            ASSERT(data_bytes == 0 && data_bits == 1);
            sub = *data & 1;
            break;
        case 9: // Cin (Carry input)
            ASSERT(data_bytes == 0 && data_bits == 1);
            carry_in = *data & 1;
            break;
        default:
            LOG("error: AdderSubtractor4Bit: unimplemented conn-id " + AsString((int)conn_id));
            return false;
    }
    return true;
}