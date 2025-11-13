#include "Memristor.h"

Memristor::Memristor(double min_resistance, double max_resistance, 
                     double initial_state, double memristance_coefficient)
    : min_resistance(min_resistance < 0.1 ? 0.1 : min_resistance),
      max_resistance(max_resistance > min_resistance ? max_resistance : min_resistance * 10),
      current_resistance(min_resistance + (max_resistance - min_resistance) * initial_state),
      state_variable(initial_state < 0.0 ? 0.0 : (initial_state > 1.0 ? 1.0 : initial_state)),
      memristance_coefficient(memristance_coefficient < 0.001 ? 0.001 : memristance_coefficient),
      charge_history(0.0),
      delta_flux(0.0),
      delta_charge(0.0),
      terminal_A_state(false), terminal_B_state(false) {
    
    AddBidirectional("A");    // One terminal
    AddBidirectional("B");    // Other terminal
}

void Memristor::SetMinResistance(double r) {
    min_resistance = r < 0.1 ? 0.1 : r;
    // Ensure max resistance is always greater than min
    if (min_resistance >= max_resistance) {
        max_resistance = min_resistance * 10.0;
    }
    // Update current resistance based on state
    current_resistance = min_resistance + (max_resistance - min_resistance) * state_variable;
}

void Memristor::SetMaxResistance(double r) {
    max_resistance = r > min_resistance ? r : min_resistance * 10;
    // Update current resistance based on state
    current_resistance = min_resistance + (max_resistance - min_resistance) * state_variable;
}

void Memristor::SetMemristanceCoefficient(double coeff) {
    memristance_coefficient = coeff < 0.001 ? 0.001 : coeff;
}

bool Memristor::Tick() {
    // Update the memristor's state based on the voltage across it and current through it
    
    // Calculate the "voltage" across the memristor in digital terms
    // This is a simplified model since we're in digital domain
    bool voltage_polarity = terminal_A_state != terminal_B_state;
    int voltage_level = terminal_A_state && !terminal_B_state ? 1 : 
                       (!terminal_A_state && terminal_B_state ? -1 : 0);
    
    // Calculate "current" through the memristor based on the voltage and current resistance
    // In digital terms, current flows when there's a voltage difference
    double current = 0.0;
    if (voltage_level != 0) {
        current = voltage_level * 1.0 / current_resistance;  // Simplified Ohm's law
    }
    
    // Update charge history
    delta_charge = current;  // In our digital model, we'll consider each tick as a unit of time
    charge_history += delta_charge;
    
    // Update state variable based on charge flow
    // The state changes proportionally to the current and the memristance coefficient
    state_variable += memristance_coefficient * current;
    
    // Clamp state variable between 0 and 1
    if (state_variable > 1.0) state_variable = 1.0;
    if (state_variable < 0.0) state_variable = 0.0;
    
    // Update resistance based on the new state
    current_resistance = min_resistance + (max_resistance - min_resistance) * state_variable;
    
    return true;
}

bool Memristor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        byte temp_data[1] = {0};
        bool signal_pass = false;

        // For signal passing, we consider the current resistance (history-dependent)
        // In digital simulation, signals pass based on the current memristance
        // The memristor in low resistance state allows signals to pass more easily
        if (current_resistance <= (min_resistance + (max_resistance - min_resistance) * 0.7)) {
            // Low to medium resistance state - allow signal to pass
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
        // In high resistance state, still allow digital signals to pass 
        // (though in a real memristor, this would be more limited)
        else {
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

bool Memristor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    switch (conn_id) {
        case 0:  // Terminal A
            terminal_A_state = (*data & 1) ? true : false;
            return true;
        case 1:  // Terminal B
            terminal_B_state = (*data & 1) ? true : false;
            return true;
        default:
            LOG("error: Memristor: unimplemented conn-id " << conn_id);
            return false;
    }
}