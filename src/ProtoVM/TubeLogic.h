#ifndef _ProtoVM_TubeLogic_h_
#define _ProtoVM_TubeLogic_h_

#include "Component.h"
#include "Common.h"

// Triode tube model for basic logic operations
// A triode consists of Cathode (K), Grid (G), and Plate (Anode - A)
// When grid is negative relative to cathode, it blocks current flow
// When grid is positive, current flows from cathode to plate
class TubeTriode : public ElcBase {
public:
    TubeTriode();
    virtual ~TubeTriode() {}

    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool cathode = 0;    // Cathode voltage (0 = off, 1 = on)
    bool grid = 0;       // Grid voltage (0 = blocking, 1 = allowing current)
    bool plate = 0;      // Plate voltage (output when current flows)
    
    // Tube state
    bool current_flow = 0;  // Whether current flows from cathode to plate (1) or not (0)
};

// NOT gate using triode tube
// When input (grid) is 0, no current flows, output (plate) is 1 (high)
// When input (grid) is 1, current flows, output (plate) is 0 (low)
class TubeNot : public TubeTriode {
public:
    TubeNot();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool in = 0;    // Input connected to grid
    bool out = 1;   // Output from plate (inverted)
};

// NAND gate using two triodes in series
// Both inputs must be high for output to be low
class TubeNand : public ElcBase {
public:
    TubeNand();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool in0 = 0;
    bool in1 = 0;
    bool out = 1;
};

// AND gate using a NAND gate followed by a NOT gate
class TubeAnd : public ElcBase {
public:
    TubeAnd();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool in0 = 0;
    bool in1 = 0;
    bool out = 0;
    
    // Internal components
    TubeNand nand_gate;
    TubeNot not_gate;
};

// OR gate using triode parallel configuration
class TubeOr : public ElcBase {
public:
    TubeOr();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool in0 = 0;
    bool in1 = 0;
    bool out = 0;
};

// NOR gate using OR gate followed by NOT gate
class TubeNor : public ElcBase {
public:
    TubeNor();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool in0 = 0;
    bool in1 = 0;
    bool out = 1;
    
    // Internal components
    TubeOr or_gate;
    TubeNot not_gate;
};

// Tube-based SR Latch (Set-Reset Latch using cross-coupled triodes)
class TubeSRLatch : public ElcBase {
public:
    TubeSRLatch();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool s = 0;          // Set input
    bool r = 0;          // Reset input
    bool q = 0;          // Output
    bool qn = 1;         // Inverted output
};

// Tube-based D Flip-Flop using SR Latch with clocking
class TubeDFlipFlop : public ElcBase {
public:
    TubeDFlipFlop();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool d = 0;          // Data input
    bool clk = 0;        // Clock input
    bool q = 0;          // Output
    bool qn = 1;         // Inverted output
    bool en = 1;         // Enable input (active high)
    bool clr = 0;        // Clear input (active high)
    bool last_clk = 0;   // Previous clock state for edge detection
};

// Tube-based JK Flip-Flop
class TubeJKFlipFlop : public ElcBase {
public:
    TubeJKFlipFlop();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool j = 0;          // J input
    bool k = 0;          // K input
    bool clk = 0;        // Clock input
    bool q = 0;          // Output
    bool qn = 1;         // Inverted output
    bool en = 1;         // Enable input (active high)
    bool clr = 0;        // Clear input (active high)
    bool last_clk = 0;   // Previous clock state for edge detection
};

// Tube-based 4-bit register using D flip-flops
class TubeRegister4Bit : public ElcBase {
public:
    TubeRegister4Bit();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool d[4] = {0, 0, 0, 0};   // 4-bit data input
    bool clk = 0;               // Clock input
    bool en = 1;                // Enable input (active high)
    bool clr = 0;               // Clear input (active high)
    bool q[4] = {0, 0, 0, 0};   // 4-bit output
    bool last_clk = 0;          // Previous clock state for edge detection
};

// Tube-based counter using D flip-flops
class TubeCounter4Bit : public ElcBase {
public:
    TubeCounter4Bit();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool clk = 0;               // Clock input
    bool en = 1;                // Enable input (active high)
    bool clr = 0;               // Clear input (active high)
    bool load = 0;              // Load input (active high)
    bool d[4] = {0, 0, 0, 0};   // 4-bit parallel load data
    bool q[4] = {0, 0, 0, 0};   // 4-bit output (count value)
    bool last_clk = 0;          // Previous clock state for edge detection
    int count = 0;              // Internal count value
};

// Tube-based binary counter that increments on each clock pulse
class TubeBinaryCounter4Bit : public ElcBase {
public:
    TubeBinaryCounter4Bit();
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

private:
    bool clk = 0;               // Clock input
    bool en = 1;                // Enable input (active high)
    bool clr = 0;               // Clear input (active high)
    bool q[4] = {0, 0, 0, 0};   // 4-bit output (count value)
    bool last_clk = 0;          // Previous clock state for edge detection
    int count = 0;              // Internal count value
};

#endif