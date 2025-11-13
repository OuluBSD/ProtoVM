#include "CustomTransformer.h"

CustomTransformer::CustomTransformer(double primary_inductance, double secondary_inductance, 
                                     double coupling_coefficient, double primary_resistance,
                                     double secondary_resistance, double frequency)
    : primary_inductance(primary_inductance < 1e-9 ? 1e-9 : primary_inductance),
      secondary_inductance(secondary_inductance < 1e-9 ? 1e-9 : secondary_inductance),
      coupling_coefficient(coupling_coefficient < 0.0 ? 0.0 : (coupling_coefficient > 1.0 ? 1.0 : coupling_coefficient)),
      primary_resistance(primary_resistance < 0.001 ? 0.001 : primary_resistance),
      secondary_resistance(secondary_resistance < 0.001 ? 0.001 : secondary_resistance),
      frequency(frequency < 0.1 ? 0.1 : frequency),
      primary_current(0.0), secondary_current(0.0),
      primary_in_state(false), primary_out_state(false),
      secondary_in_state(false), secondary_out_state(false) {
    
    // Calculate mutual inductance from coupling coefficient
    mutual_inductance = coupling_coefficient * sqrt(primary_inductance * secondary_inductance);
    
    // Add primary connections
    AddBidirectional("PriIn");
    AddBidirectional("PriOut");
    
    // Add secondary connections
    AddBidirectional("SecIn");
    AddBidirectional("SecOut");
}

void CustomTransformer::SetPrimaryInductance(double L) {
    primary_inductance = L < 1e-9 ? 1e-9 : L;
    // Update mutual inductance based on new primary inductance
    mutual_inductance = coupling_coefficient * sqrt(primary_inductance * secondary_inductance);
}

void CustomTransformer::SetSecondaryInductance(double L) {
    secondary_inductance = L < 1e-9 ? 1e-9 : L;
    // Update mutual inductance based on new secondary inductance
    mutual_inductance = coupling_coefficient * sqrt(primary_inductance * secondary_inductance);
}

void CustomTransformer::SetMutualInductance(double M) {
    mutual_inductance = M;
    // Update coupling coefficient to reflect the new mutual inductance
    if (sqrt(primary_inductance * secondary_inductance) > 0) {
        coupling_coefficient = M / sqrt(primary_inductance * secondary_inductance);
        // Clamp coupling coefficient to valid range
        if (coupling_coefficient > 1.0) coupling_coefficient = 1.0;
        if (coupling_coefficient < 0.0) coupling_coefficient = 0.0;
    }
}

void CustomTransformer::SetCouplingCoefficient(double coeff) {
    coupling_coefficient = coeff < 0.0 ? 0.0 : (coeff > 1.0 ? 1.0 : coeff);
    // Update mutual inductance based on new coupling coefficient
    mutual_inductance = coupling_coefficient * sqrt(primary_inductance * secondary_inductance);
}

void CustomTransformer::SetPrimaryResistance(double R) {
    primary_resistance = R < 0.001 ? 0.001 : R;
}

void CustomTransformer::SetSecondaryResistance(double R) {
    secondary_resistance = R < 0.001 ? 0.001 : R;
}

void CustomTransformer::SetFrequency(double freq) {
    frequency = freq < 0.1 ? 0.1 : freq;
}

bool CustomTransformer::Tick() {
    // Calculate reactances based on frequency
    double primary_reactance = 2.0 * M_PI * frequency * primary_inductance;
    double secondary_reactance = 2.0 * M_PI * frequency * secondary_inductance;
    
    // Calculate impedances
    double primary_impedance = sqrt(primary_resistance*primary_resistance + primary_reactance*primary_reactance);
    double secondary_impedance = sqrt(secondary_resistance*secondary_resistance + secondary_reactance*secondary_reactance);
    
    // In digital simulation, we approximate the transformer behavior
    // The signal at primary affects secondary and vice versa through mutual inductance
    if (primary_in_state != primary_out_state) {
        // There's a signal change on the primary side
        // Transform this to the secondary side based on turns ratio, coupling, and impedances
        double turns_ratio = sqrt(secondary_inductance / primary_inductance);  // Approximation
        secondary_out_state = (primary_in_state && (coupling_coefficient > 0.1)) ? true : false;
    }
    
    if (secondary_in_state != secondary_out_state) {
        // There's a signal change on the secondary side
        // Transform this to the primary side based on inverse turns ratio, coupling, and impedances
        double turns_ratio = sqrt(primary_inductance / secondary_inductance);  // Approximation
        primary_out_state = (secondary_in_state && (coupling_coefficient > 0.1)) ? true : false;
    }
    
    return true;
}

bool CustomTransformer::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        byte temp_data[1] = {0};
        bool signal_pass = false;

        // Calculate effective turns ratio based on inductances
        double effective_turns_ratio = sqrt(secondary_inductance / primary_inductance);
        
        // From Primary Input to Secondary Output
        if (conn_id == 0) {  // From PriIn
            if (GetConnector(3).IsConnected()) { // To SecOut
                temp_data[0] = (byte)(primary_in_state ? 1 : 0);
                // Apply coupling coefficient and impedance matching effect
                if (coupling_coefficient > 0.1) {
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

bool CustomTransformer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Update the state of the appropriate terminal
    switch (conn_id) {
        case 0:  // PriIn
            primary_in_state = (*data & 1) ? true : false;
            return true;
        case 1:  // PriOut
            primary_out_state = (*data & 1) ? true : false;
            return true;
        case 2:  // SecIn
            secondary_in_state = (*data & 1) ? true : false;
            return true;
        case 3:  // SecOut
            secondary_out_state = (*data & 1) ? true : false;
            return true;
        default:
            LOG("error: CustomTransformer: unimplemented conn-id " << conn_id);
            return false;
    }
}