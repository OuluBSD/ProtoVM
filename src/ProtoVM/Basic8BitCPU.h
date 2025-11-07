#ifndef _ProtoVM_Basic8BitCPU_h_
#define _ProtoVM_Basic8BitCPU_h_

#include "ProtoVM.h"

// A basic 8-bit CPU implementation with simple instruction set
// Demonstrates more complex CPU core architecture
class Basic8BitCPU : public ElcBase {
private:
    // CPU Registers
    byte A;        // Accumulator
    byte X;        // Index register X
    byte Y;        // Index register Y
    byte S;        // Stack pointer
    byte PC_L;     // Program counter low
    byte PC_H;     // Program counter high
    byte SP_L;     // Stack pointer low
    byte SP_H;     // Stack pointer high
    
    // Status flags
    bool carry_flag;
    bool zero_flag;
    bool interrupt_flag;
    bool decimal_flag;
    bool break_flag;
    bool overflow_flag;
    bool negative_flag;
    
    // Internal state
    byte opcode;
    byte operand;
    int cycle_count;
    bool executing;
    bool halt;
    
    // Memory interface
    byte bus_address[16];  // 16-bit address bus
    byte bus_data[8];      // 8-bit data bus
    bool read_write;       // R/W signal

public:
    Basic8BitCPU();
    
    // Main execution methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // CPU control methods
    void Reset();
    void ExecuteInstruction();
    void UpdateFlags(byte result);
    
    // Getter methods for debugging
    byte GetAccumulator() const { return A; }
        byte GetXRegister() const { return X; }
    byte GetYRegister() const { return Y; }
    byte GetStackPointer() const { return S; }
    uint16 GetProgramCounter() const { return (PC_H << 8) | PC_L; }
    bool GetHaltState() const { return halt; }
    
    // Setter methods
    void SetHalt(bool h) { halt = h; }
};

#endif