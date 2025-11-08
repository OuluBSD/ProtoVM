#include "ProtoVM.h"
#include "AddressDecoder4004.h"

/*
 * Address Decoder for Intel 4004 System
 *
 * Handles memory mapping for the 4004 system:
 * - Decodes 12-bit addresses to select appropriate memory/IO chips
 * - Manages chip select signals for ROM and RAM chips
 */

AddressDecoder4004::AddressDecoder4004() {
    current_address = 0;
    ram_select = false;
    rom_select = false;
    io_select = false;
    
    // Default memory map for 4004 system
    ram_start = 0x000;
    ram_end = 0x0FF;   // First 256 addresses for RAM
    rom_start = 0x400;
    rom_end = 0xFFF;   // Addresses 0x400-0xFFF for ROM
    io_start = 0x100;
    io_end = 0x1FF;    // Addresses 0x100-0x1FF for I/O

    // Add the pins for the address decoder
    // Address inputs (12 bits)
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

    // Chip select outputs
    AddSource("RAM_CS");  // RAM chip select
    AddSource("ROM_CS");  // ROM chip select
    AddSource("IO_CS");   // I/O chip select

    // Control signals
    AddSink("CM4");       // Clock input

    in_pins = 0;

    LOG("AddressDecoder4004: Initialized with default 4004 memory map");
}

bool AddressDecoder4004::Tick() {
    // Update address from input pins
    current_address = ((in_pins & (1 << A11)) ? 2048 : 0) |
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

    // Decode address to determine chip selects
    DecodeAddress();
    
    // Update chip select outputs based on decoded address
    UpdateChipSelects();

    // Reset input values for next tick
    in_pins = 0;

    return true;
}

bool AddressDecoder4004::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    union {
        byte tmp[2];
        uint16 tmp16;
    };
    
    if (type == WRITE) {
        switch (conn_id) {
            // Handle chip select outputs
            case RAM_CS:
                tmp[0] = ram_select ? 1 : 0;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);
            case ROM_CS:
                tmp[0] = rom_select ? 1 : 0;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);
            case IO_CS:
                tmp[0] = io_select ? 1 : 0;
                return dest.PutRaw(dest_conn_id, tmp, 0, 1);

            default:
                LOG("AddressDecoder4004::Process: unimplemented connection-id " << conn_id);
                return false;
        }
    }

    return true;
}

bool AddressDecoder4004::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    bool value;
    
    switch (conn_id) {
        // Handle control pins
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
            ASSERT(data_bytes == 0 && data_bits == 1);
            value = (*data & 0x1) != 0;
            SetPin(conn_id, value);
            break;

        default:
            LOG("AddressDecoder4004::PutRaw: error: unsupported conn-id " << conn_id);
            return false;
    }

    return true;
}

void AddressDecoder4004::SetPin(int i, bool b) {
    uint32 mask = 1UL << i;
    if (b)
        in_pins |= mask;
    else
        in_pins &= ~mask;
}

void AddressDecoder4004::DecodeAddress() {
    // Determine which chip should be selected based on current address
    ram_select = (current_address >= ram_start && current_address <= ram_end);
    rom_select = (current_address >= rom_start && current_address <= rom_end);
    io_select = (current_address >= io_start && current_address <= io_end);
    
    // Ensure only one chip is selected at a time (mutual exclusion if overlapping regions)
    // In a real 4004 system, memory regions don't typically overlap
}

void AddressDecoder4004::UpdateChipSelects() {
    // The chip selects are updated in DecodeAddress, but we update change status here
    bool cs_changed = false;
    static bool last_ram_cs = !ram_select;
    static bool last_rom_cs = !rom_select;
    static bool last_io_cs = !io_select;
    
    if (ram_select != last_ram_cs || rom_select != last_rom_cs || io_select != last_io_cs) {
        cs_changed = true;
        last_ram_cs = ram_select;
        last_rom_cs = rom_select;
        last_io_cs = io_select;
    }
    
    SetChanged(cs_changed);
}