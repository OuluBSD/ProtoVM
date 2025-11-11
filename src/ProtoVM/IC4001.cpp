#include "ProtoVM.h"
#include "IC4001.h"

/*
 * Intel 4001 ROM Implementation for ProtoVM
 *
 * The Intel 4001 is a 2048-bit (256x8) read-only memory chip used with the 4004 CPU.
 * In the 4004 system, ROMs are used for program storage.
 */

IC4001::IC4001() {
    // Initialize all memory to 0
    for (int i = 0; i < 4096; i++) {
        memory[i] = 0;
    }
    
    address = 0;
    output_data = 0;
    enabled = true;

    // Add the pins for the 4001 ROM
    // Address inputs (12 bits for 4004 compatibility)
    AddSink("A0");   // Address bit 0
    AddSink("A1");   // Address bit 1
    AddSink("A2");   // Address bit 2
    AddSink("A3");   // Address bit 3
    AddSink("A4");   // Address bit 4
    AddSink("A5");   // Address bit 5
    AddSink("A6");   // Address bit 6
    AddSink("A7");   // Address bit 7
    AddSink("A8");   // Address bit 8
    AddSink("A9");   // Address bit 9
    AddSink("A10");  // Address bit 10
    AddSink("A11");  // Address bit 11

    // Output data bits
    AddSource("O0"); // Output data bit 0
    AddSource("O1"); // Output data bit 1
    AddSource("O2"); // Output data bit 2
    AddSource("O3"); // Output data bit 3

    // Control signals
    AddSink("CM");    // Clock input from CPU
    AddSink("CM4");   // Clock input from system
    AddSink("JAM");   // Chip enable

    in_pins = 0;

    LOG("IC4001: Initialized with 2048x4-bit memory (4096 addresses of 4 bits each)");
}

bool IC4001::Tick() {
    bool clock_active = (in_pins & (1 << CM4)) != 0;
    bool chip_enabled = (in_pins & (1 << JAM)) == 0; // Active low enable

    // Update address based on input pins - only update when chip is enabled
    if (chip_enabled) {
        address = ((in_pins & (1 << A11)) ? 2048 : 0) |
                  ((in_pins & (1 << A10)) ? 1024 : 0) |
                  ((in_pins & (1 << A9))  ? 512  : 0) |
                  ((in_pins & (1 << A8))  ? 256  : 0) |
                  ((in_pins & (1 << A7))  ? 128  : 0) |
                  ((in_pins & (1 << A6))  ? 64   : 0) |
                  ((in_pins & (1 << A5))  ? 32   : 0) |
                  ((in_pins & (1 << A4))  ? 16   : 0) |
                  ((in_pins & (1 << A3))  ? 8    : 0) |
                  ((in_pins & (1 << A2))  ? 4    : 0) |
                  ((in_pins & (1 << A1))  ? 2    : 0) |
                  ((in_pins & (1 << A0))  ? 1    : 0);
    }

    // Perform memory read if clock is active and chip is enabled
    if (clock_active && chip_enabled) {
        ReadMemory();
    }

    // Update output pins based on current memory state
    UpdateOutput();
    
    // Reset input values for next tick
    in_pins = 0;

    return true;
}

bool IC4001::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };
    
    if (type == WRITE) {
        switch (conn_id) {
            // Handle output data pins
            case O0:
            case O0+1:
            case O0+2:
            case O0+3:
                // Send the appropriate output bit
                tmp[0] = (output_data >> (conn_id - O0)) & 0x1;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);

            default:
                LOG("IC4001::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool IC4001::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;
    
    switch (conn_id) {
        // Handle control and address pins
        case A0:
        case A1:
        case A2:
        case A3:
        case A4:
        case A5:
        case A6:
        case A7:
        case A8:
        case A9:
        case A10:
        case A11:
        case CM4:
        case JAM:
            ASSERT(data_bytes == 0 && data_bits == 1);
            value = (*data & 0x1) != 0;
            SetPin(conn_id, value);
            break;

        default:
            LOG("IC4001::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void IC4001::SetPin(int i, bool b) {
    uint32 mask = 1UL << i;
    if (b)
        in_pins |= mask;
    else
        in_pins &= ~mask;
}

void IC4001::ReadMemory() {
    // Read from the specified address, with bounds checking
    if (address < 4096) {
        output_data = memory[address] & 0x0F;  // Ensure only 4 bits
    } else {
        output_data = 0;  // Default to 0 if out of bounds
    }
}

void IC4001::UpdateOutput() {
    // The output is already updated in ReadMemory, but we update change status here
    bool output_changed = false;
    static byte last_output = 0xFF;
    
    if (output_data != last_output) {
        output_changed = true;
        last_output = output_data;
    }
    
    SetChanged(output_changed);
}

void IC4001::LoadProgram(const byte* data, int size) {
    if (size > 4096) size = 4096;  // Limit to available memory
    
    for (int i = 0; i < size; i++) {
        memory[i] = data[i] & 0x0F;  // Ensure only 4 bits
    }
    
    LOG("IC4001: Loaded program of size " << size << " bytes");
}

void IC4001::SetMemory(int addr, byte value) {
    if (addr >= 0 && addr < 4096) {
        memory[addr] = value & 0x0F;  // Ensure only 4 bits
    }
}

byte IC4001::GetMemory(int addr) const {
    if (addr >= 0 && addr < 4096) {
        return memory[addr] & 0x0F;  // Ensure only 4 bits
    }
    return 0;  // Default to 0 if out of bounds
}