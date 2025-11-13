#include "Transformer.h"

Transformer::Transformer(double turns_ratio, double coupling_coefficient) 
    : turns_ratio(turns_ratio),
      coupling_coefficient(coupling_coefficient < 0.0 ? 0.0 : (coupling_coefficient > 1.0 ? 1.0 : coupling_coefficient)),
      primary_voltage(0.0), secondary_voltage(0.0),
      primary_in_state(false), primary_out_state(false),
      secondary_in_state(false), secondary_out_state(false) {
    // Primary side connections
    AddBidirectional("PriIn");   // Primary input
    AddBidirectional("PriOut");  // Primary output
    
    // Secondary side connections
    AddBidirectional("SecIn");   // Secondary input
    AddBidirectional("SecOut");  // Secondary output
}

void Transformer::SetTurnsRatio(double ratio) {
    turns_ratio = ratio;
}

void Transformer::SetCouplingCoefficient(double coeff) {
    coupling_coefficient = coeff < 0.0 ? 0.0 : (coeff > 1.0 ? 1.0 : coeff);
}

bool Transformer::Tick() {
    // Calculate secondary voltage based on primary voltage and turns ratio
    // In a digital simulation, we're modeling the transformation of signal states
    // rather than actual voltages, but we maintain the conceptual relationship
    
    // In digital terms: if the primary side has a signal transition, 
    // the secondary side should reflect this with the appropriate transformation
    if (primary_in_state != primary_out_state) {
        // There's a signal change on the primary side
        // Transform this to the secondary side based on turns ratio and coupling
        secondary_out_state = primary_in_state;  // Simplified: pass through with coupling effect
    } else {
        secondary_out_state = secondary_in_state;  // Maintain secondary state
    }
    
    // Also transform secondary to primary (bidirectional effect)
    if (secondary_in_state != secondary_out_state) {
        // There's a signal change on the secondary side
        // Transform this to the primary side based on inverse turns ratio and coupling
        primary_out_state = secondary_in_state;  // Simplified: pass through with coupling effect
    } else {
        primary_out_state = primary_in_state;  // Maintain primary state
    }
    
    return true;
}

bool Transformer::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        // Handle signal transformation between primary and secondary sides
        byte temp_data[1] = {0};
        bool signal_pass = false;

        // From Primary Input to Secondary Output
        if (conn_id == 0) {  // From PriIn
            if (GetConnector(3).IsConnected()) { // To SecOut
                // Apply transformation: voltage scales by turns ratio
                temp_data[0] = (byte)(primary_in_state ? 1 : 0);
                // Apply coupling coefficient effect
                if (coupling_coefficient > 0.1) { // If sufficiently coupled
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
        }
        // From Primary Output to Secondary Input
        else if (conn_id == 1) {  // From PriOut
            if (GetConnector(2).IsConnected()) { // To SecIn
                temp_data[0] = (byte)(primary_out_state ? 1 : 0);
                if (coupling_coefficient > 0.1) {
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
        }
        // From Secondary Input to Primary Output
        else if (conn_id == 2) {  // From SecIn
            if (GetConnector(1).IsConnected()) { // To PriOut
                temp_data[0] = (byte)(secondary_in_state ? 1 : 0);
                if (coupling_coefficient > 0.1) {
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
        }
        // From Secondary Output to Primary Input
        else if (conn_id == 3) {  // From SecOut
            if (GetConnector(0).IsConnected()) { // To PriIn
                temp_data[0] = (byte)(secondary_out_state ? 1 : 0);
                if (coupling_coefficient > 0.1) {
                    signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
                }
            }
        }

        return signal_pass;
    }

    return false;
}

bool Transformer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Update the state of the appropriate terminal
    switch (conn_id) {
        case 0:  // PriIn
            primary_in_state = (*data & 1) ? true : false;
            // Calculate the effect on secondary based on turns ratio
            secondary_voltage = primary_voltage * turns_ratio * coupling_coefficient;
            return true;
        case 1:  // PriOut
            primary_out_state = (*data & 1) ? true : false;
            return true;
        case 2:  // SecIn
            secondary_in_state = (*data & 1) ? true : false;
            // Calculate the effect on primary based on inverse turns ratio
            primary_voltage = secondary_voltage / turns_ratio * coupling_coefficient;
            return true;
        case 3:  // SecOut
            secondary_out_state = (*data & 1) ? true : false;
            return true;
        default:
            LOG("error: Transformer: unimplemented conn-id " << conn_id);
            return false;
    }
}