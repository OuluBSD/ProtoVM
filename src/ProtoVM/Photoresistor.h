#ifndef _ProtoVM_Photoresistor_h_
#define _ProtoVM_Photoresistor_h_

#include "ProtoVM.h"

// Photoresistor (LDR - Light Dependent Resistor) component
// Resistance varies based on incident light intensity
class Photoresistor : public ElcBase {
private:
    double base_resistance;           // Resistance in dark conditions (Ohms)
    double min_resistance;            // Minimum resistance in bright light (Ohms)
    double light_sensitivity;         // Sensitivity factor to light changes
    double current_resistance;        // Current resistance based on light level
    double light_level;               // Current light intensity (0.0 to 1.0)
    bool terminal_A_state;            // Current state of terminal A
    bool terminal_B_state;            // Current state of terminal B

public:
    Photoresistor(double base_resistance = 1000000.0,    // 1MΩ in dark
                  double min_resistance = 100.0,          // 100Ω in bright light
                  double light_sensitivity = 0.5);        // Sensitivity factor
    
    void SetBaseResistance(double r);
    double GetBaseResistance() const { return base_resistance; }
    
    void SetMinResistance(double r);
    double GetMinResistance() const { return min_resistance; }
    
    void SetLightSensitivity(double s);
    double GetLightSensitivity() const { return light_sensitivity; }
    
    void SetLightLevel(double level);  // Set light level from 0.0 (dark) to 1.0 (bright)
    double GetLightLevel() const { return light_level; }
    
    double GetCurrentResistance() const { return current_resistance; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif