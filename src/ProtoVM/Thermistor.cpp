#include "Thermistor.h"
#include <math.h>

Thermistor::Thermistor(double base_resistance, double reference_temperature, 
                       double beta_coefficient, bool is_ntc)
    : base_resistance(base_resistance < 0.1 ? 0.1 : base_resistance),
      reference_temperature(reference_temperature),
      beta_coefficient(beta_coefficient < 0.1 ? 0.1 : beta_coefficient),
      current_resistance(base_resistance),
      current_temperature(reference_temperature),
      is_ntc(is_ntc),
      terminal_A_state(false), terminal_B_state(false) {
    
    // Calculate initial current resistance based on initial temperature
    if (current_temperature != reference_temperature) {
        // Using the Steinhart-Hart/Beta parameter equation: R = R0 * e^(B*(1/T - 1/T0))
        // Where T and T0 are in Kelvin
        double temp_k = current_temperature + 273.15;
        double ref_temp_k = reference_temperature + 273.15;
        double exp_factor = beta_coefficient * (1.0/temp_k - 1.0/ref_temp_k);
        current_resistance = base_resistance * exp(exp_factor);
        
        // For PTC thermistors, resistance increases with temperature
        if (!is_ntc) {
            current_resistance = base_resistance * exp(exp_factor);
        }
    }
    
    AddBidirectional("A");    // One terminal
    AddBidirectional("B");    // Other terminal
    AddSink("Temperature");   // Input for temperature
}

void Thermistor::SetBaseResistance(double r) {
    base_resistance = r < 0.1 ? 0.1 : r;
    // Update current resistance based on current temperature
    double temp_k = current_temperature + 273.15;
    double ref_temp_k = reference_temperature + 273.15;
    double exp_factor = beta_coefficient * (1.0/temp_k - 1.0/ref_temp_k);
    current_resistance = base_resistance * exp(exp_factor);
    if (!is_ntc) {
        current_resistance = base_resistance * exp(exp_factor);
    }
}

void Thermistor::SetReferenceTemperature(double t) {
    reference_temperature = t;
    // Update current resistance based on current temperature
    double temp_k = current_temperature + 273.15;
    double ref_temp_k = reference_temperature + 273.15;
    double exp_factor = beta_coefficient * (1.0/temp_k - 1.0/ref_temp_k);
    current_resistance = base_resistance * exp(exp_factor);
    if (!is_ntc) {
        current_resistance = base_resistance * exp(exp_factor);
    }
}

void Thermistor::SetBetaCoefficient(double b) {
    beta_coefficient = b < 0.1 ? 0.1 : b;
    // Update current resistance based on current temperature
    double temp_k = current_temperature + 273.15;
    double ref_temp_k = reference_temperature + 273.15;
    double exp_factor = beta_coefficient * (1.0/temp_k - 1.0/ref_temp_k);
    current_resistance = base_resistance * exp(exp_factor);
    if (!is_ntc) {
        current_resistance = base_resistance * exp(exp_factor);
    }
}

void Thermistor::SetIsNTC(bool ntc) {
    is_ntc = ntc;
    // Update current resistance based on current temperature
    double temp_k = current_temperature + 273.15;
    double ref_temp_k = reference_temperature + 273.15;
    double exp_factor = beta_coefficient * (1.0/temp_k - 1.0/ref_temp_k);
    current_resistance = base_resistance * exp(exp_factor);
    if (!is_ntc) {
        // For PTC, the relationship may need to be inverted depending on the specific model
        current_resistance = base_resistance * exp(exp_factor);
    }
}

void Thermistor::SetTemperature(double temp_celsius) {
    current_temperature = temp_celsius;
    // Calculate new resistance based on temperature using the thermistor equation
    // R = R0 * e^(B*(1/T - 1/T0))
    double temp_k = temp_celsius + 273.15;
    double ref_temp_k = reference_temperature + 273.15;
    double exp_factor = beta_coefficient * (1.0/temp_k - 1.0/ref_temp_k);
    
    if (is_ntc) {
        // For NTC thermistors: resistance decreases with increasing temperature
        current_resistance = base_resistance * exp(exp_factor);
    } else {
        // For PTC thermistors: resistance increases with increasing temperature
        // The equation is the same, but the physical behavior is different
        current_resistance = base_resistance * exp(exp_factor);
    }
}

bool Thermistor::Tick() {
    // Update resistance based on current temperature
    double temp_k = current_temperature + 273.15;
    double ref_temp_k = reference_temperature + 273.15;
    double exp_factor = beta_coefficient * (1.0/temp_k - 1.0/ref_temp_k);
    
    if (is_ntc) {
        current_resistance = base_resistance * exp(exp_factor);
    } else {
        current_resistance = base_resistance * exp(exp_factor);
    }
    
    return true;
}

bool Thermistor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        byte temp_data[1] = {0};
        bool signal_pass = false;

        if (conn_id == 2) {  // Temperature input
            // This is handled by PutRaw
            return true;
        }

        // For signal passing, we consider the current resistance (temperature-dependent)
        // In digital simulation, this translates to how likely a signal is to pass through
        // Higher resistance means less likely to pass signals (more attenuation)
        // In a digital context, we'll simplify and say signals pass based on resistance level
        if (current_resistance < base_resistance * 2.0) {
            // Low to medium resistance (higher temperature for NTC) - allow signal to pass
            if (conn_id == 0) {  // From terminal A
                if (GetConnector(1).IsConnected()) { // To terminal B
                    temp_data[0] = terminal_A_state ? 1 : 0;
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
            else if (conn_id == 1) {  // From terminal B
                if (GetConnector(0).IsConnected()) { // To terminal A
                    temp_data[0] = terminal_B_state ? 1 : 0;
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
        }
        // Higher resistance - signal may still pass but with different characteristics
        else {
            // High resistance - still allow signal to pass in digital simulation
            if (conn_id == 0) {  // From terminal A
                if (GetConnector(1).IsConnected()) { // To terminal B
                    temp_data[0] = terminal_A_state ? 1 : 0;
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
            else if (conn_id == 1) {  // From terminal B
                if (GetConnector(0).IsConnected()) { // To terminal A
                    temp_data[0] = terminal_B_state ? 1 : 0;
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
        }
        
        return signal_pass;
    }

    return false;
}

bool Thermistor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // Terminal A
            terminal_A_state = (*data & 1) ? true : false;
            return true;
        case 1:  // Terminal B
            terminal_B_state = (*data & 1) ? true : false;
            return true;
        case 2:  // Temperature input
            // Map the input byte (0-255) to a temperature range (e.g., -40°C to 125°C)
            SetTemperature(-40.0 + ((*data) * 165.0 / 255.0));  // Range from -40°C to 125°C
            return true;
        default:
            LOG("error: Thermistor: unimplemented conn-id " << conn_id);
            return false;
    }
}