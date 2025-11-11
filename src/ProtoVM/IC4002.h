#ifndef _ProtoVM_IC4002_h_
#define _ProtoVM_IC4002_h_

#include <algorithm> // for std::min

/*
 * Intel 4002 4-bit Static RAM
 * 
 * The Intel 4002 is a 4-bit static RAM chip with:
 * - 4-bit data input/output
 * - 4-bit address input (16 x 4-bit = 64 bits total storage)
 * - Separate input/output pins for data
 * - Chip select and write enable control
 */

class IC4002 : public Chip {
private:
    static constexpr int MAX_SIZE = 16; // 16 x 4-bit = 64 bits storage
    static constexpr int A0 = 0;        // Address pins (4 bits)
    static constexpr int A1 = 1;
    static constexpr int A2 = 2;
    static constexpr int A3 = 3;
    static constexpr int D0 = 4;        // Bidirectional data pins (4 bits)
    static constexpr int D1 = 5;
    static constexpr int D2 = 6;
    static constexpr int D3 = 7;
    static constexpr int CS = 8;        // Chip Select (active low)
    static constexpr int WE = 9;        // Write Enable (active high)

    // Memory storage
    byte memory[MAX_SIZE];  // 4-bit values stored in 8-bit bytes (upper 4 bits unused)

    // Control signals
    bool chip_select;
    bool write_enable;
    uint8_t address;
    byte data_in;
    byte data_out;

    // Input values
    uint8_t in_addr;
    byte in_data;
    bool in_cs;
    bool in_we;

public:
    IC4002(int size = MAX_SIZE);

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    String GetClassName() const override { return "IC4002"; }

    // Helper methods
    void WriteMemory(uint8_t addr, byte value);
    byte ReadMemory(uint8_t addr) const;
    void SetMemory(uint8_t addr, byte value) { WriteMemory(addr, value); }
    byte GetMemory(uint8_t addr) const { return ReadMemory(addr); }
};

#endif
