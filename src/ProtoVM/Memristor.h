#ifndef _ProtoVM_Memristor_h_
#define _ProtoVM_Memristor_h_

#include "ProtoVM.h"

// Memristor component - the fourth fundamental circuit element
// Resistance depends on the history of charge that has flowed through it
class Memristor : public ElcBase {
private:
    double min_resistance;            // Minimum resistance value (Ohms)
    double max_resistance;            // Maximum resistance value (Ohms)
    double current_resistance;        // Current resistance value (Ohms)
    double state_variable;            // Internal state variable (0.0 to 1.0, representing memristor state)
    double memristance_coefficient;   // Coefficient affecting how quickly state changes
    double charge_history;            // Tracks the charge that has flowed through the device
    double delta_flux;                // Change in flux during this tick
    double delta_charge;              // Change in charge during this tick
    bool terminal_A_state;            // Current state of terminal A
    bool terminal_B_state;            // Current state of terminal B

public:
    Memristor(double min_resistance = 100.0,      // Minimum resistance
              double max_resistance = 10000.0,    // Maximum resistance
              double initial_state = 0.5,         // Initial state (0.0 to 1.0)
              double memristance_coefficient = 0.1); // Coefficient affecting state change speed
    
    void SetMinResistance(double r);
    double GetMinResistance() const { return min_resistance; }
    
    void SetMaxResistance(double r);
    double GetMaxResistance() const { return max_resistance; }
    
    void SetMemristanceCoefficient(double coeff);
    double GetMemristanceCoefficient() const { return memristance_coefficient; }
    
    double GetCurrentResistance() const { return current_resistance; }
    double GetStateVariable() const { return state_variable; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif