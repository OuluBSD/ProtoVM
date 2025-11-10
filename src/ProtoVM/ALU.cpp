#include "ProtoVM.h"
#include "ALU.h"

ALU::ALU(int width) {
    // Ensure width is valid (between 1 and 8 bits for this implementation)
    ASSERT(width > 0 && width <= 8);
    
    // Add input sinks for operands A and B based on width
    for (int i = 0; i < width; i++) {
        AddSink(String().Cat() << "A" << i);  // A0, A1, ..., A7
        AddSink(String().Cat() << "B" << i);  // B0, B1, ..., B7
    }
    
    // Add operation control input (4 bits to support up to 16 operations)
    for (int i = 0; i < 4; i++) {
        AddSink(String().Cat() << "OP" << i); // OP0, OP1, OP2, OP3
    }
    
    // Add carry input
    AddSink("Cin");
    
    // Add result outputs based on width
    for (int i = 0; i < width; i++) {
        AddSource(String().Cat() << "R" << i).SetMultiConn(); // R0, R1, ..., R7
    }
    
    // Add flags
    AddSource("Zero").SetMultiConn();
    AddSource("Carry").SetMultiConn();
    AddSource("Overflow").SetMultiConn();
    AddSource("Negative").SetMultiConn();
    
    // Initialize result array
    for (int i = 0; i < width; i++) {
        result[i] = 0;
    }
}

void ALU::ComputeResult() {
    byte temp_a = 0;
    byte temp_b = 0;
    byte temp_result = 0;
    
    // Reconstruct A and B from individual bits
    for (int i = 0; i < 8; i++) {  // Use 8 bits for internal processing
        if (i < WIDTH) {
            temp_a |= (a[i] ? (1 << i) : 0);
            temp_b |= (b[i] ? (1 << i) : 0);
        }
    }
    
    // Reset flags
    carry_out = 0;
    overflow = 0;
    zero = 1;
    negative = 0;
    
    // Perform the operation based on the operation code
    switch (op) {
        case OP_AND:
            temp_result = temp_a & temp_b;
            break;
        case OP_OR:
            temp_result = temp_a | temp_b;
            break;
        case OP_XOR:
            temp_result = temp_a ^ temp_b;
            break;
        case OP_NOT_A:
            temp_result = ~temp_a & ((1 << WIDTH) - 1);  // Mask to WIDTH bits
            break;
        case OP_NOT_B:
            temp_result = ~temp_b & ((1 << WIDTH) - 1);  // Mask to WIDTH bits
            break;
        case OP_ADD: {
            unsigned int sum = temp_a + temp_b + carry_in;
            temp_result = sum & ((1 << WIDTH) - 1);
            carry_out = (sum >> WIDTH) & 1;
            // Overflow occurs when adding two positives yields negative or two negatives yield positive
            if (WIDTH > 1) {
                bool a_sign = (temp_a >> (WIDTH-1)) & 1;
                bool b_sign = (temp_b >> (WIDTH-1)) & 1;
                bool result_sign = (temp_result >> (WIDTH-1)) & 1;
                overflow = (a_sign == b_sign) && (a_sign != result_sign);
            }
            break;
        }
        case OP_SUB: {
            // For subtraction: A - B, we compute A + (~B + 1) which is two's complement
            unsigned int sub = temp_a - temp_b - !carry_in;
            temp_result = sub & ((1 << WIDTH) - 1);
            carry_out = (!((sub) >> WIDTH)) & 1;  // Borrow is inverted carry
            // Overflow detection for subtraction
            if (WIDTH > 1) {
                bool a_sign = (temp_a >> (WIDTH-1)) & 1;
                bool b_sign = (temp_b >> (WIDTH-1)) & 1;
                bool result_sign = (temp_result >> (WIDTH-1)) & 1;
                overflow = (a_sign != b_sign) && (a_sign != result_sign);
            }
            break;
        }
        case OP_INC_A:
            temp_result = (temp_a + 1) & ((1 << WIDTH) - 1);
            carry_out = ((temp_a + 1) >> WIDTH) & 1;
            break;
        case OP_DEC_A:
            temp_result = (temp_a - 1) & ((1 << WIDTH) - 1);
            carry_out = (!((temp_a - 1) >> WIDTH)) & 1;  // Borrow inverted
            break;
        case OP_PASS_A:
            temp_result = temp_a;
            break;
        case OP_PASS_B:
            temp_result = temp_b;
            break;
        case OP_NAND:
            temp_result = ~(temp_a & temp_b) & ((1 << WIDTH) - 1);
            break;
        case OP_NOR:
            temp_result = ~(temp_a | temp_b) & ((1 << WIDTH) - 1);
            break;
        case OP_XNOR:
            temp_result = ~(temp_a ^ temp_b) & ((1 << WIDTH) - 1);
            break;
        case OP_SHL: {
            temp_result = (temp_a << 1) & ((1 << WIDTH) - 1);
            carry_out = (temp_a >> (WIDTH-1)) & 1;  // MSB shifted out becomes carry
            break;
        }
        case OP_SHR: {
            temp_result = temp_a >> 1;
            carry_out = temp_a & 1;  // LSB shifted out becomes carry
            break;
        }
        default:
            temp_result = 0;  // Invalid operation defaults to 0
            break;
    }
    
    // Update result bits
    for (int i = 0; i < WIDTH; i++) {
        result[i] = (temp_result >> i) & 1;
        if (result[i]) {
            zero = 0;  // Result is not zero if any bit is set
        }
    }
    
    // Set negative flag if MSB is set
    if (WIDTH > 0) {
        negative = result[WIDTH-1];
    }
}

bool ALU::Tick() {
    ComputeResult();
    return true;
}

bool ALU::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Handle outputs
        if (conn_id >= 2*WIDTH && conn_id < 2*WIDTH + 4) {  // Operation control (not handled in Process)
            // Skip operation control inputs - they are handled by PutRaw
        }
        else if (conn_id == 2*WIDTH + 4) {  // Carry input (not handled in Process)
            // Skip carry input - handled by PutRaw
        } 
        else if (conn_id >= 0 && conn_id < WIDTH) {  // Result outputs R0..R(WIDTH-1)
            return dest.PutRaw(dest_conn_id, (byte*)&result[conn_id], 0, 1);
        }
        else if (conn_id == 2*WIDTH + 5) {  // Zero flag
            bool z = zero;
            return dest.PutRaw(dest_conn_id, (byte*)&z, 0, 1);
        }
        else if (conn_id == 2*WIDTH + 6) {  // Carry flag
            return dest.PutRaw(dest_conn_id, (byte*)&carry_out, 0, 1);
        }
        else if (conn_id == 2*WIDTH + 7) {  // Overflow flag
            return dest.PutRaw(dest_conn_id, (byte*)&overflow, 0, 1);
        }
        else if (conn_id == 2*WIDTH + 8) {  // Negative flag
            return dest.PutRaw(dest_conn_id, (byte*)&negative, 0, 1);
        }
        else if (conn_id < 2*WIDTH || conn_id == 2*WIDTH + 4) {
            // These are input connections, no output processing needed
        }
        else {
            LOG("error: ALU: unhandled conn-id " << (int)conn_id);
            return false;
        }
    }
    else {
        LOG("error: ALU: unimplemented ProcessType");
        return false;
    }
    return true;
}

bool ALU::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    ASSERT(data_bytes == 0 && data_bits == 1);
    
    if (conn_id >= 0 && conn_id < WIDTH) {  // A inputs A0..A(WIDTH-1)
        a[conn_id] = *data & 1;
    }
    else if (conn_id >= WIDTH && conn_id < 2*WIDTH) {  // B inputs B0..B(WIDTH-1)
        b[conn_id - WIDTH] = *data & 1;
    }
    else if (conn_id >= 2*WIDTH && conn_id < 2*WIDTH + 4) {  // Operation control OP0..OP3
        int op_bit = conn_id - 2*WIDTH;
        // Update operation code based on all 4 bits
        if (op_bit == 0) {
            op = static_cast<Operation>((op & 0xE) | ((*data & 1) << 0));
        } else if (op_bit == 1) {
            op = static_cast<Operation>((op & 0xD) | ((*data & 1) << 1));
        } else if (op_bit == 2) {
            op = static_cast<Operation>((op & 0xB) | ((*data & 1) << 2));
        } else if (op_bit == 3) {
            op = static_cast<Operation>((op & 0x7) | ((*data & 1) << 3));
        }
    }
    else if (conn_id == 2*WIDTH + 4) {  // Carry input
        carry_in = *data & 1;
    }
    else {
        LOG("error: ALU: unimplemented conn-id " << (int)conn_id);
        return false;
    }
    
    return true;
}