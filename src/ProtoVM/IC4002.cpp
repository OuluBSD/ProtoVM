#include "ProtoVM.h"
#include "IC4002.h"

/*
 * Intel 4002 RAM Implementation for ProtoVM
 *
 * The Intel 4002 is a 40-bit (40x1) static RAM chip with:
 * - 40 memory locations of 1 bit each
 * - 4 outputs (4 banks of 10 bits each)
 * - Separate input and output pins
 * - Character generator RAM in original implementation
 */

IC4002::IC4002() {
    // Initialize all memory to 0
    for (int bank = 0; bank < 4; bank++) {
        for (int pos = 0; pos < 10; pos++) {
            memory[bank][pos] = 0;
        }
    }
    
    address = 0;
    bank = 0;
    write_mode = false;
    input_data = 0;
    output_data = 0;

    // Add the pins for the 4002 RAM
    // Address inputs
    AddSink("A0");  // Address bit 0
    AddSink("A1");  // Address bit 1
    AddSink("A2");  // Address bit 2
    AddSink("A3");  // Address bit 3

    // Bank selection inputs
    AddSink("C0");  // Bank 0 select
    AddSink("C1");  // Bank 1 select
    AddSink("C2");  // Bank 2 select
    AddSink("C3");  // Bank 3 select

    // Input data bits
    AddSink("I0");  // Input data bit 0
    AddSink("I1");  // Input data bit 1
    AddSink("I2");  // Input data bit 2
    AddSink("I3");  // Input data bit 3

    // Output data bits
    AddSource("O0"); // Output data bit 0
    AddSource("O1"); // Output data bit 1
    AddSource("O2"); // Output data bit 2
    AddSource("O3"); // Output data bit 3

    // Control signals
    AddSink("WM");   // Write Memory enable
    AddSink("CM4");  // Clock input

    in_pins = 0;
    in_data = 0;

    LOG("IC4002: Initialized with 40-bit memory (4 banks of 10 bits each)");
}

bool IC4002::Tick() {
    // Check if clock is active and determine if we should process
    bool clock_active = (in_pins & (1 << CM4)) != 0;
    bool write_enable = (in_pins & (1 << WM)) != 0;

    // Update address and bank based on input pins
    address = ((in_pins & (1 << A3)) ? 8 : 0) |
              ((in_pins & (1 << A2)) ? 4 : 0) |
              ((in_pins & (1 << A1)) ? 2 : 0) |
              ((in_pins & (1 << A0)) ? 1 : 0);
    
    // Limit to valid range (0-9 for 4002)
    if (address > 9) address = 9;
    
    bank = ((in_pins & (1 << C3)) ? 8 : 0) |
           ((in_pins & (1 << C2)) ? 4 : 0) |
           ((in_pins & (1 << C1)) ? 2 : 0) |
           ((in_pins & (1 << C0)) ? 1 : 0);
    
    // Limit to valid range (0-3 for 4 banks)
    if (bank > 3) bank = 3;

    // Perform memory operation if clock is active
    if (clock_active) {
        if (write_enable && write_mode) {
            WriteMemory();
        } else {
            ReadMemory();
        }
    }

    // Update output pins based on current memory state
    UpdateOutput();
    
    // Reset input values for next tick
    in_data = 0;
    in_pins = 0;

    return true;
}

bool IC4002::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
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
                LOG("IC4002::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool IC4002::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;
    
    switch (conn_id) {
        // Handle input data pins
        case I0:
        case I1:
        case I2:
        case I3:
            if (data_bytes == 0 && data_bits == 1) {
                byte bit_pos = conn_id - I0;
                byte mask = 1 << bit_pos;
                input_data = (input_data & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle control and address pins
        case A0:
        case A1:
        case A2:
        case A3:
        case C0:
        case C1:
        case C2:
        case C3:
        case WM:
        case CM4:
            ASSERT(data_bytes == 0 && data_bits == 1);
            value = (*data & 0x1) != 0;
            SetPin(conn_id, value);
            break;

        default:
            LOG("IC4002::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void IC4002::SetPin(int i, bool b) {
    uint32 mask = 1UL << i;
    if (b)
        in_pins |= mask;
    else
        in_pins &= ~mask;
}

void IC4002::ReadMemory() {
    // Read from the specified address in the selected bank
    if (address < 10 && bank < 4) {
        // For 4002, we have 4 separate output pins (O0-O3), each from a different bank
        // at the same address. However, in this implementation we'll treat it as one
        // 4-bit wide output from the selected bank.
        
        output_data = 0;
        for (int i = 0; i < 4; i++) {
            if (bank + i < 4) {
                byte bit_val = memory[bank + i][address] & 0x1;
                output_data |= (bit_val << i);
            }
        }
    }
}

void IC4002::WriteMemory() {
    // Write to the specified address in the selected bank
    if (address < 10 && bank < 4) {
        // In 4002, there are 4 input pins that correspond to 4 different memory banks
        // at the same address position.
        for (int i = 0; i < 4; i++) {
            if (bank + i < 4) {
                memory[bank + i][address] = (input_data >> i) & 0x1;
            }
        }
    }
}

void IC4002::UpdateOutput() {
    // The output is already updated in ReadMemory, but we update change status here
    bool output_changed = false;
    static byte last_output = 0xFF;
    
    if (output_data != last_output) {
        output_changed = true;
        last_output = output_data;
    }
    
    SetChanged(output_changed);
}