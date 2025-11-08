#ifndef ProtoVM_IC4001_h
#define ProtoVM_IC4001_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"

/*
 * Intel 4001 ROM Implementation for ProtoVM
 *
 * The Intel 4001 is a 2048-bit (256x8) read-only memory chip used with the 4004 CPU.
 * In the 4004 system, ROMs are used for program storage.
 *
 * Pinout:
 * - A0-A7: Address inputs (8-bit address for 256 locations)
 * - A8-A9: Additional address inputs (10-bit total for 1024 locations in some configurations)
 * - O0-O3: Output data bits (4-bit output - in 4004 system, data is 4 bits)
 * - CM: Clock output (to synchronize with CPU)
 * - CM4: Clock input (from CPU)
 * - JAM: Chip enable/disable
 */

class IC4001 : public Chip {
public:
    IC4001();
    virtual ~IC4001() {}

    virtual bool Tick();
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id);
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits);

    virtual const char* GetClassName() const { return "IC4001"; }
    
    // Method to load program data into ROM
    void LoadProgram(const byte* data, int size);
    void SetMemory(int addr, byte value);
    byte GetMemory(int addr) const;

private:
    // 4001 has 256 words of 4-bits each (emulating 2048x1 configuration)
    // For 4004 compatibility, we'll use 4096 4-bit words (as 4004 has 12-bit address space)
    byte memory[4096];  // 4096 x 4-bit locations = 2048 bytes of 4-bit data
    
    // Current address
    uint16 address;     // 12-bit address (0-4095)
    
    // Output data
    byte output_data;   // 4 bit output data
    
    // Status
    bool enabled;       // Whether chip is enabled
    
    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        A0 = 0,    // Address bits (12 in total for 4004 compatibility)
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
        O0 = 12,   // Output data
        O1 = 13,
        O2 = 14,
        O3 = 15,
        CM = 16,   // Clock output to CPU
        CM4 = 17,  // Clock input from CPU
        JAM = 18   // Chip enable
    };

    // Internal state for current tick
    uint32 in_pins;
    
    void SetPin(int i, bool b);
    void ReadMemory();
    void UpdateOutput();
};

#endif