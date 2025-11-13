#ifndef _ProtoVM_CrystalOscillator_h_
#define _ProtoVM_CrystalOscillator_h_

#include "ProtoVM.h"

// Crystal oscillator component - generates stable clock signal at resonant frequency
class CrystalOscillator : public ElcBase {
private:
    double frequency;                 // Oscillation frequency in Hz
    double period_ticks;              // Period in simulation ticks
    int current_tick;                 // Current tick in the oscillation cycle
    bool output_state;                // Current output state (high/low)
    bool enable_state;                // Whether oscillator is enabled
    bool load_capacitance;            // Simulated load capacitance effect
    double stability_factor;          // Factor affecting frequency stability
    double aging_factor;              // Factor simulating long-term frequency drift
    double temperature_coefficient;   // PPM change per degree Celsius
    double current_temperature;       // Current temperature in Celsius

public:
    CrystalOscillator(double frequency = 1000000.0,  // 1 MHz by default
                      bool initially_enabled = true,
                      double stability_factor = 0.999,      // 0.1% stability
                      double aging_factor = 0.000001,       // 1 PPM per hour
                      double temperature_coefficient = 0.5); // 0.5 PPM/°C
    
    void SetFrequency(double freq_hz);
    double GetFrequency() const { return frequency; }
    
    void SetStabilityFactor(double factor);
    double GetStabilityFactor() const { return stability_factor; }
    
    void SetAgingFactor(double factor);
    double GetAgingFactor() const { return aging_factor; }
    
    void SetTemperatureCoefficient(double coeff);
    double GetTemperatureCoefficient() const { return temperature_coefficient; }
    
    void SetTemperature(double temp_celsius);
    double GetTemperature() const { return current_temperature; }
    
    void Enable();
    void Disable();
    bool IsEnabled() const { return enable_state; }
    
    bool GetOutputState() const { return output_state; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif