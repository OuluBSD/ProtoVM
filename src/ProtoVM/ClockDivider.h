#ifndef _ProtoVM_ClockDivider_h_
#define _ProtoVM_ClockDivider_h_

#include "ProtoVM.h"

// ClockDivider - Generates a slower clock from a faster input clock
// The output clock frequency is input frequency divided by a specified factor
class ClockDivider : public ElcBase {
private:
    // Configuration
    int division_factor = 2;    // Default to divide by 2 (50% duty cycle)
    int counter = 0;            // Current count toward next division
    bool output_clock = false;  // Current output clock state
    bool input_clock = false;   // Current input clock state
    bool last_input_clock = false;  // Previous input clock state (for edge detection)

public:
    ClockDivider(int factor = 2);  // Default to divide-by-2
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    
    // Configuration methods
    void SetDivisionFactor(int factor);
    int GetDivisionFactor() const { return division_factor; }
    
    // State methods
    bool GetOutputClock() const { return output_clock; }
    int GetCounter() const { return counter; }
};

#endif