#include "Photoresistor.h"

Photoresistor::Photoresistor(double base_resistance, double min_resistance, double light_sensitivity)
    : base_resistance(base_resistance < 1.0 ? 1.0 : base_resistance),
      min_resistance(min_resistance < 0.1 ? 0.1 : min_resistance),
      light_sensitivity(light_sensitivity < 0.0 ? 0.0 : (light_sensitivity > 1.0 ? 1.0 : light_sensitivity)),
      current_resistance(base_resistance),
      light_level(0.0),  // Start in dark condition
      terminal_A_state(false), terminal_B_state(false) {
    
    // Ensure min_resistance is less than base_resistance
    if (this->min_resistance > this->base_resistance) {
        this->min_resistance = this->base_resistance * 0.01;  // 1% of base resistance
    }
    
    // Calculate initial current resistance based on initial light level
    double light_factor = 1.0 - light_level;  // Inverse relationship: more light = less resistance
    current_resistance = min_resistance + (base_resistance - min_resistance) * light_factor;
    
    AddBidirectional("A");    // One terminal
    AddBidirectional("B");    // Other terminal
    AddSink("Light");         // Input for light level (0.0 to 1.0)
}

void Photoresistor::SetBaseResistance(double r) {
    base_resistance = r < 1.0 ? 1.0 : r;
    // Maintain the constraint that min_resistance < base_resistance
    if (min_resistance > base_resistance) {
        min_resistance = base_resistance * 0.01;
    }
    // Update current resistance based on light level
    double light_factor = 1.0 - light_level;
    current_resistance = min_resistance + (base_resistance - min_resistance) * light_factor;
}

void Photoresistor::SetMinResistance(double r) {
    min_resistance = r < 0.1 ? 0.1 : r;
    // Maintain the constraint that min_resistance < base_resistance
    if (min_resistance > base_resistance) {
        min_resistance = base_resistance * 0.01;
    }
    // Update current resistance based on light level
    double light_factor = 1.0 - light_level;
    current_resistance = min_resistance + (base_resistance - min_resistance) * light_factor;
}

void Photoresistor::SetLightSensitivity(double s) {
    light_sensitivity = s < 0.0 ? 0.0 : (s > 1.0 ? 1.0 : s);
}

void Photoresistor::SetLightLevel(double level) {
    light_level = level < 0.0 ? 0.0 : (level > 1.0 ? 1.0 : level);
    // Calculate resistance based on light level
    // More light decreases resistance
    double light_factor = 1.0 - light_level;
    // Apply sensitivity factor to make the response more or less pronounced
    light_factor = pow(light_factor, light_sensitivity);
    current_resistance = min_resistance + (base_resistance - min_resistance) * light_factor;
}

bool Photoresistor::Tick() {
    // Update resistance based on current light level
    double light_factor = 1.0 - light_level;
    light_factor = pow(light_factor, light_sensitivity);
    current_resistance = min_resistance + (base_resistance - min_resistance) * light_factor;
    
    return true;
}

bool Photoresistor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        byte temp_data[1] = {0};
        bool signal_pass = false;

        if (conn_id == 2) {  // Light input
            // This is handled by PutRaw
            return true;
        }

        // For signal passing, we consider the current resistance (light-dependent)
        // In digital simulation, this translates to how likely a signal is to pass through
        // Higher resistance means less likely to pass signals (more attenuation)
        if (current_resistance < base_resistance * 0.5) {
            // Low resistance (bright light) - allow signal to pass
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
        // For high resistance (dark), signal may not pass, but in digital simulation
        // we'll still allow some signal to pass with reduced "strength"
        else if (current_resistance < base_resistance * 0.9) {
            // Medium resistance - allow signal to pass with some likelihood
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
        // Very high resistance (very dark) - signal is heavily attenuated
        // In digital terms, we still allow it to pass for the simulation
        
        return signal_pass;
    }

    return false;
}

bool Photoresistor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // Terminal A
            terminal_A_state = (*data & 1) ? true : false;
            return true;
        case 1:  // Terminal B
            terminal_B_state = (*data & 1) ? true : false;
            return true;
        case 2:  // Light input (0-255 mapped to 0.0-1.0)
            SetLightLevel((*data) / 255.0);
            return true;
        default:
            LOG("error: Photoresistor: unimplemented conn-id " << conn_id);
            return false;
    }
}