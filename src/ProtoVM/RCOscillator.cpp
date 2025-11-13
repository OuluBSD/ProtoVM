#include "RCOscillator.h"
#include <algorithm>

RCOscillator::RCOscillator(double resistance1, double resistance2, 
                           double capacitance, double supply_voltage)
    : supply_voltage(supply_voltage),
      output_voltage(supply_voltage),  // Initialize with supply voltage to start oscillating
      capacitor_voltage(0.1),  // Initialize with a small voltage to kick-start oscillation
      is_charging(true),
      initial_resistance1(resistance1),
      initial_resistance2(resistance2),
      initial_capacitance(capacitance) {
    
    // Calculate thresholds (using 1/3 and 2/3 of supply voltage as in 555 timer)
    target_voltage_low = supply_voltage / 3.0;   // Lower threshold
    target_voltage_high = (2.0 * supply_voltage) / 3.0;  // Upper threshold
    
    // Add output pin
    AddSource("OUT").SetRequired(false);
    analog_values.resize(1, 0.0);
    
    // Initialize the oscillator circuit components
    InitializeCircuit();
}

void RCOscillator::InitializeCircuit() {
    // Create the components
    r1 = new AnalogResistor(initial_resistance1);
    r2 = new AnalogResistor(initial_resistance2);
    c1 = new AnalogCapacitor(initial_capacitance);  // Use the initial capacitance value
    q1 = new AnalogNPNTransistor(100.0);  // Beta of 100
    q2 = new AnalogNPNTransistor(100.0);  // Beta of 100
    
    // For a more complete simulation, we'd connect these components together
    // However, that requires a more complex connection system
    
    // Simplified model for demonstration purposes
    // In a real implementation, we would connect these using the ProtoVM connection system
}

bool RCOscillator::Tick() {
    // Update the oscillator state based on time and component values
    UpdateOscillatorState();
    
    // Call parent tick
    AnalogNodeBase::Tick();
    
    return true;
}

void RCOscillator::UpdateOscillatorState() {
    // Calculate time constant for the RC circuit
    // tau = R * C
    double effective_resistance = is_charging ? r1->GetResistance() : r2->GetResistance();
    double time_constant = effective_resistance * c1->GetCapacitance();
    
    // Calculate time step (should match the simulation time step)
    double dt = SIMULATION_TIMESTEP;
    
    // For a relaxation oscillator, determine the charging/discharging state
    if (is_charging) {
        // Charging towards supply voltage
        capacitor_voltage = RCResponse(capacitor_voltage, supply_voltage, time_constant, dt);
        
        // Check if we've reached the upper threshold
        if (capacitor_voltage >= target_voltage_high) {
            is_charging = false;  // Start discharging
            output_voltage = 0.0; // Output goes low
        }
        // Output stays high while charging and below threshold
    } else {
        // Discharging towards 0V
        capacitor_voltage = RCResponse(capacitor_voltage, 0.0, time_constant, dt);
        
        // Check if we've reached the lower threshold
        if (capacitor_voltage <= target_voltage_low) {
            is_charging = true;   // Start charging again
            output_voltage = supply_voltage; // Output goes high
        }
        // Output stays low while discharging and above threshold
    }
    
    // Update the output connector with the calculated voltage
    UpdateAnalogValue(0, output_voltage);
}

void RCOscillator::SetSupplyVoltage(double v) {
    supply_voltage = v;
    target_voltage_high = (2.0 * supply_voltage) / 3.0;
    target_voltage_low = supply_voltage / 3.0;
}