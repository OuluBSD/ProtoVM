#ifndef _ProtoVM_StandardLibrary_h_
#define _ProtoVM_StandardLibrary_h_

#include "ProtoVM.h"

// Standard library of commonly used digital components
// This provides a collection of pre-built components for rapid circuit design

// Simple logic gates with multiple inputs
class AndGate3 : public ElcBase {
    bool in[3] = {0, 0, 0};
    bool out = 0;

public:
    AndGate3();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class AndGate4 : public ElcBase {
    bool in[4] = {0, 0, 0, 0};
    bool out = 0;

public:
    AndGate4();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class OrGate3 : public ElcBase {
    bool in[3] = {0, 0, 0};
    bool out = 0;

public:
    OrGate3();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class OrGate4 : public ElcBase {
    bool in[4] = {0, 0, 0, 0};
    bool out = 0;

public:
    OrGate4();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class Buffer : public ElcBase {
    bool in = 0;
    bool out = 0;

public:
    Buffer();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class TriStateBuffer : public ElcBase {
    bool input = 0;
    bool enable = 0;
    bool output = 0;

public:
    TriStateBuffer();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// 8-bit versions of common components
class Register8Bit : public ElcBase {
    bool d[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8-bit data input
    bool clk = 0;                           // Clock input
    bool en = 1;                            // Enable input (active high)
    bool clr = 0;                           // Clear input (active high)
    bool q[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8-bit output
    bool last_clk = 0;                      // Previous clock state for edge detection

public:
    Register8Bit();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class FlipFlopD8Bit : public ElcBase {
    bool d[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8-bit data input
    bool clk = 0;                           // Clock input
    bool q[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8-bit output
    bool qn[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // 8-bit inverted output
    bool en = 1;                            // Enable input (active high)
    bool clr = 0;                           // Clear input (active high)
    bool last_clk = 0;                      // Previous clock state for edge detection

public:
    FlipFlopD8Bit();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// N-bit versions of common components
template<int N>
class RegisterNBit : public ElcBase {
    bool d[N];      // N-bit data input
    bool clk = 0;   // Clock input
    bool en = 1;    // Enable input (active high)
    bool clr = 0;   // Clear input (active high)
    bool q[N];      // N-bit output
    bool last_clk = 0;  // Previous clock state for edge detection

public:
    RegisterNBit() {
        // Initialize arrays
        for (int i = 0; i < N; i++) {
            d[i] = 0;
            q[i] = 0;
        }

        // Add sink connections for N-bit data input (D0 to D{N-1})
        for (int i = 0; i < N; i++) {
            AddSink(String().Cat() << "D" << (N-1-i));  // D7, D6, ..., D0 for an 8-bit register
        }

        // Add sink connections for control signals
        AddSink("Ck");   // Clock
        AddSink("En");   // Enable
        AddSink("Clr");  // Clear

        // Add source connections for N-bit output (Q0 to Q{N-1})
        for (int i = 0; i < N; i++) {
            AddSource(String().Cat() << "Q" << (N-1-i)).SetMultiConn();  // Q7, Q6, ..., Q0 for an 8-bit register
        }
    }

    bool Tick() override {
        // Check for clear condition - if clear is active, reset all outputs regardless of clock
        if (clr) {
            for (int i = 0; i < N; i++) {
                q[i] = 0;
            }
        } else {
            // Detect rising edge of clock
            bool rising_edge = (clk && !last_clk);

            // On rising edge, and if enabled, update all output bits with input data
            if (rising_edge && en) {
                for (int i = 0; i < N; i++) {
                    q[i] = d[i];
                }
            }
        }

        // Store current clock state for next rising edge detection
        last_clk = clk;

        return true;
    }

    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE) {
            // Handle data input connections (0 to N-1)
            if (conn_id >= 0 && conn_id < N) {
                // Skip data input connections - handled by PutRaw
            }
            // Handle control signal connections (N to N+2)
            else if (conn_id >= N && conn_id <= N+2) {
                // Skip control connections - handled by PutRaw
            }
            // Handle output connections (N+3 to N+2+N)
            else if (conn_id >= N+3 && conn_id < N+3+N) {
                int output_idx = conn_id - (N+3);
                if (output_idx < N) {
                    return dest.PutRaw(dest_conn_id, (byte*)&q[output_idx], 0, 1);
                }
            }
        }
        return true;
    }

    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        // Handle data input connections (0 to N-1)
        if (conn_id >= 0 && conn_id < N) {
            ASSERT(data_bytes == 0 && data_bits == 1);
            int input_idx = conn_id;
            d[input_idx] = *data & 1;
        }
        // Handle control signals
        else if (conn_id == N) {      // Clock
            ASSERT(data_bytes == 0 && data_bits == 1);
            clk = *data & 1;
        }
        else if (conn_id == N+1) {    // Enable
            ASSERT(data_bytes == 0 && data_bits == 1);
            en = *data & 1;
        }
        else if (conn_id == N+2) {    // Clear
            ASSERT(data_bytes == 0 && data_bits == 1);
            clr = *data & 1;
        }
        else {
            LOG("error: RegisterNBit: unimplemented conn-id " << conn_id);
            return false;
        }
        return true;
    }
};

// Common counter implementations
class Counter4Bit : public ElcBase {
    int count = 0;      // Current count value (0-15 for 4-bit)
    bool clk = 0;       // Clock input
    bool en = 1;        // Enable input (active high)
    bool clr = 0;       // Clear input (active high)
    bool load = 0;      // Load input (active high)
    bool d[4] = {0, 0, 0, 0};  // 4-bit parallel load data
    bool q[4] = {0, 0, 0, 0};  // 4-bit output
    bool last_clk = 0;  // Previous clock state for edge detection
    bool carry_out = 0; // Carry out for cascading

public:
    Counter4Bit();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class Counter8Bit : public ElcBase {
    int count = 0;      // Current count value (0-255 for 8-bit)
    bool clk = 0;       // Clock input
    bool en = 1;        // Enable input (active high)
    bool clr = 0;       // Clear input (active high)
    bool load = 0;      // Load input (active high)
    bool d[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8-bit parallel load data
    bool q[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8-bit output
    bool last_clk = 0;  // Previous clock state for edge detection
    bool carry_out = 0; // Carry out for cascading

public:
    Counter8Bit();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Memory components
class RAM16x8 : public ElcBase {
    byte memory[16];    // 16 locations of 8 bits each
    bool addr[4] = {0, 0, 0, 0};  // 4-bit address (for 16 locations)
    bool din[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // 8-bit data input
    bool dout[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // 8-bit data output
    bool we = 0;        // Write enable (active high)
    bool oe = 1;        // Output enable (active high)
    bool cs = 1;        // Chip select (active high)

public:
    RAM16x8();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

class ROM16x8 : public ElcBase {
    byte memory[16];    // 16 locations of 8 bits each
    bool addr[4] = {0, 0, 0, 0};  // 4-bit address (for 16 locations)
    bool dout[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // 8-bit data output
    bool oe = 1;        // Output enable (active high)
    bool cs = 1;        // Chip select (active high)

public:
    ROM16x8(const byte init_data[16] = nullptr);  // Initialize with optional initial data

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

// Common interface components
class ShiftRegister8Bit : public ElcBase {
    bool data_in = 0;       // Serial data input
    bool clk = 0;           // Clock input
    bool clr = 0;           // Clear input
    bool mode = 0;          // Mode: 0=shift, 1=load
    bool ser_load = 0;      // Serial/parallel load control
    bool d[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Parallel data input
    bool q[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Parallel output
    bool q7_serial = 0;     // Serial output (from the last stage)
    bool last_clk = 0;      // Previous clock state for edge detection

public:
    ShiftRegister8Bit();

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif