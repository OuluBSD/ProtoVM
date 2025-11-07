#ifndef _ProtoVM_PLL_h_
#define _ProtoVM_PLL_h_

#include "ProtoVM.h"

// PLL (Phase-Locked Loop) - Simulates a phase-locked loop for frequency synthesis
// Can generate an output clock with a frequency that's a multiple of the input frequency
class PLL : public ElcBase {
private:
    // Configuration
    int multiplication_factor = 4;  // Default to 4x frequency multiplication
    int input_frequency = 1000;     // Input frequency in Hz (for reference)
    int output_frequency = 4000;    // Output frequency in Hz (for reference)
    
    // Internal state
    bool input_clock = false;       // Current input clock state
    bool output_clock = false;      // Current output clock state
    int output_counter = 0;         // Counter for output clock generation
    int max_output_count = 2;       // When counter reaches this, toggle output
    bool locked = false;            // Whether PLL is currently locked
    int lock_threshold = 10;        // Cycles needed to achieve lock

public:
    PLL(int mult_factor = 4);  // Default to 4x multiplication
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // Configuration methods
    void SetMultiplicationFactor(int factor);
    int GetMultiplicationFactor() const { return multiplication_factor; }
    
    // Status methods
    bool IsLocked() const { return locked; }
    bool GetOutputClock() const { return output_clock; }
    int GetOutputCounter() const { return output_counter; }
};

#endif