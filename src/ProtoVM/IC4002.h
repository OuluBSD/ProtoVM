#ifndef ProtoVM_IC4002_h
#define ProtoVM_IC4002_h

#include "Component.h"
#include "ICs.h"
#include "Common.h"
#include "Bus.h"

/*
 * Intel 4002 RAM Implementation for ProtoVM
 *
 * The Intel 4002 is a 40-bit (40x1) static RAM chip with:
 * - 40 memory locations of 1 bit each
 * - 4 outputs (4 banks of 10 bits each)
 * - Separate input and output pins
 * - Character generator RAM in original implementation
 *
 * Pinout:
 * - A0-A3: Address inputs (4-bit address for 10 positions per bank)
 * - C0-C3: Bank select inputs (4 banks)
 * - I0-I3: Input data bits (4-bit input)
 * - O0-O3: Output data bits (4-bit output)
 * - WM: Write Memory enable
 * - CM4: Clock input
 */

class IC4002 : public Chip {
public:
    IC4002();
    virtual ~IC4002() {}

    virtual bool Tick();
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id);
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits);

    virtual const char* GetClassName() const { return "IC4002"; }

private:
    // 4002 has 40 memory bits organized as 4 banks of 10 bits each
    byte memory[4][10];  // 4 banks of 10 bits each = 40 bits total
    
    // Current address and bank
    byte address;        // 0-9 address within bank
    byte bank;           // 0-3 bank selection
    bool write_mode;     // Whether in write mode
    
    // Input and output data
    byte input_data;     // 4 bit input data
    byte output_data;    // 4 bit output data
    
    // Pin mappings (relative to AddPin calls)
    enum PinNames {
        A0 = 0,    // Address bits
        A1 = 1,
        A2 = 2,
        A3 = 3,
        C0 = 4,    // Bank selection
        C1 = 5,
        C2 = 6,
        C3 = 7,
        I0 = 8,    // Input data
        I1 = 9, 
        I2 = 10,
        I3 = 11,
        O0 = 12,   // Output data
        O1 = 13,
        O2 = 14,
        O3 = 15,
        WM = 16,   // Write Memory enable
        CM4 = 17   // Clock input
    };

    // Internal state for current tick
    uint32 in_pins;
    byte in_data;
    
    void SetPin(int i, bool b);
    void ReadMemory();
    void WriteMemory();
    void UpdateOutput();
};

#endif