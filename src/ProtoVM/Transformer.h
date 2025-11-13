#ifndef _ProtoVM_Transformer_h_
#define _ProtoVM_Transformer_h_

#include "ProtoVM.h"

// Transformer component with primary and secondary windings
// Models ideal transformer behavior with turns ratio and coupling coefficient
class Transformer : public ElcBase {
private:
    double turns_ratio;          // N_secondary / N_primary (turns ratio)
    double coupling_coefficient; // Coupling coefficient (0.0 to 1.0, where 1.0 is perfect coupling)
    double primary_voltage;      // Voltage at primary winding
    double secondary_voltage;    // Voltage at secondary winding (calculated from primary * turns_ratio)
    bool primary_in_state;       // Input state at primary side
    bool primary_out_state;      // Output state at primary side
    bool secondary_in_state;     // Input state at secondary side
    bool secondary_out_state;    // Output state at secondary side

public:
    Transformer(double turns_ratio = 1.0, double coupling_coefficient = 0.99);
    
    void SetTurnsRatio(double ratio);
    double GetTurnsRatio() const { return turns_ratio; }
    
    void SetCouplingCoefficient(double coeff);
    double GetCouplingCoefficient() const { return coupling_coefficient; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif