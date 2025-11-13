#include "Relay.h"

Relay::Relay(double coil_inductance, double coil_resistance, 
             int num_poles, int num_throws, 
             double activation_threshold, double release_threshold)
    : coil_inductance(coil_inductance < 1e-6 ? 1e-6 : coil_inductance),
      coil_resistance(coil_resistance < 0.1 ? 0.1 : coil_resistance),
      coil_energized(false), contacts_closed(false),
      num_poles(num_poles < 1 ? 1 : num_poles), 
      num_throws(num_throws < 1 ? 1 : num_throws),
      contact_states(),
      activation_threshold(activation_threshold),
      release_threshold(release_threshold),
      activation_delay(2),  // 2 tick delay for activation
      release_delay(2),     // 2 tick delay for release
      current_delay(0) {
    
    // Initialize contact states
    contact_states.SetCount(num_poles);
    for (int i = 0; i < num_poles; i++) {
        contact_states[i].SetCount(num_throws);
        for (int j = 0; j < num_throws; j++) {
            contact_states[i][j] = false;  // Initially open
        }
    }
    
    // Add coil connections
    AddSink("CoilA");
    AddSink("CoilK");  // Coil common/kathode
    
    // Add contact connections for each pole and throw
    for (int pole = 0; pole < num_poles; pole++) {
        for (int throw_idx = 0; throw_idx < num_throws; throw_idx++) {
            AddBidirectional("P" + IntStr(pole) + "T" + IntStr(throw_idx));
        }
    }
}

void Relay::SetCoilInductance(double L) {
    coil_inductance = L < 1e-6 ? 1e-6 : L;
}

void Relay::SetCoilResistance(double R) {
    coil_resistance = R < 0.1 ? 0.1 : R;
}

bool Relay::GetContactState(int pole, int throw_index) const {
    if (pole >= 0 && pole < num_poles && throw_index >= 0 && throw_index < num_throws) {
        return contact_states[pole][throw_index];
    }
    return false;  // Invalid indices
}

void Relay::SetContactState(int pole, int throw_index, bool state) {
    if (pole >= 0 && pole < num_poles && throw_index >= 0 && throw_index < num_throws) {
        contact_states[pole][throw_index] = state;
    }
}

bool Relay::Tick() {
    // Handle activation/release delays
    if (current_delay > 0) {
        current_delay--;
        if (current_delay == 0) {
            // Delay period ended, apply the action
            if (coil_energized) {
                contacts_closed = true;
                // Set contact states when relay is activated
                for (int pole = 0; pole < num_poles; pole++) {
                    // For a simple relay, connect pole to first throw when activated
                    // For more complex relays, this would be configurable
                    contact_states[pole][0] = true;  // Connect to first throw
                    for (int throw_idx = 1; throw_idx < num_throws; throw_idx++) {
                        contact_states[pole][throw_idx] = false;  // Disconnect other throws
                    }
                }
            } else {
                contacts_closed = false;
                // Set contact states when relay is deactivated
                for (int pole = 0; pole < num_poles; pole++) {
                    // Reset to default state (first throw open, others as needed)
                    contact_states[pole][0] = false;  // Disconnect normally open contacts
                    // For normally closed contacts, they would be true initially
                }
            }
        }
    }
    
    return true;
}

bool Relay::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        // Handle coil energization first
        if (conn_id < 2) {  // Coil connections
            // Coil control - handled by PutRaw
            return true;
        }
        
        // Handle contact connections
        int contact_idx = conn_id - 2;  // Contacts start after coil connections
        int pole = contact_idx / num_throws;
        int throw_idx = contact_idx % num_throws;
        
        if (pole < num_poles && throw_idx < num_throws) {
            // Check if this contact is closed based on relay state
            if (contact_states[pole][throw_idx]) {
                // The contact is closed, allow signal to pass to other connected throws in same pole
                byte temp_data[1] = {0};
                bool signal_pass = false;
                
                // Send signal to other connected terminals in the same pole
                for (int other_throw = 0; other_throw < num_throws; other_throw++) {
                    if (other_throw != throw_idx && contact_states[pole][other_throw]) {
                        // If both throws are connected (closed to each other), pass signal
                        temp_data[0] = 1; // Set data to reflect connection
                        if (GetConnector(2 + pole * num_throws + other_throw).IsConnected()) {
                            signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                        }
                    }
                }
                
                return signal_pass;
            }
        }
    }

    return false;
}

bool Relay::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle coil energization based on input
    if (conn_id == 0 || conn_id == 1) {  // Coil connections
        bool new_energized = (*data & 1) ? true : false;
        
        // If energization state changes, start appropriate delay
        if (new_energized != coil_energized) {
            coil_energized = new_energized;
            current_delay = new_energized ? activation_delay : release_delay;
        }
        
        return true;
    }
    
    // Handle contact connections
    int contact_idx = conn_id - 2;  // Contacts start after coil connections
    int pole = contact_idx / num_throws;
    int throw_idx = contact_idx % num_throws;
    
    if (pole < num_poles && throw_idx < num_throws) {
        // Update the contact state based on input, if the contact is closed
        if (contact_states[pole][throw_idx]) {
            // For a relay contact in digital simulation, we just acknowledge the connection
            return true;
        }
    }

    LOG("error: Relay: unimplemented conn-id " << conn_id);
    return false;
}