#include "ProtoVM.h"
#include "IC4001.h"

/*
 * Intel 4001 4-bit ROM Implementation
 */

IC4001::IC4001(int size) {
    // Initialize memory to 0
    for (int i = 0; i < MAX_SIZE; i++) {
        memory[i] = 0;
    }

    // Initialize control signals
    chip_select = false;
    output_enable = false;
    address = 0;
    data_out = 0;

    in_addr = 0;
    in_cs = false;
    in_oe = false;

    // Add the pins for the 4001 ROM chip
    AddSink("A0");      // Address bit 0
    AddSink("A1");      // Address bit 1
    AddSink("A2");      // Address bit 2
    AddSink("A3");      // Address bit 3
    AddSink("A4");      // Address bit 4
    AddSink("A5");      // Address bit 5
    AddSink("A6");      // Address bit 6
    AddSink("A7");      // Address bit 7
    AddSink("A8");      // Address bit 8
    AddSink("A9");      // Address bit 9
    AddBidirectional("D0");    // Bidirectional data pin 0
    AddBidirectional("D1");    // Bidirectional data pin 1
    AddBidirectional("D2");    // Bidirectional data pin 2
    AddBidirectional("D3");    // Bidirectional data pin 3
    AddSink("~CS");     // Chip Select (active low)
    AddSink("~OE");     // Output Enable (active low)

    LOG("IC4001: Initialized 4-bit ROM with " << MAX_SIZE << " 4-bit locations");
}

bool IC4001::Tick() {
    // Store old values for change detection
    byte old_data_out = data_out;
    bool old_cs = chip_select;
    bool old_oe = output_enable;

    // Update from input values
    chip_select = !in_cs;   // Active low CS
    output_enable = !in_oe; // Active low OE
    address = in_addr;

    // Perform memory operation based on control signals
    if (chip_select && output_enable && address < MAX_SIZE) {
        // Output the data from the addressed location
        data_out = memory[address] & 0x0F;  // Only output lower 4 bits
    } else {
        // Chip not selected or output disabled, output high-impedance
        data_out = 0x0F; // High-impedance state represented as all highs
    }

    // Clear input values for next tick
    in_addr = 0;
    in_cs = false;
    in_oe = false;

    // Detect changes
    bool state_changed = (data_out != old_data_out) ||
                         (chip_select != old_cs) ||
                         (output_enable != old_oe);

    SetChanged(state_changed);

    return true;
}

bool IC4001::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };

    if (type == WRITE) {
        switch (conn_id) {
            // Handle bidirectional data pins - only drive when in output mode
            case D0:
            case D1:
            case D2:
            case D3:
                // Only drive the bus when the chip is selected AND output is enabled
                if (chip_select && output_enable) {  // Only drive output when selected and enabled
                    // Extract the correct bit of data_out
                    byte bit_val = (data_out >> (conn_id - D0)) & 0x1;
                    return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
                }
                // If not selected or output disabled, don't drive the bus (high-impedance)
                break;

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
        // Handle address inputs
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
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                byte bit_pos = conn_id - A0;
                byte mask = 1 << bit_pos;
                in_addr = (in_addr & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle control inputs
        case CS: // ~CS (active low)
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_cs = (*data & 0x1) != 0;
            break;
        case OE: // ~OE (active low)
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_oe = (*data & 0x1) != 0;
            break;

        default:
            LOG("IC4001::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void IC4001::WriteMemory(uint16_t addr, byte value) {
    if (addr < MAX_SIZE) {
        memory[addr] = value & 0x0F;  // Only store lower 4 bits
        LOG("IC4001::WriteMemory: Wrote 0x" << HexStr(value & 0x0F) << " to address 0x" << HexStr(addr));
    } else {
        LOG("IC4001::WriteMemory: Address out of range: 0x" << HexStr(addr));
    }
}

byte IC4001::ReadMemory(uint16_t addr) const {
    if (addr < MAX_SIZE) {
        byte value = memory[addr] & 0x0F;
        LOG("IC4001::ReadMemory: Read 0x" << HexStr(value) << " from address 0x" << HexStr(addr));
        return value;
    } else {
        LOG("IC4001::ReadMemory: Address out of range: 0x" << HexStr(addr));
        return 0x0F; // Return all highs for invalid address
    }
}