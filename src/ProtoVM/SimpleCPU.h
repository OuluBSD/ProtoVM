#ifndef _ProtoVM_SimpleCPU_h_
#define _ProtoVM_SimpleCPU_h_

#include "ProtoVM.h"
#include "ALU.h"
#include "StateMachine.h"

// A simple 8-bit CPU with basic components
class SimpleCPU : public ElcBase {
private:
    // CPU registers
    byte accumulator = 0;        // A register
    byte program_counter = 0;    // PC register (8-bit for simplicity)
    byte instruction_register = 0; // IR register
    byte memory_address_register = 0; // MAR register
    byte memory_data_register = 0;    // MDR register
    
    // CPU control flags
    bool carry_flag = false;
    bool zero_flag = false;
    bool negative_flag = false;
    
    // Connected components
    ALU alu;  // Arithmetic Logic Unit
    FsmController control_fsm;  // Control state machine
    
    // Memory interface (simplified)
    // In a real implementation, this would connect to external memory
    
    enum CPUState {
        FETCH = 0,
        DECODE,
        EXECUTE,
        WRITEBACK
    };
    
    // Instruction set
    enum Instruction {
        NOP = 0x00,   // No operation
        LDA_IMM = 0xA9, // Load accumulator with immediate value
        LDA_ABS = 0xAD, // Load accumulator from absolute address
        STA_ABS = 0x8D, // Store accumulator to absolute address
        ADC_IMM = 0x69, // Add with carry immediate
        ADC_ABS = 0x6D, // Add with carry absolute
        SBC_IMM = 0xE9, // Subtract with carry immediate
        SBC_ABS = 0xED, // Subtract with carry absolute
        JMP = 0x4C,   // Jump to address
        BEQ = 0xF0,   // Branch if equal (zero flag set)
        BNE = 0xD0,   // Branch if not equal (zero flag clear)
        BRK = 0x01    // Break/Interrupt (different from NOP)
    };

public:
    SimpleCPU();
    
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // Helper methods
    void ExecuteInstruction();
    void FetchInstruction();
    void DecodeInstruction();
    void ExecuteALUOp(byte op, byte operand);
    void UpdateFlags(byte result);
    
    // Getters for registers
    byte GetAccumulator() const { return accumulator; }
    byte GetProgramCounter() const { return program_counter; }
    
    // Setters for debugging
    void SetAccumulator(byte val) { accumulator = val; }
    void SetProgramCounter(byte val) { program_counter = val; }
};

#endif