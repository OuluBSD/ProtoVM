#ifndef _ProtoVM_Potentiometer_h_
#define _ProtoVM_Potentiometer_h_

#include "ProtoVM.h"

// Potentiometer component - a variable resistor with three terminals
// Terminal A: one end of the resistive element
// Terminal B: other end of the resistive element  
// Terminal W: wiper that moves along the resistive element
// The position of the wiper determines the resistance between A-W and B-W
class Potentiometer : public ElcBase {
private:
    double total_resistance;      // Total resistance of the potentiometer (Ohms)
    double wiper_position;        // Position of wiper (0.0 to 1.0, where 0.0 is A, 1.0 is B)
    bool terminal_A_state;        // Current state of terminal A
    bool terminal_B_state;        // Current state of terminal B
    bool terminal_W_state;        // Current state of wiper terminal W

public:
    Potentiometer(double resistance = 10000.0, double initial_position = 0.5);  // Default 10kΩ, half position
    
    void SetResistance(double r);
    double GetResistance() const { return total_resistance; }
    
    void SetWiperPosition(double pos);  // Position from 0.0 to 1.0
    double GetWiperPosition() const { return wiper_position; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif