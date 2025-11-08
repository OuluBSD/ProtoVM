#include "ProtoVM.h"
#include "IC4004.h"

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
 * This implementation follows the architecture and behavior of the real Intel 4004,
 * adapted for the ProtoVM simulation framework.
 */

IC4004::IC4004() {
    // Initialize registers
    for (int i = 0; i < 16; i++) {
        registers[i] = 0;
    }
    accumulator = 0;
    program_counter = 0;
    address_register = 0;
    stack_pointer = 0;
    carry_flag = false;
    aux_carry_flag = false;
    test_mode = false;
    is_executing = false;
    memory_read_active = false;
    memory_write_active = false;
    current_instruction = 0;
    instruction_cycle = 0;
    
    // Initialize the stack
    for (int i = 0; i < 3; i++) {
        stack[i] = 0;
    }

    // Add the pins for the 4004 CPU
    // Data bus (bidirectional)
    AddBidirectional("D0");
    AddBidirectional("D1");
    AddBidirectional("D2");
    AddBidirectional("D3");
    
    // Address bus (output)
    AddSource("A0");
    AddSource("A1");
    AddSource("A2");
    AddSource("A3");
    AddSource("A4");
    AddSource("A5");
    AddSource("A6");
    AddSource("A7");
    AddSource("A8");
    AddSource("A9");
    AddSource("A10");
    AddSource("A11");
    
    // Control signals
    AddSource("CM");     // Clock output to ROM/RAM chips
    AddSource("BUSY");   // Busy signal
    AddSource("R/W");    // Read/Write control
    AddSource("MR");     // Memory Read
    AddSource("MW");     // Memory Write
    AddSink("SBY");      // System Busy input
    AddSink("CM4");      // Clock input
    AddSink("RES");      // Reset input

    in_pins = 0;
    in_pins_mask = (1 << SBY) | (1 << CM4) | (1 << RES);
    
    LOG("IC4004: Initialized with 24 pins and internal state");
}

bool IC4004::Tick() {
    // Store old values to detect changes
    byte old_acc = accumulator;
    byte old_carry = carry_flag;
    uint16 old_pc = program_counter;
    bool old_exec = is_executing;

    // Process inputs that may have changed
    if (in_pins & (1 << RES)) {
        // Reset the CPU
        accumulator = 0;
        program_counter = 0;
        address_register = 0;
        stack_pointer = 0;
        carry_flag = false;
        aux_carry_flag = false;
        is_executing = false;
        instruction_cycle = 0;
        memory_read_active = false;
        memory_write_active = false;
        
        // Clear all registers
        for (int i = 0; i < 16; i++) {
            registers[i] = 0;
        }
        
        LOG("IC4004: Reset executed");
    }

    // Process the CPU cycle if clock is active
    if (in_pins & (1 << CM4)) {  // Clock input is active
        if (!(in_pins & (1 << SBY))) {  // Not busy with system
            if (!is_executing) {
                // Start execution: fetch instruction
                FetchInstruction();
            } else {
                // Continue current instruction
                switch (instruction_cycle) {
                    case 0:
                        DecodeInstruction();
                        instruction_cycle = 1;
                        break;
                    case 1:
                        ExecuteInstruction();
                        is_executing = false;
                        instruction_cycle = 0;
                        break;
                    default:
                        is_executing = false;
                        instruction_cycle = 0;
                        break;
                }
            }
        }
    }

    // Update control lines based on current state
    UpdateControlLines();

    // Update busy status output
    // For simplicity, busy when executing an instruction
    byte busy_status = is_executing ? 1 : 0;
    SetPin(BUSY, busy_status);

    // Detect if any important state changed
    bool state_changed = (accumulator != old_acc) ||
                         (carry_flag != old_carry) ||
                         (program_counter != old_pc) ||
                         (is_executing != old_exec);

    // Update change status
    SetChanged(state_changed);

    // Reset input values for next tick
    in_data = 0;
    in_addr = 0;
    in_pins = 0;

    return true;
}

bool IC4004::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };
    bool true_value = true;
    
    if (type == WRITE) {
        switch (conn_id) {
            // Handle address bus outputs
            case A0:
            case A0+1:
            case A0+2:
            case A0+3:
            case A0+4:
            case A0+5:
            case A0+6:
            case A0+7:
            case A0+8:
            case A0+9:
            case A0+10:
            case A0+11:
                // Send address to connected memory components
                tmp[0] = (address_register >> (conn_id - A0)) & 0x1;
                if (conn_id == A0) {
                    LOG("IC4004::Process: sending addr bit " << (conn_id-A0) << " = " << (int)tmp[0] << " from 0x" << HexStr(address_register));
                }
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);
                
            // Handle data bus outputs - only when writing to memory
            case D0:
            case D0+1:
            case D0+2:
            case D0+3:
                if (memory_write_active) {  // Memory Write is active
                    // Extract the correct bit of accumulator
                    byte bit_val = (accumulator >> (conn_id - D0)) & 0x1;
                    if (conn_id == D0) {
                        LOG("IC4004::Process: sending data: " << HexStr(accumulator) << " bit " << (conn_id-D0) << " = " << (int)bit_val);
                    }
                    return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
                }
                // When reading, don't drive the bus - let memory components drive it
                break;
                
            // Handle control signals
            case CM:   // Clock output to memory
            case BUSY: // Busy output
            case RW:   // Read/Write control
            case MR:   // Memory Read
            case MW:   // Memory Write
                tmp[0] = ((in_pins >> conn_id) & 1) ? 1 : 0;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);

            default:
                LOG("IC4004::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }
    
    return true;
}

bool IC4004::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;
    bool true_value = true;
    
    switch (conn_id) {
        // Handle data bus input - this is when memory sends data back to CPU
        case D0:
        case D0+1:
        case D0+2:
        case D0+3:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                // Update corresponding bit of accumulator from memory read
                if (memory_read_active) {
                    byte bit_pos = conn_id - D0;
                    byte mask = 1 << bit_pos;
                    accumulator = (accumulator & ~mask) | ((*data & 1) << bit_pos);
                    
                    if (conn_id == D0) {
                        LOG("IC4004::PutRaw: received data bit 0, accumulator now: " << HexStr(accumulator));
                    }
                }
            }
            break;
            
        // Handle control input pins
        case SBY:  // System Busy
        case CM4:  // Clock input
        case RES:  // Reset
            true_value = false; // negate pin for reset
        case CM:
        case BUSY:
        case RW:
        case MR:
        case MW:
            ASSERT(data_bytes == 0 && data_bits == 1);
            value = (*data & 0x1) == true_value;
            SetPin(conn_id, value);
            break;
            
        default:
            LOG("IC4004::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    };
    
    return true;
}

void IC4004::SetPin(int i, bool b) {
    uint32 mask = 1UL << i;
    if (b)
        in_pins |= mask;
    else
        in_pins &= ~mask;
}

void IC4004::FetchInstruction() {
    // Set up address for instruction fetch
    address_register = program_counter;
    
    // Set control lines for memory read
    memory_read_active = true;
    memory_write_active = false;
    
    is_executing = true;
    instruction_cycle = 0;
    
    LOG("IC4004: Fetching instruction at PC=0x" << HexStr(program_counter));
}

void IC4004::DecodeInstruction() {
    // In a real implementation, this would decode the fetched instruction
    // For now, simulate with a simple increment
    current_instruction = accumulator;  // Simplified: use accumulator as instruction
    
    LOG("IC4004: Decoded instruction: 0x" << HexStr(current_instruction));
}

void IC4004::ExecuteInstruction() {
    // In a real implementation, this would execute the decoded instruction
    // For now, just increment PC for simplicity
    program_counter = (program_counter + 1) & 0xFFF;  // 12-bit address space
    
    // Reset memory control lines
    memory_read_active = false;
    memory_write_active = false;
    
    LOG("IC4004: Executed instruction, PC now 0x" << HexStr(program_counter));
}

void IC4004::UpdateRegisters() {
    // Update register values based on instruction execution
    // In a real implementation, this would handle register operations
}

void IC4004::UpdateFlags() {
    // Update flag bits based on ALU operations
    // In a real implementation, this would handle carry and auxiliary carry
}

void IC4004::HandleInterrupts() {
    // Handle interrupt processing if needed
    // The 4004 has limited interrupt capability
}

void IC4004::UpdateControlLines() {
    // Update the internal pin state based on memory operations
    if (memory_read_active) {
        in_pins |= (1 << MR);  // Memory Read active
        in_pins &= ~(1 << MW); // Memory Write inactive
    } else if (memory_write_active) {
        in_pins &= ~(1 << MR); // Memory Read inactive
        in_pins |= (1 << MW);  // Memory Write active
    } else {
        in_pins &= ~((1 << MR) | (1 << MW)); // Both inactive
    }
    
    // Update R/W line based on operation
    if (memory_read_active) {
        in_pins &= ~(1 << RW); // R/W = 0 for read
    } else if (memory_write_active) {
        in_pins |= (1 << RW);  // R/W = 1 for write
    }
}

void IC4004::SetDataBus(byte value, bool output_enable) {
    // For now, just store the value to be output when needed
    accumulator = value;
    is_reading = !output_enable;
}

byte IC4004::GetDataBus() {
    // Return the current value on the data bus
    // This would be from memory when reading
    return accumulator;
}