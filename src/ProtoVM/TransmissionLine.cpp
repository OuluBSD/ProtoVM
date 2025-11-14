#include "TransmissionLine.h"

// TransmissionLine Implementation
TransmissionLine::TransmissionLine(double characteristic_impedance, double delay_time) 
    : characteristic_impedance(characteristic_impedance < MIN_IMPEDANCE ? MIN_IMPEDANCE : characteristic_impedance),
      delay_time(delay_time < MIN_DELAY ? MIN_DELAY : delay_time) {
    // A transmission line has two terminals
    AddBidirectional("A");
    AddBidirectional("B");
    analog_values.resize(2, 0.0);  // Initialize both terminals to 0V
    
    // Calculate delay in samples based on simulation timestep
    delay_samples = static_cast<int>(delay_time / SIMULATION_TIMESTEP);
    if (delay_samples > MAX_DELAY_SAMPLES) {
        delay_samples = MAX_DELAY_SAMPLES;  // Cap at maximum
    }
    
    // Initialize delay buffers
    voltage_delay_buffer_a.resize(MAX_DELAY_SAMPLES, 0.0);
    voltage_delay_buffer_b.resize(MAX_DELAY_SAMPLES, 0.0);
    current_sample_index = 0;
}

bool TransmissionLine::Tick() {
    double voltage_a = GetAnalogValue(0);  // Voltage at terminal A
    double voltage_b = GetAnalogValue(1);  // Voltage at terminal B
    
    // Get the delayed voltage values from the buffers
    int delayed_index = (current_sample_index - delay_samples + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
    
    // For a simple model of a transmission line, we'll simulate the delay and 
    // characteristic impedance effect
    // In a real transmission line, signals propagate in both directions simultaneously
    // with characteristic impedance determining the relationship between voltage and current
    
    // Calculate the voltage that has propagated from each end
    double delayed_voltage_a = voltage_delay_buffer_a[delayed_index];
    double delayed_voltage_b = voltage_delay_buffer_b[delayed_index];
    
    // Store current voltages in the delay buffers for future retrieval
    voltage_delay_buffer_a[current_sample_index] = voltage_a;
    voltage_delay_buffer_b[current_sample_index] = voltage_b;
    
    // Update the output terminals with delayed values
    // In a more realistic model, we would account for reflections
    // For now, we'll just apply the delay with some characteristic impedance effect
    double output_a = delayed_voltage_a;
    double output_b = delayed_voltage_b;
    
    // Apply some basic characteristic impedance behavior
    // If there's a mismatch between source impedance and characteristic impedance,
    // we could see reflections, but for simplicity we'll model it as a simple delay
    UpdateAnalogValue(0, output_a);
    UpdateAnalogValue(1, output_b);
    
    // Update current sample index for next iteration
    current_sample_index = (current_sample_index + 1) % MAX_DELAY_SAMPLES;
    
    AnalogNodeBase::Tick();  // Call parent tick
    return true;
}

void TransmissionLine::SetCharacteristicImpedance(double z0) {
    characteristic_impedance = z0 < MIN_IMPEDANCE ? MIN_IMPEDANCE : z0;
}

void TransmissionLine::SetDelayTime(double delay_time) {
    this->delay_time = delay_time < MIN_DELAY ? MIN_DELAY : delay_time;
    // Recalculate delay samples
    delay_samples = static_cast<int>(this->delay_time / SIMULATION_TIMESTEP);
    if (delay_samples > MAX_DELAY_SAMPLES) {
        delay_samples = MAX_DELAY_SAMPLES;  // Cap at maximum
    }
}