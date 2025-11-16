#include "ModulationMatrix.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

ModulationMatrix::ModulationMatrix(int max_connections)
    : max_connections(max_connections)
    , active_connections(0)
    , velocity_value(0.0)
    , aftertouch_value(0.0)
    , wheel_value(0.0)
    , gate_value(0.0)
    , pressure_value(0.0)
{
    connections.reserve(max_connections);
}

bool ModulationMatrix::Tick() {
    // The modulation matrix processes values but doesn't have an output of its own
    // The processing happens in ProcessModulation when other components call it
    return true;
}

bool ModulationMatrix::AddConnection(const ModulationConnection& connection) {
    if (connections.size() >= max_connections) {
        return false; // Matrix is full
    }
    
    connections.push_back(connection);
    if (connection.active) {
        active_connections++;
    }
    
    return true;
}

bool ModulationMatrix::RemoveConnection(int index) {
    if (index < 0 || index >= connections.size()) {
        return false;
    }
    
    if (connections[index].active) {
        active_connections--;
    }
    
    connections.erase(connections.begin() + index);
    return true;
}

bool ModulationMatrix::UpdateConnection(int index, double new_amount) {
    if (index < 0 || index >= connections.size()) {
        return false;
    }
    
    connections[index].amount = std::max(-1.0, std::min(1.0, new_amount)); // Clamp between -1.0 and 1.0
    return true;
}

void ModulationMatrix::ClearAllConnections() {
    connections.clear();
    active_connections = 0;
}

double ModulationMatrix::ProcessModulation(ModulationDestination dest, double base_value) {
    double total_modulation = 0.0;
    
    // Process each active connection that targets the specified destination
    for (auto& conn : connections) {
        if (conn.active && conn.destination == dest) {
            double modulation_value = 0.0;
            bool source_valid = true;
            
            // Get the modulation value based on the source
            switch (conn.source) {
                case ModulationSource::LFO1:
                case ModulationSource::LFO2:
                case ModulationSource::LFO3:
                    {
                        int lfo_id = static_cast<int>(conn.source) - static_cast<int>(ModulationSource::LFO1) + 1;
                        auto it = lfo_values.find(lfo_id);
                        if (it != lfo_values.end()) {
                            modulation_value = it->second;
                        } else {
                            source_valid = false;
                        }
                    }
                    break;
                    
                case ModulationSource::ADSR1:
                case ModulationSource::ADSR2:
                    {
                        int adsr_id = static_cast<int>(conn.source) - static_cast<int>(ModulationSource::ADSR1) + 1;
                        auto it = adsr_values.find(adsr_id);
                        if (it != adsr_values.end()) {
                            modulation_value = it->second;
                        } else {
                            source_valid = false;
                        }
                    }
                    break;
                    
                case ModulationSource::VELOCITY:
                    modulation_value = velocity_value;
                    break;
                    
                case ModulationSource::AFTERTOUCH:
                    modulation_value = aftertouch_value;
                    break;
                    
                case ModulationSource::WHEEL:
                    modulation_value = wheel_value;
                    break;
                    
                case ModulationSource::GATE:
                    modulation_value = gate_value;
                    break;
                    
                case ModulationSource::PRESSURE:
                    modulation_value = pressure_value;
                    break;
                    
                case ModulationSource::KEY_TRACK:
                    // For key tracking, we might use the base note frequency
                    // This would be implemented based on the keyboard input
                    modulation_value = 0.0; // Placeholder
                    break;
                    
                case ModulationSource::ENV_FOLLOW:
                    // Envelope follower would analyze the input signal
                    modulation_value = 0.0; // Placeholder
                    break;
                    
                case ModulationSource::VELOCITY_FOLLOW:
                    // Velocity follower would track velocity changes over time
                    modulation_value = 0.0; // Placeholder
                    break;
                    
                case ModulationSource::RANDOM:
                    // Random modulation - would typically be generated here
                    modulation_value = 0.0; // Would implement random value
                    break;
                    
                case ModulationSource::CUSTOM:
                    // Custom sources would be implemented as needed
                    modulation_value = 0.0; // Placeholder
                    break;
                    
                default:
                    source_valid = false;
                    break;
            }
            
            if (source_valid) {
                // Apply the modulation amount to the modulation value
                double applied_mod = modulation_value * conn.amount;
                total_modulation += applied_mod;
            }
        }
    }
    
    // Apply the total modulation to the base value
    return ApplyModulation(base_value, total_modulation, 1.0);
}

double ModulationMatrix::GetModulationAmount(int index) const {
    if (index < 0 || index >= connections.size()) {
        return 0.0;
    }
    return connections[index].amount;
}

void ModulationMatrix::SetModulationAmount(int index, double amount) {
    if (index < 0 || index >= connections.size()) {
        return;
    }
    connections[index].amount = std::max(-1.0, std::min(1.0, amount)); // Clamp between -1.0 and 1.0
}

void ModulationMatrix::SetConnectionActive(int index, bool active) {
    if (index < 0 || index >= connections.size()) {
        return;
    }
    
    bool was_active = connections[index].active;
    connections[index].active = active;
    
    if (was_active && !active) {
        active_connections--;
    } else if (!was_active && active) {
        active_connections++;
    }
}

bool ModulationMatrix::IsConnectionActive(int index) const {
    if (index < 0 || index >= connections.size()) {
        return false;
    }
    return connections[index].active;
}

void ModulationMatrix::SetLFOValue(int lfo_id, double value) {
    lfo_values[lfo_id] = value;
}

void ModulationMatrix::SetADSRValue(int adsr_id, double value) {
    adsr_values[adsr_id] = value;
}

void ModulationMatrix::SetVelocityValue(double value) {
    velocity_value = std::max(0.0, std::min(1.0, value));
}

void ModulationMatrix::SetAftertouchValue(double value) {
    aftertouch_value = std::max(0.0, std::min(1.0, value));
}

void ModulationMatrix::SetWheelValue(double value) {
    wheel_value = std::max(-1.0, std::min(1.0, value)); // Wheel typically ranges from -1 to 1
}

void ModulationMatrix::SetGateValue(double value) {
    gate_value = value;
}

void ModulationMatrix::SetPressureValue(double value) {
    pressure_value = std::max(0.0, std::min(1.0, value));
}

std::string ModulationMatrix::GetSourceName(ModulationSource source) {
    switch (source) {
        case ModulationSource::LFO1: return "LFO1";
        case ModulationSource::LFO2: return "LFO2";
        case ModulationSource::LFO3: return "LFO3";
        case ModulationSource::ADSR1: return "ADSR1";
        case ModulationSource::ADSR2: return "ADSR2";
        case ModulationSource::ENV_FOLLOW: return "ENV_FOLLOW";
        case ModulationSource::KEY_TRACK: return "KEY_TRACK";
        case ModulationSource::VELOCITY: return "VELOCITY";
        case ModulationSource::AFTERTOUCH: return "AFTERTOUCH";
        case ModulationSource::WHEEL: return "WHEEL";
        case ModulationSource::GATE: return "GATE";
        case ModulationSource::VELOCITY_FOLLOW: return "VELOCITY_FOLLOW";
        case ModulationSource::PRESSURE: return "PRESSURE";
        case ModulationSource::RANDOM: return "RANDOM";
        case ModulationSource::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

std::string ModulationMatrix::GetDestinationName(ModulationDestination destination) {
    switch (destination) {
        case ModulationDestination::VCO1_PITCH: return "VCO1_PITCH";
        case ModulationDestination::VCO2_PITCH: return "VCO2_PITCH";
        case ModulationDestination::VCO3_PITCH: return "VCO3_PITCH";
        case ModulationDestination::VCO_ALL_PITCH: return "VCO_ALL_PITCH";
        case ModulationDestination::VCF_CUTOFF: return "VCF_CUTOFF";
        case ModulationDestination::VCA_LEVEL: return "VCA_LEVEL";
        case ModulationDestination::LFO1_RATE: return "LFO1_RATE";
        case ModulationDestination::LFO2_RATE: return "LFO2_RATE";
        case ModulationDestination::VCF_RESONANCE: return "VCF_RESONANCE";
        case ModulationDestination::VCO1_PWM: return "VCO1_PWM";
        case ModulationDestination::VCO2_PWM: return "VCO2_PWM";
        case ModulationDestination::VCO3_PWM: return "VCO3_PWM";
        case ModulationDestination::CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

double ModulationMatrix::ApplyModulation(double base_value, double modulation_amount, double modulation_depth) {
    // Apply the modulation to the base value
    // This could be implemented in various ways: linear addition, exponential, etc.
    // For now, we'll use simple linear addition scaled by depth
    double final_modulation = modulation_amount * modulation_depth;
    return base_value + final_modulation;
}