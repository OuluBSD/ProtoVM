#ifndef ProtoVM_AddressDecoder4004_h
#define ProtoVM_AddressDecoder4004_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"

/*
 * Address Decoder for Intel 4004 System
 *
 * Handles memory mapping for the 4004 system:
 * - Decodes 12-bit addresses to select appropriate memory/IO chips
 * - Manages chip select signals for ROM and RAM chips
 *
 * In the 4004 system, the address space is segmented:
 * - 0x000-0x0FF: RAM (4002) - 256 addresses
 * - 0x100-0x3FF: Optional additional RAM
 * - 0x400-0xFFF: ROM (4001) - 3072 addresses
 */

class AddressDecoder4004 : public Chip {
public:
    AddressDecoder4004();
    virtual ~AddressDecoder4004() {}

    virtual bool Tick();
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id);
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits);

    virtual const char* GetClassName() const { return "AddressDecoder4004"; }

private:
    uint16 current_address;  // 12-bit address from CPU
    
    // Output control signals
    bool ram_select;         // RAM chip select
    bool rom_select;         // ROM chip select
    bool io_select;          // I/O chip select
    
    // Memory mapping configuration
    uint16 ram_start;        // Start address for RAM
    uint16 ram_end;          // End address for RAM
    uint16 rom_start;        // Start address for ROM
    uint16 rom_end;          // End address for ROM
    uint16 io_start;         // Start address for I/O
    uint16 io_end;           // End address for I/O

    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        A0 = 0,    // Address bits (12 in total)
        A1 = 1,
        A2 = 2,
        A3 = 3,
        A4 = 4,
        A5 = 5,
        A6 = 6,
        A7 = 7,
        A8 = 8,
        A9 = 9,
        A10 = 10,
        A11 = 11,
        RAM_CS = 12,  // RAM chip select output
        ROM_CS = 13,  // ROM chip select output
        IO_CS = 14,   // I/O chip select output
        CM4 = 15      // Clock input
    };

    // Internal state for current tick
    uint32 in_pins;
    
    void SetPin(int i, bool b);
    void DecodeAddress();
    void UpdateChipSelects();
    
public:
    // Set memory map regions
    void SetRAMRegion(uint16 start, uint16 end) { ram_start = start; ram_end = end; }
    void SetROMRegion(uint16 start, uint16 end) { rom_start = start; rom_end = end; }
    void SetIORegion(uint16 start, uint16 end) { io_start = start; io_end = end; }
};

#endif