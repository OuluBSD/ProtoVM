#include "AnalogCommon.h"

AnalogNodeBase::AnalogNodeBase() {
    // Initialize analog values to 0V for each connector
    analog_values.resize(conns.GetCount(), 0.0);
}

bool AnalogNodeBase::Tick() {
    // Update simulation time
    simulation_time += SIMULATION_TIMESTEP;
    
    // Process analog behavior
    UpdateState();
    ComputeOutputs();
    
    // Set changed flag if needed
    SetChanged(false); // For now, assume no change unless proven otherwise
    
    return true;
}

bool AnalogNodeBase::ProcessAnalog(double input_voltage, int pin_id) {
    if (pin_id >= 0 && pin_id < analog_values.size()) {
        analog_values[pin_id] = input_voltage;
        return true;
    }
    return false;
}

void AnalogNodeBase::SetAnalogValue(int pin_id, double voltage) {
    if (pin_id >= 0 && pin_id < analog_values.size()) {
        analog_values[pin_id] = voltage;
    }
}

double AnalogNodeBase::GetAnalogValue(int pin_id) const {
    if (pin_id >= 0 && pin_id < analog_values.size()) {
        return analog_values[pin_id];
    }
    return 0.0; // Default to 0V if invalid pin
}

void AnalogNodeBase::UpdateAnalogValue(int pin_id, double voltage) {
    if (pin_id >= 0 && pin_id < analog_values.size()) {
        analog_values[pin_id] = voltage;
    }
}

double AnalogNodeBase::CalculateRCConstant(double resistance, double capacitance) {
    return resistance * capacitance;
}

double AnalogNodeBase::RCResponse(double initial_voltage, double target_voltage, 
                                 double time_constant, double time_elapsed) {
    if (time_constant <= 0) {
        return target_voltage;  // Immediate change if no time constant
    }
    
    // Exponential response: V(t) = Vf + (Vi - Vf) * e^(-t/tau)
    return target_voltage + (initial_voltage - target_voltage) * 
           exp(-time_elapsed / time_constant);
}

void AnalogNodeBase::UpdateState() {
    // Default implementation - to be overridden by subclasses
}

void AnalogNodeBase::ComputeOutputs() {
    // Default implementation - to be overridden by subclasses
}