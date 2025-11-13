#include "AnalogSemiconductors.h"
#include <algorithm>

// AnalogDiode Implementation
AnalogDiode::AnalogDiode(double saturation_current, double emission_coefficient) 
    : saturation_current(saturation_current), 
      emission_coefficient(emission_coefficient) {
    // A diode has anode and cathode
    AddBidirectional("ANODE");
    AddBidirectional("CATHODE");
    analog_values.resize(2, 0.0);  // Initialize both terminals to 0V
}

bool AnalogDiode::Tick() {
    double anode_voltage = GetAnalogValue(0);
    double cathode_voltage = GetAnalogValue(1);
    double voltage_across_diode = anode_voltage - cathode_voltage;
    
    // Use Shockley diode equation: I = IS * (e^(Vd/n*VT) - 1)
    double exponent = voltage_across_diode / (emission_coefficient * VT);
    
    // Limit the exponent to prevent overflow issues
    exponent = std::max(-300.0, std::min(300.0, exponent));
    
    double diode_current = saturation_current * (exp(exponent) - 1.0);
    
    // For simulation purposes, we'll update the voltages based on the current
    // In a real simulation, this would be connected to a circuit solver
    if (voltage_across_diode > 0.7) {  // Forward bias, silicon diode
        // Diode is conducting, drop voltage accordingly
        UpdateAnalogValue(0, std::max(anode_voltage, cathode_voltage + 0.7));
        UpdateAnalogValue(1, std::min(cathode_voltage, anode_voltage - 0.7));
    } else {
        // Diode is not conducting, just pass voltages through
        // but with some leakage
        UpdateAnalogValue(0, anode_voltage);
        UpdateAnalogValue(1, cathode_voltage);
    }
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void AnalogDiode::SetSaturationCurrent(double is) {
    saturation_current = is > 1e-20 ? is : 1e-20;  // Set a minimum value
}

void AnalogDiode::SetEmissionCoefficient(double n) {
    emission_coefficient = n > 0.1 ? n : 0.1;  // Set a minimum value
}

// AnalogNPNTransistor Implementation
AnalogNPNTransistor::AnalogNPNTransistor(double beta) : beta(beta), 
                                           collector_current(0.0),
                                           base_current(0.0),
                                           emitter_current(0.0) {
    // An NPN transistor has three terminals
    AddBidirectional("COLLECTOR");
    AddBidirectional("BASE");
    AddBidirectional("EMITTER");
    analog_values.resize(3, 0.0);  // Initialize all terminals to 0V
}

bool AnalogNPNTransistor::Tick() {
    double collector_voltage = GetAnalogValue(0);
    double base_voltage = GetAnalogValue(1);
    double emitter_voltage = GetAnalogValue(2);
    
    // Calculate base-emitter and base-collector voltages
    double vbe = base_voltage - emitter_voltage;
    double vbc = base_voltage - collector_voltage;
    
    // Determine operating region
    if (vbe > 0.7 && vbc < 0.4) {
        // Forward active region - transistor is on
        // Ebers-Moll model approximation
        
        // Base current from base-emitter voltage (simplified)
        if (vbe > 0.7) {
            // Base-emitter junction is forward biased
            double ib_temp = (vbe - 0.7) / 0.026;  // Simplified
            base_current = std::max(0.0, ib_temp);
        } else {
            base_current = 0.0;
        }
        
        // Collector current based on beta
        collector_current = beta * base_current;
        emitter_current = collector_current + base_current;
        
        // Update voltages based on current and external circuit
        // This is a simplified model - in a real simulator, 
        // this would interact with the circuit equations
        if (collector_current > 0) {
            // Transistor conducting
            UpdateAnalogValue(0, collector_voltage);  // Collector
            UpdateAnalogValue(1, base_voltage);       // Base
            UpdateAnalogValue(2, emitter_voltage);    // Emitter
        } else {
            // Transistor off
            UpdateAnalogValue(0, collector_voltage);
            UpdateAnalogValue(1, base_voltage);
            UpdateAnalogValue(2, emitter_voltage);
        }
    } else if (vbe <= 0.7 && vbc <= 0.7) {
        // Cut-off region - transistor is off
        collector_current = MIN_CURRENT;
        base_current = MIN_CURRENT;
        emitter_current = MIN_CURRENT;
        
        UpdateAnalogValue(0, collector_voltage);
        UpdateAnalogValue(1, base_voltage);
        UpdateAnalogValue(2, emitter_voltage);
    } else {
        // Saturation or reverse active region
        // Simplified treatment
        collector_current = beta * base_current;
        emitter_current = collector_current + base_current;
        
        UpdateAnalogValue(0, collector_voltage);
        UpdateAnalogValue(1, base_voltage);
        UpdateAnalogValue(2, emitter_voltage);
    }
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void AnalogNPNTransistor::SetBeta(double b) {
    beta = b > 0.1 ? b : 0.1;  // Set a minimum value for beta
}