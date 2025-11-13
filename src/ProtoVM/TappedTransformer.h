#ifndef _ProtoVM_TappedTransformer_h_
#define _ProtoVM_TappedTransformer_h_

#include "ProtoVM.h"

// Tapped Transformer component with primary winding and multiple secondary taps
// Can be used for center-tapped transformers or transformers with multiple taps
class TappedTransformer : public ElcBase {
private:
    double primary_turns;              // Number of turns on primary winding
    Vector<double> secondary_turns;    // Number of turns for each secondary tap
    double coupling_coefficient;       // Coupling coefficient (0.0 to 1.0)
    int num_taps;                      // Number of secondary taps
    Vector<double> tap_voltages;       // Calculated voltages at each tap
    Vector<bool> tap_states;           // Current states of each tap
    bool primary_in_state;             // Input state at primary side
    bool primary_out_state;            // Output state at primary side

public:
    // Constructor with number of taps and an array of secondary turns for each tap
    TappedTransformer(double primary_turns = 100, const Vector<double>& secondary_turns = Vector<double>(), 
                      double coupling_coefficient = 0.99);
    
    void SetPrimaryTurns(double turns);
    double GetPrimaryTurns() const { return primary_turns; }
    
    void SetSecondaryTurns(int tap_index, double turns);
    double GetSecondaryTurns(int tap_index) const;
    
    void SetCouplingCoefficient(double coeff);
    double GetCouplingCoefficient() const { return coupling_coefficient; }
    
    int GetNumTaps() const { return num_taps; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif