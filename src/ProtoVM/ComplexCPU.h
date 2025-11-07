#ifndef _ProtoVM_ComplexCPU_h_
#define _ProtoVM_ComplexCPU_h_

#include "ProtoVM.h"
#include "StandardLibrary.h"
#include "ComponentHierarchy.h"
#include "ALU.h"
#include "IC6502.h"

// A more complex 8-bit CPU core with control unit and instruction decoding
class Complex8BitCPU : public HierarchicalComponent {
private:
    // Main functional units
    ALU* alu;                    // Arithmetic Logic Unit
    Register8Bit* reg_a;         // Accumulator
    Register8Bit* reg_x;         // X register (index)
    Register8Bit* reg_y;         // Y register (index)
    Register8Bit* reg_sp;        // Stack pointer
    Counter8Bit* pc;             // Program counter
    Register8Bit* ir;            // Instruction register
    Counter4Bit* cycles;         // Cycle counter for multi-cycle instructions
    Mux4to1* addr_mux;          // Address multiplexer
    Mux4to1* data_mux;          // Data multiplexer
    
    // Flags register (using individual bits)
    bool carry_flag;
    bool zero_flag;
    bool negative_flag;
    bool overflow_flag;
    
    // Internal control signals
    bool fetch_cycle;
    bool decode_cycle;
    bool execute_cycle;
    
    // Instruction types for the control unit
    enum InstructionType {
        NOP = 0x00,
        LDA_IMM = 0xA9,  // Load Accumulator with Immediate
        LDA_ABS = 0xAD,  // Load Accumulator with Absolute
        STA_ABS = 0x8D,  // Store Accumulator to Absolute
        ADC_IMM = 0x69,  // Add with Carry Immediate
        SBC_IMM = 0xE9,  // Subtract with Carry Immediate
        AND_IMM = 0x29,  // AND with Immediate
        ORA_IMM = 0x09,  // OR with Immediate
        EOR_IMM = 0x49,  // XOR with Immediate
        CMP_IMM = 0xC9,  // Compare with Immediate
        BEQ_REL = 0xF0,  // Branch if Equal (Relative)
        BNE_REL = 0xD0,  // Branch if Not Equal (Relative)
        JSR_ABS = 0x20,  // Jump to Subroutine (Absolute)
        RTS_IMP = 0x60,  // Return from Subroutine
        PHA_IMP = 0x48,  // Push Accumulator
        PLA_IMP = 0x68,  // Pull Accumulator
        TAX_IMP = 0xAA,  // Transfer A to X
        TAY_IMP = 0xA8,  // Transfer A to Y
        TXA_IMP = 0x8A,  // Transfer X to A
        TYA_IMP = 0x98,  // Transfer Y to A
        BRK_IMP = 0x00   // Break instruction
    };
    
public:
    Complex8BitCPU();
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
    
    // Helper methods for execution
    byte GetCurrentInstruction();
    void ExecuteInstruction(byte instruction);
    void SetFlagsFromResult(byte result);
    
    // Memory access methods
    void WriteMemory(int addr, byte data);
    byte ReadMemory(int addr);
    
    // Debugging and inspection methods
    void DumpCPUState();
    String GetRegisterState();
};

// A simple test program for the CPU
class CPUTestProgram {
public:
    static void SetupTestProgram(Machine& machine, Complex8BitCPU& cpu);
    static void SetupSimpleAddProgram(Machine& machine, Complex8BitCPU& cpu);
};

#endif