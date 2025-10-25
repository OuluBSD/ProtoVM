#ifndef _ProtoVM_Arithmetic_h_
#define _ProtoVM_Arithmetic_h_

#include "ProtoVM.h"

// Full Adder component that takes two bits and a carry-in, and produces a sum bit and carry-out
class FullAdder : public ElcBase {
    //RTTI_DECL1(FullAdder, ElcBase);
    bool a = 0;        // First input bit
    bool b = 0;        // Second input bit
    bool carry_in = 0; // Carry input
    bool sum = 0;      // Sum output
    bool carry_out = 0; // Carry output
    
public:
    FullAdder();
    
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// 4-bit Adder component using four Full Adder components
class Adder4Bit : public ElcBase {
    //RTTI_DECL1(Adder4Bit, ElcBase);
    bool a[4] = {0, 0, 0, 0};  // First 4-bit operand
    bool b[4] = {0, 0, 0, 0};  // Second 4-bit operand
    bool carry_in = 0;         // Carry input
    bool sum[4] = {0, 0, 0, 0}; // 4-bit sum output
    bool carry_out = 0;        // Carry output
    
public:
    Adder4Bit();
    
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// 4-bit Adder/Subtractor component using four Full Adder components and XOR gates for subtraction
class AdderSubtractor4Bit : public ElcBase {
    //RTTI_DECL1(AdderSubtractor4Bit, ElcBase);
    bool a[4] = {0, 0, 0, 0};  // First 4-bit operand
    bool b[4] = {0, 0, 0, 0};  // Second 4-bit operand
    bool sub = 0;              // Subtraction control (0=add, 1=subtract)
    bool carry_in = 0;         // Carry input
    bool sum[4] = {0, 0, 0, 0}; // 4-bit sum/output
    bool carry_out = 0;        // Carry output
    
private:
    // Internal XOR gates to invert b when doing subtraction
    bool b_xor[4] = {0, 0, 0, 0};  // XOR result of b with sub control
    bool internal_carry = 0;        // Internal carry connection

public:
    AdderSubtractor4Bit();
    
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif