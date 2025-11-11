#ifndef _ProtoVM_4004BusController_h_
#define _ProtoVM_4004BusController_h_

/*
 * Bus Controller for Intel 4004 System
 * 
 * Manages the shared data bus between CPU, ROM, and RAM
 * Implements proper bus arbitration and tri-state control
 */

class BusController4004 : public Chip {
public:
    // Bus pin connections
    static constexpr int CPU_D0 = 0;   // CPU data pins (4 bits)
    static constexpr int CPU_D1 = 1;
    static constexpr int CPU_D2 = 2;
    static constexpr int CPU_D3 = 3;
    static constexpr int ROM_D0 = 4;   // ROM data pins (4 bits)
    static constexpr int ROM_D1 = 5;
    static constexpr int ROM_D2 = 6;
    static constexpr int ROM_D3 = 7;
    static constexpr int RAM_D0_IN = 8;  // RAM data input pins (4 bits)
    static constexpr int RAM_D1_IN = 9;
    static constexpr int RAM_D2_IN = 10;
    static constexpr int RAM_D3_IN = 11;
    static constexpr int RAM_D0_OUT = 12; // RAM data output pins (4 bits)
    static constexpr int RAM_D1_OUT = 13;
    static constexpr int RAM_D2_OUT = 14;
    static constexpr int RAM_D3_OUT = 15;
    static constexpr int CPU_CLK = 16;    // CPU clock
    static constexpr int MEM_CLK = 17;    // Memory clock
    static constexpr int CPU_RW = 18;     // CPU read/write signal
    static constexpr int CPU_MR = 19;     // CPU memory read signal
    static constexpr int CPU_MW = 20;     // CPU memory write signal
    
    // Internal state
    byte cpu_data;      // Data from CPU
    byte rom_data;      // Data from ROM
    byte ram_data_in;   // Data to RAM
    byte ram_data_out;  // Data from RAM
    bool cpu_rw;        // CPU read/write control
    bool cpu_mr;        // CPU memory read
    bool cpu_mw;        // CPU memory write
    bool cpu_clock;
    bool mem_clock;
    
    // Input values
    byte in_cpu_data;
    byte in_rom_data;
    byte in_ram_data_in;
    byte in_ram_data_out;
    bool in_cpu_rw;
    bool in_cpu_mr;
    bool in_cpu_mw;
    bool in_cpu_clk;
    bool in_mem_clk;

public:
    BusController4004();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    // Helper methods
    void SetPinState(int pin, bool state, byte value = 0);
    byte GetCpuData() const { return cpu_data; }
    byte GetRomData() const { return rom_data; }
    byte GetRamDataIn() const { return ram_data_in; }
    byte GetRamDataOut() const { return ram_data_out; }
};

#endif