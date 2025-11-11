#include "ProtoVM.h"
#include "IC4002.h"

/*
 * Intel 4002 4-bit Static RAM Implementation
 */

IC4002::IC4002(int size) {
    // Initialize memory to 0
    for (int i = 0; i < MAX_SIZE; i++) {
        memory[i] = 0;
    }

    // Initialize control signals
    chip_select = false;
    write_enable = false;
    address = 0;
    data_in = 0;
    data_out = 0;

    in_addr = 0;
    in_data = 0;
    in_cs = false;
    in_we = false;

    // Add the pins for the 4002 RAM chip
    AddSink("A0");      // Address bit 0
    AddSink("A1");      // Address bit 1
    AddSink("A2");      // Address bit 2
    AddSink("A3");      // Address bit 3
    AddBidirectional("D0");    // Bidirectional data pin 0
    AddBidirectional("D1");    // Bidirectional data pin 1
    AddBidirectional("D2");    // Bidirectional data pin 2
    AddBidirectional("D3");    // Bidirectional data pin 3
    AddSink("~CS");     // Chip Select (active low)
    AddSink("WE");      // Write Enable (active high)

    LOG("IC4002: Initialized 4-bit RAM with " << MAX_SIZE << " 4-bit locations");
}

bool IC4002::Tick() {
    // Store old values for change detection
    byte old_data_out = data_out;
    bool old_cs = chip_select;
    bool old_we = write_enable;

    // Update from input values
    chip_select = !in_cs;  // Active low CS
    write_enable = in_we;
    address = in_addr;
    data_in = in_data;

    // Perform memory operation based on control signals
    if (chip_select) {
        if (write_enable && address < MAX_SIZE) {
            // Write operation: write data_in to memory at address
            memory[address] = data_in & 0x0F;  // Only use lower 4 bits
        }

        // Output the data from the addressed location
        if (address < MAX_SIZE) {
            data_out = memory[address] & 0x0F;  // Only output lower 4 bits
        } else {
            data_out = 0x0F; // Invalid address, output all highs (default high-impedance state)
        }
    } else {
        // Chip not selected, output high-impedance
        data_out = 0x0F; // High-impedance state represented as all highs
    }

    // Clear input values for next tick
    in_addr = 0;
    in_data = 0;
    in_cs = false;
    in_we = false;

    // Detect changes
    bool state_changed = (data_out != old_data_out) ||
                         (chip_select != old_cs) ||
                         (write_enable != old_we);

    SetChanged(state_changed);

    return true;
}

bool IC4002::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };

    if (type == WRITE) {
        switch (conn_id) {
            // Handle bidirectional data pins - only drive when in output mode (read operation)
            case D0:
            case D1:
            case D2:
            case D3:
                // Only drive the bus when the chip is selected AND we're in read mode (not write)
                if (chip_select && !write_enable) {  // Only drive output when selected and in read mode
                    // Extract the correct bit of data_out
                    byte bit_val = (data_out >> (conn_id - D0)) & 0x1;
                    return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
                }
                // If not selected or in write mode, don't drive the bus (high-impedance)
                break;

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
        // Handle address inputs
        case A0:
        case A1:
        case A2:
        case A3:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                byte bit_pos = conn_id - A0;
                byte mask = 1 << bit_pos;
                in_addr = (in_addr & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle bidirectional data pins
        case D0:
        case D1:
        case D2:
        case D3:
            if (data_bytes == 0 && data_bits == 1) {  // Single bit input
                byte bit_pos = conn_id - D0;
                byte mask = 1 << bit_pos;
                in_data = (in_data & ~mask) | ((*data & 1) << bit_pos);
            }
            break;

        // Handle control inputs
        case CS: // ~CS (active low)
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_cs = (*data & 0x1) != 0;
            break;
        case WE: // WE (active high)
            ASSERT(data_bytes == 0 && data_bits == 1);
            in_we = (*data & 0x1) != 0;
            break;

        default:
            LOG("IC4002::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void IC4002::WriteMemory(uint8_t addr, byte value) {
    if (addr < MAX_SIZE) {
        memory[addr] = value & 0x0F;  // Only store lower 4 bits
        LOG("IC4002::WriteMemory: Wrote 0x" << HexStr(value & 0x0F) << " to address 0x" << HexStr(addr));
    } else {
        LOG("IC4002::WriteMemory: Address out of range: 0x" << HexStr(addr));
    }
}

byte IC4002::ReadMemory(uint8_t addr) const {
    if (addr < MAX_SIZE) {
        byte value = memory[addr] & 0x0F;
        LOG("IC4002::ReadMemory: Read 0x" << HexStr(value) << " from address 0x" << HexStr(addr));
        return value;
    } else {
        LOG("IC4002::ReadMemory: Address out of range: 0x" << HexStr(addr));
        return 0x0F; // Return all highs for invalid address
    }
}