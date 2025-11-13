#include "CrossSwitch.h"

CrossSwitch::CrossSwitch(int num_inputs, int num_outputs)
    : num_inputs(num_inputs < 2 ? 2 : num_inputs),
      num_outputs(num_outputs < 2 ? 2 : num_outputs),
      crosspoint_matrix(),
      input_states(),
      output_states(),
      control_enabled(true) {
    
    // Initialize the crosspoint matrix
    crosspoint_matrix.SetCount(num_inputs);
    for (int i = 0; i < num_inputs; i++) {
        crosspoint_matrix[i].SetCount(num_outputs);
        for (int j = 0; j < num_outputs; j++) {
            crosspoint_matrix[i][j] = (i == j);  // Diagonal connection by default
        }
    }
    
    // Initialize input and output states
    input_states.SetCount(num_inputs);
    output_states.SetCount(num_outputs);
    for (int i = 0; i < num_inputs; i++) {
        input_states[i] = false;
    }
    for (int i = 0; i < num_outputs; i++) {
        output_states[i] = false;
    }
    
    // Add input connections
    for (int i = 0; i < num_inputs; i++) {
        AddSink("In" + IntStr(i));
    }
    
    // Add output connections
    for (int i = 0; i < num_outputs; i++) {
        AddSource("Out" + IntStr(i)).SetMultiConn();
    }
    
    // Add control connections (for setting which input connects to which output)
    // For simplicity, we'll have separate control lines for each crosspoint
    // (In a real implementation, this might use address/data buses)
    for (int i = 0; i < num_inputs; i++) {
        for (int j = 0; j < num_outputs; j++) {
            AddSink("Ctrl_" + IntStr(i) + "_" + IntStr(j));
        }
    }
}

void CrossSwitch::SetConnection(int input, int output, bool connected) {
    if (input >= 0 && input < num_inputs && output >= 0 && output < num_outputs) {
        crosspoint_matrix[input][output] = connected;
    }
}

bool CrossSwitch::GetConnection(int input, int output) const {
    if (input >= 0 && input < num_inputs && output >= 0 && output < num_outputs) {
        return crosspoint_matrix[input][output];
    }
    return false;
}

void CrossSwitch::SetThroughConnection() {
    ClearAllConnections();
    int min_count = (num_inputs < num_outputs) ? num_inputs : num_outputs;
    for (int i = 0; i < min_count; i++) {
        crosspoint_matrix[i][i] = true;
    }
}

void CrossSwitch::ClearAllConnections() {
    for (int i = 0; i < num_inputs; i++) {
        for (int j = 0; j < num_outputs; j++) {
            crosspoint_matrix[i][j] = false;
        }
    }
}

bool CrossSwitch::Tick() {
    if (!control_enabled) {
        return true;
    }
    
    // Calculate output states based on input states and the crosspoint matrix
    for (int j = 0; j < num_outputs; j++) {
        bool output_val = false;
        // An output is high if any connected input is high
        for (int i = 0; i < num_inputs; i++) {
            if (crosspoint_matrix[i][j] && input_states[i]) {
                output_val = true;
                break;  // In a simple OR configuration
            }
        }
        output_states[j] = output_val;
    }
    
    return true;
}

bool CrossSwitch::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == ProcessType::TICK) {
        return Tick();
    }

    if (type == ProcessType::WRITE) {
        // Handle output connections
        if (conn_id >= num_inputs && conn_id < num_inputs + num_outputs) {
            // This is an output connection
            int output_idx = conn_id - num_inputs;
            if (output_idx < num_outputs) {
                return dest.PutRaw(dest_conn_id, (byte*)&output_states[output_idx], bytes, bits);
            }
        }
        // Control connections are handled by PutRaw
    }

    return false;
}

bool CrossSwitch::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    // Handle input connections
    if (conn_id < num_inputs) {
        // This is an input connection
        int input_idx = conn_id;
        input_states[input_idx] = (*data & 1) ? true : false;
        return true;
    }
    // Handle control connections
    else if (conn_id >= num_inputs + num_outputs) {
        // Calculate which control connection this is
        int ctrl_idx = conn_id - (num_inputs + num_outputs);
        // Control connections are in sequential order: input0_output0, input0_output1, ..., input1_output0, etc.
        int input_num = ctrl_idx / num_outputs;
        int output_num = ctrl_idx % num_outputs;
        
        if (input_num < num_inputs && output_num < num_outputs) {
            bool connected = (*data & 1) ? true : false;
            crosspoint_matrix[input_num][output_num] = connected;
            return true;
        }
    }

    LOG("error: CrossSwitch: unimplemented conn-id " << conn_id);
    return false;
}