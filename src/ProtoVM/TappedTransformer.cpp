#include "TappedTransformer.h"

TappedTransformer::TappedTransformer(double primary_turns, const Vector<double>& secondary_turns, 
                                     double coupling_coefficient)
    : primary_turns(primary_turns < 1.0 ? 1.0 : primary_turns),
      secondary_turns(secondary_turns),
      coupling_coefficient(coupling_coefficient < 0.0 ? 0.0 : (coupling_coefficient > 1.0 ? 1.0 : coupling_coefficient)),
      num_taps(secondary_turns.GetCount()),
      tap_voltages(),
      tap_states(),
      primary_in_state(false), primary_out_state(false) {
    
    // Initialize tap voltages and states
    tap_voltages.SetCount(num_taps);
    tap_states.SetCount(num_taps);
    for (int i = 0; i < num_taps; i++) {
        tap_voltages[i] = 0.0;
        tap_states[i] = false;
    }
    
    // Add primary connections
    AddBidirectional("PriIn");
    AddBidirectional("PriOut");
    
    // Add connections for each tap
    for (int i = 0; i < num_taps; i++) {
        AddBidirectional("Sec" + IntStr(i) + "In");
        AddBidirectional("Sec" + IntStr(i) + "Out");
    }
}

void TappedTransformer::SetPrimaryTurns(double turns) {
    primary_turns = turns < 1.0 ? 1.0 : turns;
}

void TappedTransformer::SetSecondaryTurns(int tap_index, double turns) {
    if (tap_index >= 0 && tap_index < num_taps) {
        secondary_turns[tap_index] = turns < 1.0 ? 1.0 : turns;
    }
}

double TappedTransformer::GetSecondaryTurns(int tap_index) const {
    if (tap_index >= 0 && tap_index < num_taps) {
        return secondary_turns[tap_index];
    }
    return 0.0;  // Invalid index
}

void TappedTransformer::SetCouplingCoefficient(double coeff) {
    coupling_coefficient = coeff < 0.0 ? 0.0 : (coeff > 1.0 ? 1.0 : coeff);
}

bool TappedTransformer::Tick() {
    // Update tap voltages based on primary voltage and turns ratios
    // In digital simulation, we're modeling the transformation of signal states
    
    // Calculate the effect of primary on all secondary taps
    for (int i = 0; i < num_taps; i++) {
        if (num_taps > 0) {
            double turns_ratio = secondary_turns[i] / primary_turns;
            // Apply the transformation to each tap
            tap_states[i] = primary_in_state;  // Simplified: primary influences all taps based on turns ratio
        }
    }
    
    // The primary state may also be influenced by secondary taps (in real transformers)
    // For simplicity in digital simulation, we'll just update tap states
    
    return true;
}

bool TappedTransformer::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        byte temp_data[1] = {0};
        bool signal_pass = false;

        // If the connection is from primary, route to all secondary taps based on turns ratio
        if (conn_id == 0) {  // From PriIn
            // Send to all secondary taps with appropriate transformation
            for (int i = 0; i < num_taps; i++) {
                if (GetConnector(2 + 2*i + 1).IsConnected()) { // To Sec<i>Out connector
                    // Calculate transformed signal based on turns ratio
                    double turns_ratio = secondary_turns[i] / primary_turns;
                    // In digital terms: apply scaling based on ratio and coupling
                    temp_data[0] = (byte)(primary_in_state ? 1 : 0);
                    // Apply coupling coefficient effect
                    if (coupling_coefficient > 0.1) { // If sufficiently coupled
                        signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                    }
                }
            }
        }
        else if (conn_id == 1) {  // From PriOut
            // Similar to above but from other primary terminal
            for (int i = 0; i < num_taps; i++) {
                if (GetConnector(2 + 2*i).IsConnected()) { // To Sec<i>In connector
                    temp_data[0] = (byte)(primary_out_state ? 1 : 0);
                    if (coupling_coefficient > 0.1) {
                        signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                    }
                }
            }
        }
        // If the connection is from any secondary tap, route to primary
        else {
            int tap_index = (conn_id - 2) / 2;  // Calculate which tap
            if (tap_index >= 0 && tap_index < num_taps) {
                if ((conn_id - 2) % 2 == 0) {  // From Sec<i>In
                    // Route to primary with inverse transformation
                    if (GetConnector(1).IsConnected()) { // To PriOut
                        temp_data[0] = (byte)(tap_states[tap_index] ? 1 : 0);
                        if (coupling_coefficient > 0.1) {
                            signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                        }
                    }
                } else {  // From Sec<i>Out
                    // Route to primary with inverse transformation
                    if (GetConnector(0).IsConnected()) { // To PriIn
                        temp_data[0] = (byte)(tap_states[tap_index] ? 1 : 0);
                        if (coupling_coefficient > 0.1) {
                            signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                        }
                    }
                }
            }
        }

        return signal_pass;
    }

    return false;
}

bool TappedTransformer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Update the state of the appropriate terminal
    if (conn_id == 0) {  // PriIn
        primary_in_state = (*data & 1) ? true : false;
        // Update all tap voltages based on primary voltage and turns ratio
        for (int i = 0; i < num_taps; i++) {
            double turns_ratio = secondary_turns[i] / primary_turns;
            tap_voltages[i] = 0; // Placeholder - in digital terms we just transform the state
        }
        return true;
    }
    else if (conn_id == 1) {  // PriOut
        primary_out_state = (*data & 1) ? true : false;
        return true;
    }
    else {
        // Handle secondary tap connections
        int tap_index = (conn_id - 2) / 2;  // Calculate which tap
        if (tap_index >= 0 && tap_index < num_taps) {
            if ((conn_id - 2) % 2 == 0) {  // Sec<i>In
                tap_states[tap_index] = (*data & 1) ? true : false;
            } else {  // Sec<i>Out
                tap_states[tap_index] = (*data & 1) ? true : false;
            }
            return true;
        }
    }

    LOG("error: TappedTransformer: unimplemented conn-id " << conn_id);
    return false;
}