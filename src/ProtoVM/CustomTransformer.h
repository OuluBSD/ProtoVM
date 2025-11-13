#ifndef _ProtoVM_CustomTransformer_h_
#define _ProtoVM_CustomTransformer_h_

#include "ProtoVM.h"

// Custom Transformer component with fully configurable parameters
// Allows configuration of inductances, coupling, and frequency characteristics
class CustomTransformer : public ElcBase {
private:
    double primary_inductance;        // Primary winding inductance (Henries)
    double secondary_inductance;      // Secondary winding inductance (Henries)
    double mutual_inductance;         // Mutual inductance between windings (Henries)
    double coupling_coefficient;      // Coupling coefficient (0.0 to 1.0)
    double primary_resistance;        // Primary winding resistance (Ohms)
    double secondary_resistance;      // Secondary winding resistance (Ohms)
    double primary_capacitance;       // Primary-to-core capacitance (Farads)
    double secondary_capacitance;     // Secondary-to-core capacitance (Farads)
    double frequency;                 // Operating frequency (Hz) - affects reactance
    
    // Internal states
    double primary_current;           // Current in primary winding
    double secondary_current;         // Current in secondary winding
    bool primary_in_state;            // Input state at primary side
    bool primary_out_state;           // Output state at primary side
    bool secondary_in_state;          // Input state at secondary side
    bool secondary_out_state;         // Output state at secondary side

public:
    CustomTransformer(double primary_inductance = 0.1, double secondary_inductance = 0.1, 
                      double coupling_coefficient = 0.99, double primary_resistance = 1.0,
                      double secondary_resistance = 1.0, double frequency = 60.0);
    
    void SetPrimaryInductance(double L);
    double GetPrimaryInductance() const { return primary_inductance; }
    
    void SetSecondaryInductance(double L);
    double GetSecondaryInductance() const { return secondary_inductance; }
    
    void SetMutualInductance(double M);
    double GetMutualInductance() const { return mutual_inductance; }
    
    void SetCouplingCoefficient(double coeff);
    double GetCouplingCoefficient() const { return coupling_coefficient; }
    
    void SetPrimaryResistance(double R);
    double GetPrimaryResistance() const { return primary_resistance; }
    
    void SetSecondaryResistance(double R);
    double GetSecondaryResistance() const { return secondary_resistance; }
    
    void SetFrequency(double freq);
    double GetFrequency() const { return frequency; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif