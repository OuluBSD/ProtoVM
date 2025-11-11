#ifndef ProtoVM_IC4004_h
#define ProtoVM_IC4004_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"

/*
 * Intel 4004 CPU Implementation for ProtoVM
 * 
 * The Intel 4004 is a 4-bit microprocessor with:
 * - 4-bit data bus
 * - 12-bit address bus (4096 bytes addressable)
 * - 16 registers (4-bit each)
 * - 45 instructions
 * - 4-bit accumulator (A register)
 * - Clock frequency: 740 kHz (typical)
 * 
 * Pinout:
 * - D0-D3: Data bus (bidirectional)
 * - A0-A11: Address bus (output)
 * - CM: Clock output to ROM/RAM chips
 * - BUSY: Busy signal (output)
 * - R/W: Read/Write control
 * - MR: Memory Read (output)
 * - MW: Memory Write (output)
 * - SBY: System Busy (input)
 * - CM4: Clock input from system
 * - RES: Reset (input)
 */

class IC4004 : public Chip {
public:
    IC4004();
    virtual ~IC4004() {}
    
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    String GetClassName() const override { return "IC4004"; }

    // Debugging methods
    byte GetAccumulator() const { return accumulator; }
    uint16 GetProgramCounter() const { return program_counter; }
    uint16 GetAddressRegister() const { return address_register; }
    byte GetStackPointer() const { return stack_pointer; }
    bool GetCarryFlag() const { return carry_flag; }
    bool GetAuxCarryFlag() const { return aux_carry_flag; }
    bool GetTestMode() const { return test_mode; }
    bool GetIsExecuting() const { return is_executing; }
    bool GetMemoryReadActive() const { return memory_read_active; }
    bool GetMemoryWriteActive() const { return memory_write_active; }
    int GetCurrentInstruction() const { return current_instruction; }
    int GetInstructionCycle() const { return instruction_cycle; }
    byte GetRegister(int index) const { return registers[index % 16]; }
    const byte* GetRegisterPtr() const { return registers; }

private:
    // 4004 has 16 registers of 4 bits each, plus accumulator
    byte registers[16];  // R0-R15: 4-bit registers
    byte accumulator;    // 4-bit accumulator (A register)
    byte stack[3];       // 3-level stack for subroutine calls
    int stack_pointer;
    uint16 program_counter;  // 12-bit program counter
    uint16 address_register; // 12-bit address register for memory access
    
    // 4004 flags (status bits)
    bool carry_flag;
    bool aux_carry_flag;
    bool test_mode; // Used for test mode in 4004
    
    // Internal state
    int current_instruction;
    int instruction_cycle;
    bool is_executing;
    bool memory_read_active;
    bool memory_write_active;
    bool is_reading;
    
    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        D0 = 0,    // Data bus pins
        D1 = 1,
        D2 = 2,
        D3 = 3,
        A0 = 4,    // Address bus pins (12 bits)
        A1 = 5,
        A2 = 6,
        A3 = 7,
        A4 = 8,
        A5 = 9,
        A6 = 10,
        A7 = 11,
        A8 = 12,
        A9 = 13,
        A10 = 14,
        A11 = 15,
        CM = 16,   // Clock output
        BUSY = 17, // Busy signal
        RW = 18,   // Read/Write control
        MR = 19,   // Memory Read
        MW = 20,   // Memory Write
        SBY = 21,  // System Busy input
        CM4 = 22,  // Clock input
        RES = 23,  // Reset input
        OUT0 = 24, // Output port 0
        OUT1 = 25, // Output port 1
        OUT2 = 26, // Output port 2
        OUT3 = 27  // Output port 3
    };
    
    // Internal state for current tick
    byte in_data;
    uint16 in_addr;
    uint32 in_pins;
    uint32 in_pins_mask;
    
    // Timing and clock management
    int current_cycle;          // Current cycle within instruction
    int total_cycles;           // Total cycles for current instruction
    int clock_divider;          // To simulate different clock speeds
    int clock_count;            // Internal clock counter

    void SetPin(int i, bool b);
    void FetchInstruction();
    void DecodeInstruction();
    void ExecuteInstruction();
    void UpdateRegisters();
    void UpdateFlags();
    void HandleInterrupts();
    void UpdateControlLines();
    void SetDataBus(byte value, bool output_enable = true);
    byte GetDataBus();
    byte GetMemoryAtAddress(uint16 addr);
    
    // Timing-related methods
    void UpdateTiming(int current_tick);
    bool CheckTimingRequirements();
    void SetClockDivider(int divider);
    int GetClockDivider() const { return clock_divider; }
    void UpdateClockCount();
    bool IsClockRisingEdge();
};

#endif
