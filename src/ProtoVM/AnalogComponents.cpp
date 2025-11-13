#include "AnalogComponents.h"

// AnalogResistor Implementation
AnalogResistor::AnalogResistor(double resistance) : resistance(resistance < MIN_RESISTANCE ? MIN_RESISTANCE : resistance) {
    // A resistor has two terminals
    AddBidirectional("A");
    AddBidirectional("B");
    analog_values.resize(2, 0.0);  // Initialize both terminals to 0V
}

bool AnalogResistor::Tick() {
    // Apply Ohm's law: V = I * R, or I = V / R
    // For simulation, we'll determine current based on voltage difference
    
    double voltage_a = GetAnalogValue(0);  // Voltage at terminal A
    double voltage_b = GetAnalogValue(1);  // Voltage at terminal B
    double voltage_diff = voltage_a - voltage_b;
    
    // Current flows from higher potential to lower potential
    double current = voltage_diff / resistance;
    
    // In a real simulation, we'd interface this with connected components
    // For now, we'll just update the internal representation
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void AnalogResistor::SetResistance(double r) {
    resistance = r < MIN_RESISTANCE ? MIN_RESISTANCE : r;
}

// AnalogCapacitor Implementation
AnalogCapacitor::AnalogCapacitor(double capacitance) 
    : capacitance(capacitance < MIN_CAPACITANCE ? MIN_CAPACITANCE : capacitance),
      voltage_across_capacitor(0.0),
      charge(0.0) {
    // A capacitor has two terminals
    AddBidirectional("POS");
    AddBidirectional("NEG");
    analog_values.resize(2, 0.0);  // Initialize both terminals to 0V
}

bool AnalogCapacitor::Tick() {
    // Calculate voltage difference across the capacitor
    double voltage_pos = GetAnalogValue(0);  // Positive terminal
    double voltage_neg = GetAnalogValue(1);  // Negative terminal
    double voltage_diff = voltage_pos - voltage_neg;
    
    // Update internal voltage based on the applied voltage
    // In a real circuit simulation, we would consider the current flowing in/out
    // For now, we'll make a simple approximation
    
    // Simple model: capacitor charges/discharges towards applied voltage
    // with a time constant related to the external circuit
    double time_constant = 0.001; // 1ms for demo purposes, in real use this should come from circuit
    double time_step = SIMULATION_TIMESTEP;
    
    // Exponential approximation for charging/discharging
    voltage_across_capacitor = RCResponse(voltage_across_capacitor, voltage_diff, time_constant, time_step);
    
    // Update terminal voltages based on internal voltage
    // For a simple model, we'll just make the output reflect the internal voltage
    UpdateAnalogValue(0, voltage_across_capacitor + voltage_neg);
    UpdateAnalogValue(1, voltage_neg);  // Keep negative terminal stable for this simple model
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void AnalogCapacitor::SetCapacitance(double c) {
    capacitance = c < MIN_CAPACITANCE ? MIN_CAPACITANCE : c;
}

// AnalogInductor Implementation
AnalogInductor::AnalogInductor(double inductance) 
    : inductance(inductance < MIN_INDUCTANCE ? MIN_INDUCTANCE : inductance),
      current_through_inductor(0.0) {
    // An inductor has two terminals
    AddBidirectional("A");
    AddBidirectional("B");
    analog_values.resize(2, 0.0);  // Initialize both terminals to 0V
}

bool AnalogInductor::Tick() {
    // Calculate voltage difference across the inductor
    double voltage_a = GetAnalogValue(0);  // Terminal A
    double voltage_b = GetAnalogValue(1);  // Terminal B
    double voltage_diff = voltage_a - voltage_b;
    
    // In a real simulation, we'd apply the inductor equation: V = L * di/dt
    // For our discrete simulation: di = (V / L) * dt
    double dt = SIMULATION_TIMESTEP;
    double di = (voltage_diff / inductance) * dt;
    
    // Update current through the inductor
    current_through_inductor += di;
    
    // Limit current to reasonable bounds
    if (current_through_inductor > 100.0) current_through_inductor = 100.0;   // 100A max
    if (current_through_inductor < -100.0) current_through_inductor = -100.0; // -100A min
    
    // The voltage across the inductor affects the connected nodes
    // For now, we'll simply pass through the voltage with some inductive effect
    double back_emf = -inductance * (di / dt);  // Back EMF effect
    UpdateAnalogValue(0, voltage_a + back_emf);
    UpdateAnalogValue(1, voltage_b);
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void AnalogInductor::SetInductance(double l) {
    inductance = l < MIN_INDUCTANCE ? MIN_INDUCTANCE : l;
}