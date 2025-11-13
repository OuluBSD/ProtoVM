#include "CrystalOscillator.h"

CrystalOscillator::CrystalOscillator(double frequency, bool initially_enabled,
                                     double stability_factor, double aging_factor,
                                     double temperature_coefficient)
    : frequency(frequency < 1.0 ? 1.0 : frequency),
      period_ticks(0),  // Will be calculated in SetFrequency
      current_tick(0),
      output_state(false),
      enable_state(initially_enabled),
      load_capacitance(true),  // Assume proper load capacitance initially
      stability_factor(stability_factor < 0.9 ? 0.9 : (stability_factor > 1.0 ? 1.0 : stability_factor)),
      aging_factor(aging_factor),
      temperature_coefficient(temperature_coefficient),
      current_temperature(25.0) {  // Default room temperature
    
    SetFrequency(frequency);  // Properly calculate the period
    
    AddSource("Out").SetMultiConn();    // Output signal
    AddSink("Enable");                  // Enable/disable control
    AddSink("Temperature");             // Temperature input for drift simulation
}

void CrystalOscillator::SetFrequency(double freq_hz) {
    frequency = freq_hz < 1.0 ? 1.0 : freq_hz;
    // Calculate period in ticks based on simulation timing
    // For this implementation, we'll assume 1 tick corresponds to some time unit
    // (In a real implementation, this would be tied to actual time)
    period_ticks = 1000000.0 / frequency;  // Simplified: assume 1 MHz reference
    if (period_ticks < 1.0) period_ticks = 1.0;  // Minimum 1 tick period
}

void CrystalOscillator::SetStabilityFactor(double factor) {
    stability_factor = factor < 0.9 ? 0.9 : (factor > 1.0 ? 1.0 : factor);
}

void CrystalOscillator::SetAgingFactor(double factor) {
    aging_factor = factor;
}

void CrystalOscillator::SetTemperatureCoefficient(double coeff) {
    temperature_coefficient = coeff;
}

void CrystalOscillator::SetTemperature(double temp_celsius) {
    current_temperature = temp_celsius;
}

void CrystalOscillator::Enable() {
    enable_state = true;
}

void CrystalOscillator::Disable() {
    enable_state = false;
}

bool CrystalOscillator::Tick() {
    if (!enable_state) {
        return true;  // If disabled, maintain current state
    }
    
    // Increment tick counter
    current_tick++;
    
    // Calculate effective frequency considering stability, aging, and temperature
    double effective_freq = frequency;
    
    // Apply aging effect (simplified)
    static int total_ticks = 0;
    total_ticks++;
    
    // Apply temperature effect (simplified)
    double temp_offset = (current_temperature - 25.0) * temperature_coefficient * 1e-6; // PPM per degree
    effective_freq *= (1.0 + temp_offset);
    
    // Apply stability factor (simplified - in real crystals this would be a random walk)
    effective_freq *= stability_factor;
    
    // Calculate adjusted period based on effective frequency
    double adjusted_period = period_ticks / (effective_freq / frequency);
    
    // Toggle output when half the period is reached
    if (current_tick >= adjusted_period / 2) {
        output_state = !output_state;
        current_tick = 0;
    }
    
    return true;
}

bool CrystalOscillator::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        if (conn_id == 0) {  // Output
            return dest.PutRaw(dest_conn_id, (byte*)&output_state, bytes, bits);
        }
        // Enable and Temperature inputs are handled by PutRaw
    }

    return false;
}

bool CrystalOscillator::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 1:  // Enable input
            enable_state = (*data & 1) ? true : false;
            // When enabled, reset tick counter to align to next cycle
            if (enable_state) {
                current_tick = 0;
            }
            return true;
        case 2:  // Temperature input
            // Map the input byte (0-255) to a temperature range (e.g., -40°C to 125°C)
            SetTemperature(-40.0 + ((*data) * 165.0 / 255.0));  // Range from -40°C to 125°C
            return true;
        default:
            LOG("error: CrystalOscillator: unimplemented conn-id " << conn_id);
            return false;
    }
}