#ifndef _ProtoVM_RCOscillator_h_
#define _ProtoVM_RCOscillator_h_

#include "AnalogCommon.h"
#include "AnalogComponents.h"
#include "AnalogSemiconductors.h"
#include "AnalogSimulation.h"

// RC Oscillator using analog components
// Implements a simple relaxation oscillator using an RC circuit and a comparator (made from transistors)
class RCOscillator : public AnalogNodeBase {
public:
    typedef RCOscillator CLASSNAME;
    
    // Create an oscillator with specified R and C values
    RCOscillator(double resistance1 = 1000.0,   // 1kΩ (faster oscillation for testing)
                 double resistance2 = 1000.0,   // 1kΩ (faster oscillation for testing)
                 double capacitance = 1e-8,     // 10nF (faster oscillation for testing)
                 double supply_voltage = 5.0);  // 5V supply
    
    virtual ~RCOscillator() {}
    
    virtual bool Tick() override;
    virtual String GetClassName() const override { return "RCOscillator"; }
    
    // Set the supply voltage
    void SetSupplyVoltage(double v);
    
    // Get the current output state (for connection to other components)
    double GetOutputVoltage() const { return output_voltage; }
    
private:
    // Components of the oscillator circuit
    AnalogResistor* r1;      // Charging resistor
    AnalogResistor* r2;      // Discharging resistor  
    AnalogCapacitor* c1;     // Timing capacitor
    AnalogNPNTransistor* q1; // First transistor for switching
    AnalogNPNTransistor* q2; // Second transistor for switching (inverter)
    
    // Parameters
    double supply_voltage;
    double target_voltage_high;  // Upper threshold for oscillation
    double target_voltage_low;   // Lower threshold for oscillation
    
    // Internal state
    double output_voltage;
    double capacitor_voltage;
    bool is_charging;  // State of the oscillator
    
    // Component values
    double initial_resistance1;
    double initial_resistance2;
    double initial_capacitance;
    
    // Update the oscillator state based on internal calculations
    void UpdateOscillatorState();
    
    // Initialize components
    void InitializeCircuit();
};

#endif