#ifndef _ProtoVM_Relay_h_
#define _ProtoVM_Relay_h_

#include "ProtoVM.h"

// Relay component with electromagnetic coil and switching contacts
// When coil is energized, contacts change state
class Relay : public ElcBase {
private:
    double coil_inductance;          // Inductance of the relay coil (Henries)
    double coil_resistance;          // Resistance of the relay coil (Ohms)
    bool coil_energized;             // Whether the coil is currently energized
    bool contacts_closed;            // Whether the main contacts are closed
    int num_poles;                   // Number of poles (single pole, double pole, etc.)
    int num_throws;                  // Number of throws per pole (single throw, double throw, etc.)
    Vector<Vector<bool>> contact_states; // States of each contact (closed/open for each pole)
    double activation_threshold;     // Threshold current/voltage to activate relay
    double release_threshold;        // Threshold to release relay
    int activation_delay;           // Delay in ticks from activation command to contact closure
    int release_delay;              // Delay in ticks from deactivation command to contact opening
    int current_delay;              // Current delay counter

public:
    Relay(double coil_inductance = 0.1, double coil_resistance = 10.0, 
          int num_poles = 1, int num_throws = 2, 
          double activation_threshold = 0.5, double release_threshold = 0.3);
    
    void SetCoilInductance(double L);
    double GetCoilInductance() const { return coil_inductance; }
    
    void SetCoilResistance(double R);
    double GetCoilResistance() const { return coil_resistance; }
    
    bool IsCoilEnergized() const { return coil_energized; }
    bool AreContactsClosed() const { return contacts_closed; }
    
    int GetNumPoles() const { return num_poles; }
    int GetNumThrows() const { return num_throws; }
    
    bool GetContactState(int pole, int throw_index) const;
    void SetContactState(int pole, int throw_index, bool state);
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif