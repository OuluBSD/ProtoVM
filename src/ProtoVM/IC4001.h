#ifndef _ProtoVM_IC4001_h_
#define _ProtoVM_IC4001_h_

#include <algorithm> // for std::min

/*
 * Intel 4001 4-bit ROM
 * 
 * The Intel 4001 is a 4-bit ROM chip with:
 * - 4-bit data output
 * - 10-bit address input (1024 x 4-bit = 4096 bits total storage)
 * - Output enable control
 */

class IC4001 : public Chip {
private:
    static constexpr int MAX_SIZE = 1024; // 1024 x 4-bit = 4096 bits storage
    static constexpr int A0 = 0;          // Address pins (10 bits)
    static constexpr int A1 = 1;
    static constexpr int A2 = 2;
    static constexpr int A3 = 3;
    static constexpr int A4 = 4;
    static constexpr int A5 = 5;
    static constexpr int A6 = 6;
    static constexpr int A7 = 7;
    static constexpr int A8 = 8;
    static constexpr int A9 = 9;
    static constexpr int D0 = 10;         // Bidirectional data pins (4 bits)
    static constexpr int D1 = 11;
    static constexpr int D2 = 12;
    static constexpr int D3 = 13;
    static constexpr int CS = 14;         // Chip Select (active low)
    static constexpr int OE = 15;         // Output Enable (active low)

    // Memory storage
    byte memory[MAX_SIZE];  // 4-bit values stored in 8-bit bytes (upper 4 bits unused)

    // Control signals
    bool chip_select;
    bool output_enable;
    uint16_t address;
    byte data_out;

    // Input values
    uint16_t in_addr;
    bool in_cs;
    bool in_oe;

public:
    IC4001(int size = MAX_SIZE);

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bits, int data_bytes) override;

    // Helper methods
    void WriteMemory(uint16_t addr, byte value);
    byte ReadMemory(uint16_t addr) const;
    void SetMemory(uint16_t addr, byte value) { WriteMemory(addr, value); }
    byte GetMemory(uint16_t addr) const { return ReadMemory(addr); }
};

#endif