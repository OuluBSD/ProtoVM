#ifndef _ProtoVM_Thermistor_h_
#define _ProtoVM_Thermistor_h_

#include "ProtoVM.h"

// Thermistor component - temperature dependent resistor
// Can be NTC (Negative Temperature Coefficient) or PTC (Positive Temperature Coefficient)
class Thermistor : public ElcBase {
private:
    double base_resistance;           // Resistance at reference temperature (Ohms)
    double reference_temperature;     // Reference temperature in Celsius (default 25°C)
    double beta_coefficient;          // Beta coefficient for thermistor equation
    double current_resistance;        // Current resistance based on temperature
    double current_temperature;       // Current temperature in Celsius
    bool is_ntc;                      // True for NTC, false for PTC
    bool terminal_A_state;            // Current state of terminal A
    bool terminal_B_state;            // Current state of terminal B

public:
    Thermistor(double base_resistance = 10000.0,    // 10kΩ at reference temperature
               double reference_temperature = 25.0,  // 25°C reference
               double beta_coefficient = 3950.0,     // Typical beta value
               bool is_ntc = true);                  // NTC by default
    
    void SetBaseResistance(double r);
    double GetBaseResistance() const { return base_resistance; }
    
    void SetReferenceTemperature(double t);
    double GetReferenceTemperature() const { return reference_temperature; }
    
    void SetBetaCoefficient(double b);
    double GetBetaCoefficient() const { return beta_coefficient; }
    
    void SetIsNTC(bool ntc);
    bool GetIsNTC() const { return is_ntc; }
    
    void SetTemperature(double temp_celsius);  // Set current temperature
    double GetTemperature() const { return current_temperature; }
    
    double GetCurrentResistance() const { return current_resistance; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif