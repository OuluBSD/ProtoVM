#ifndef _ProtoVM_BusInterface4004_h_
#define _ProtoVM_BusInterface4004_h_

/*
 * Bus Interface for Intel 4004 System
 *
 * Handles data bus conversion between 4-bit CPU bus and 8-bit memory buses:
 * - Converts 4-bit data from CPU to 8-bit memory interface
 * - Combines two 4-bit memory values into an 8-bit value when reading
 * - Splits 8-bit memory values into two 4-bit values when writing
 */

class BusInterface4004 : public Chip {
    // Pin connections
    static constexpr int CPU_D0 = 0;   // CPU data bus (4 bits)
    static constexpr int CPU_D1 = 1;
    static constexpr int CPU_D2 = 2;
    static constexpr int CPU_D3 = 3;
    static constexpr int MEM_D0 = 4;   // Memory data bus (8 bits)
    static constexpr int MEM_D1 = 5;
    static constexpr int MEM_D2 = 6;
    static constexpr int MEM_D3 = 7;
    static constexpr int MEM_D4 = 8;
    static constexpr int MEM_D5 = 9;
    static constexpr int MEM_D6 = 10;
    static constexpr int MEM_D7 = 11;
    static constexpr int R_W = 12;     // Read/Write control
    static constexpr int CPU_CLK = 13; // CPU clock
    static constexpr int MEM_CLK = 14; // Memory clock

    // Internal state
    byte cpu_data;      // 4-bit data from or to CPU
    byte mem_data_low;  // 8-bit data from or to memory (low byte)
    byte mem_data_high; // 8-bit data from or to memory (high byte)
    bool is_reading;    // Current operation is read
    bool is_writing;    // Current operation is write
    bool cpu_clock;     // CPU clock state
    bool mem_clock;     // Memory clock state

    // Input values
    byte in_cpu_data;
    byte in_mem_data;
    bool in_r_w;
    bool in_cpu_clk;
    bool in_mem_clk;

public:
    BusInterface4004();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    // Helper functions
    void SetPinState(int pin, bool state, byte value = 0);
    byte GetCpuData() const { return cpu_data; }
    byte GetMemDataLow() const { return mem_data_low; }
    byte GetMemDataHigh() const { return mem_data_high; }
};

#endif