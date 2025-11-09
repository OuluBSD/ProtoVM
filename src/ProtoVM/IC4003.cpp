#include "ProtoVM.h"
#include "IC4003.h"

/*
 * Intel 4003 Shift Register Implementation for ProtoVM
 *
 * The Intel 4003 is a 4-bit shift register used for I/O expansion in the 4004 system.
 * It's typically used to expand the limited I/O pins of the 4004 CPU.
 */

IC4003::IC4003() {
    shift_reg = 0;
    output_latch = 0;
    char_output_callback = nullptr;  // Initialize to no callback

    // Add the pins for the 4003 shift register
    // Output pins
    AddSource("O0");  // Output 0
    AddSource("O1");  // Output 1
    AddSource("O2");  // Output 2
    AddSource("O3");  // Output 3

    // Latch pins
    AddSink("L0");    // Latch 0
    AddSink("L1");    // Latch 1
    AddSink("L2");    // Latch 2
    AddSink("L3");    // Latch 3

    // Serial input/output and clock
    AddSink("SR0");   // Serial input
    AddSource("SO0"); // Serial output
    AddSink("CM4");   // Clock input

    in_pins = 0;
    in_data = 0;

    LOG("IC4003: Initialized 4-bit shift register");
}

bool IC4003::Tick() {
    bool clock_active = (in_pins & (1 << CM4)) != 0;

    // Check if latch pins are active to update output
    bool latch_active = (in_pins & (1 << L0)) ||
                        (in_pins & (1 << L1)) ||
                        (in_pins & (1 << L2)) ||
                        (in_pins & (1 << L3));

    if (clock_active) {
        // Shift on clock pulse
        ShiftRegister();
    }

    if (latch_active) {
        // Latch on latch signal
        LatchOutput();
        // Process output data for character output
        ProcessOutputData();
    }

    // Update output pins
    UpdateOutput();

    // Reset input values for next tick
    in_pins = 0;
    in_data = 0;

    return true;
}

bool IC4003::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
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
                tmp[0] = (output_latch >> (conn_id - O0)) & 0x1;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);

            // Handle serial output
            case SO0:
                tmp[0] = (shift_reg & 0x8) ? 1 : 0; // Output the MSB
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);

            default:
                LOG("IC4003::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool IC4003::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;

    switch (conn_id) {
        // Handle serial input pin
        case SR0:
            if (data_bytes == 0 && data_bits == 1) {
                // Store the serial input bit for next clock
                in_data = (*data) & 0x1;
            }
            break;

        // Handle control pins
        case L0:
        case L1:
        case L2:
        case L3:
        case CM4:
            ASSERT(data_bytes == 0 && data_bits == 1);
            value = (*data & 0x1) != 0;
            SetPin(conn_id, value);
            break;

        default:
            LOG("IC4003::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void IC4003::SetPin(int i, bool b) {
    uint32 mask = 1UL << i;
    if (b)
        in_pins |= mask;
    else
        in_pins &= ~mask;
}

void IC4003::ShiftRegister() {
    // Shift the register left and input the serial bit
    // Original LSB is lost, new bit goes to MSB
    shift_reg = ((shift_reg << 1) & 0xE) | (in_data & 0x1);
}

void IC4003::LatchOutput() {
    // Latch the current shift register value to output
    // In a real 4003, the outputs are individually latched by L0-L3
    // For simplicity in this simulation, we'll latch all bits when any latch is active

    // Check which latches are being activated and update corresponding outputs
    if (in_pins & (1 << L0)) {
        output_latch = (output_latch & 0xE) | (shift_reg & 0x1); // Update bit 0
    }
    if (in_pins & (1 << L1)) {
        output_latch = (output_latch & 0xD) | ((shift_reg & 0x2)); // Update bit 1
    }
    if (in_pins & (1 << L2)) {
        output_latch = (output_latch & 0xB) | ((shift_reg & 0x4)); // Update bit 2
    }
    if (in_pins & (1 << L3)) {
        output_latch = (output_latch & 0x7) | ((shift_reg & 0x8)); // Update bit 3
    }

    // Or for simplicity, update all outputs if any latch is active
    // (This is simplified - real chip has individual latch control)
    output_latch = shift_reg;
}

void IC4003::UpdateOutput() {
    // The output is already updated in LatchOutput, but we update change status here
    bool output_changed = false;
    static byte last_output = 0xFF;

    if (output_latch != last_output) {
        output_changed = true;
        last_output = output_latch;
    }

    SetChanged(output_changed);
}

void IC4003::SetCharacterOutputCallback(void (*callback)(char c)) {
    char_output_callback = callback;
}

void IC4003::ProcessOutputData() {
    // This method is called when output data is latched
    // In a 4004 system, character output typically happens through I/O operations
    // where the 4-bit data combined with control signals represents a character to output
    
    if (char_output_callback) {
        // For demonstration, if the output latch contains a printable character,
        // call the callback to output it
        char c = (char)(output_latch & 0x7F);  // Use only 7 bits to ensure it's a valid ASCII char
        
        // Check if it's a printable character (space through tilde)
        if (c >= 0x20 && c <= 0x7E) {
            char_output_callback(c);
        }
        // Special handling for newline
        else if (c == 0x0A || c == 0x0D) {
            char_output_callback('\n');
        }
        // Could add more special character handling as needed
    }
}