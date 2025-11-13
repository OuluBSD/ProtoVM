#include "Potentiometer.h"

Potentiometer::Potentiometer(double resistance, double initial_position) 
    : total_resistance(resistance < 1.0 ? 1.0 : resistance),  // Minimum 1Ω resistance
      wiper_position(initial_position < 0.0 ? 0.0 : (initial_position > 1.0 ? 1.0 : initial_position)),
      terminal_A_state(false), terminal_B_state(false), terminal_W_state(false) {
    AddBidirectional("A");      // One end of the resistive element
    AddBidirectional("B");      // Other end of the resistive element
    AddBidirectional("W");      // Wiper terminal
    AddSink("Position");        // Control input for wiper position (0.0 to 1.0)
}

void Potentiometer::SetResistance(double r) {
    total_resistance = r < 1.0 ? 1.0 : r;  // Ensure minimum resistance
}

void Potentiometer::SetWiperPosition(double pos) {
    wiper_position = pos < 0.0 ? 0.0 : (pos > 1.0 ? 1.0 : pos);  // Clamp between 0.0 and 1.0
}

bool Potentiometer::Tick() {
    // Update internal state if needed based on wiper position and terminal states
    // In digital simulation, we model the potentiometer's effect on signal propagation
    // The wiper position affects how signals pass between terminals
    
    // The potentiometer in a digital context acts as a variable connection between terminals
    // depending on the wiper position, but for now we'll just return true
    return true;
}

bool Potentiometer::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        // Handle signal passing between terminals based on wiper position
        // In digital simulation, we'll implement simple switching behavior
        if (conn_id == 3) {  // Position control input
            // This would be handled by the control pin logic
            return true;
        }

        // For signal passing between terminals, we'll use the wiper position to determine
        // how signals are passed between A, B, and W (wiper) terminals
        // This is a simplified model for digital simulation

        byte temp_data[1] = {0};
        bool signal_pass = false;

        // Based on wiper position, determine signal routing
        if (conn_id == 0) {  // From A terminal
            // If wiper is closer to A (wiper_pos < 0.5), allow signal to W
            if (wiper_position < 0.5 && GetConnector(2).IsConnected()) {
                temp_data[0] = terminal_A_state ? 1 : 0;
                signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
            }
            // If wiper is at B end, also allow signal to B (when wiper_position = 1.0)
            else if (wiper_position > 0.5 && GetConnector(1).IsConnected()) {
                temp_data[0] = terminal_A_state ? 1 : 0;
                signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
            }
        }
        else if (conn_id == 1) {  // From B terminal
            // If wiper is closer to B (wiper_pos > 0.5), allow signal to W
            if (wiper_position > 0.5 && GetConnector(2).IsConnected()) {
                temp_data[0] = terminal_B_state ? 1 : 0;
                signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
            }
            // If wiper is at A end, also allow signal to A (when wiper_position = 0.0)
            else if (wiper_position < 0.5 && GetConnector(0).IsConnected()) {
                temp_data[0] = terminal_B_state ? 1 : 0;
                signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
            }
        }
        else if (conn_id == 2) {  // From W (wiper) terminal
            // Allow signal to both A and B terminals based on wiper position
            if (wiper_position < 1.0 && GetConnector(0).IsConnected()) {
                temp_data[0] = terminal_W_state ? 1 : 0;
                signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
            }
            if (wiper_position > 0.0 && GetConnector(1).IsConnected()) {
                temp_data[0] = terminal_W_state ? 1 : 0;
                signal_pass = dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
            }
        }

        return signal_pass;
    }

    return false;
}

bool Potentiometer::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Update the state of the terminal that received the input
    switch (conn_id) {
        case 0:  // Terminal A
            terminal_A_state = (*data & 1) ? true : false;
            return true;
        case 1:  // Terminal B
            terminal_B_state = (*data & 1) ? true : false;
            return true;
        case 2:  // Terminal W (wiper)
            terminal_W_state = (*data & 1) ? true : false;
            return true;
        case 3:  // Position control
            // Update the wiper position based on the input value (0-255 mapped to 0.0-1.0)
            wiper_position = (*data) / 255.0;
            return true;
        default:
            LOG("error: Potentiometer: unimplemented conn-id " << conn_id);
            return false;
    }
}