#ifndef _ProtoVM_ALU_h_
#define _ProtoVM_ALU_h_

#include "ProtoVM.h"

// ALU (Arithmetic Logic Unit) component
// Supports various arithmetic and logic operations on two n-bit operands
class ALU : public ElcBase {
    // Configuration
    static const int WIDTH = 8;  // Default to 8-bit operations
    
    // Operation control codes
    typedef enum {
        OP_AND = 0,      // A AND B
        OP_OR,           // A OR B
        OP_XOR,          // A XOR B
        OP_NOT_A,        // NOT A
        OP_NOT_B,        // NOT B
        OP_ADD,          // A + B
        OP_SUB,          // A - B
        OP_INC_A,        // A + 1
        OP_DEC_A,        // A - 1
        OP_PASS_A,       // A (pass through)
        OP_PASS_B,       // B (pass through)
        OP_NAND,         // A NAND B
        OP_NOR,          // A NOR B
        OP_XNOR,         // A XNOR B
        OP_SHL,          // A << 1 (shift left)
        OP_SHR,          // A >> 1 (shift right)
        OP_MAX           // Maximum operation code
    } Operation;

    // Operation control
    Operation op = OP_ADD;  // Current operation
    
    // Input operands
    byte a[WIDTH] = {0};   // First operand A
    byte b[WIDTH] = {0};   // Second operand B
    byte carry_in = 0;     // Carry input for arithmetic operations
    
    // Results
    byte result[WIDTH] = {0};  // Result of the operation
    bool carry_out = 0;        // Carry out for arithmetic operations
    bool overflow = 0;         // Overflow flag for arithmetic operations
    bool zero = 1;             // Zero flag - set if result is zero
    bool negative = 0;         // Negative flag - set if result is negative (MSB is 1)

    // Internal computation helpers
    void ComputeResult();

public:
    ALU(int width = WIDTH);
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    // Getters for flags
    bool GetZeroFlag() const { return zero; }
    bool GetCarryFlag() const { return carry_out; }
    bool GetOverflowFlag() const { return overflow; }
    bool GetNegativeFlag() const { return negative; }
    
    // Setter for operation
    void SetOperation(Operation operation) { op = operation; }
};

#endif