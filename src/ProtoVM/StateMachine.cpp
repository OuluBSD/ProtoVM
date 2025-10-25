#include "ProtoVM.h"
#include "StateMachine.h"

StateMachine::StateMachine(int states_count, bool mealy) 
    : num_states(states_count), is_mealy_machine(mealy) {
    
    // Ensure we have at least 2 states
    if (num_states < 2) {
        num_states = 2;
    }
    
    // Calculate bits needed to represent states
    int state_bits = 1;
    int temp = num_states - 1;
    while (temp >>= 1) state_bits++;  // Calculate bits needed to represent states
    
    // Add input connectors
    AddSink("CLK");  // Clock input (for synchronous state machines)
    AddSink("RST");  // Reset input
    
    // Add input connectors for state selection and transition conditions
    for (int i = 0; i < state_bits; i++) {
        AddSink(String().Cat() << "D" << i);  // Data inputs for state selection
    }
    
    // Add output connectors for current state
    for (int i = 0; i < state_bits; i++) {
        AddSource(String().Cat() << "Q" << i).SetMultiConn();  // Current state output
    }
    
    // Add output for state change indication
    AddSource("StateChange").SetMultiConn();
    
    // Initialize transition and output tables
    transition_table.SetCount(num_states);
    for (int i = 0; i < num_states; i++) {
        transition_table[i].SetCount(num_states);
        for (int j = 0; j < num_states; j++) {
            transition_table[i][j] = 0;  // Initialize with no transitions
        }
    }
    
    output_table.SetCount(num_states);
    for (int i = 0; i < num_states; i++) {
        output_table[i] = 0;  // Initialize with zero output
    }
}

void StateMachine::SetTransition(int from_state, int to_state, byte condition) {
    if (from_state >= 0 && from_state < num_states && 
        to_state >= 0 && to_state < num_states) {
        transition_table[from_state][to_state] = condition;
    }
}

void StateMachine::SetOutputForState(int state, byte output) {
    if (state >= 0 && state < num_states) {
        output_table[state] = output;
    }
}

bool StateMachine::Tick() {
    return true;
}

bool StateMachine::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Calculate state bits needed
        int state_bits = 1;
        int temp = num_states - 1;
        while (temp >>= 1) state_bits++;  // Calculate bits needed to represent states
        
        // Handle outputs
        if (conn_id >= 2 + state_bits && conn_id < 2 + 2 * state_bits) {  // Q outputs Q0..Q(state_bits-1)
            int bit_index = conn_id - (2 + state_bits);
            byte bit_val = (current_state >> bit_index) & 1;
            return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
        }
        else if (conn_id == 2 + 2 * state_bits) {  // StateChange output
            // For now, this is just a simple indication that something changed
            byte changed = HasChanged() ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &changed, 0, 1);
        }
    }
    return true;
}

bool StateMachine::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    ASSERT(data_bytes == 0 && data_bits == 1);
    
    // Calculate state bits needed
    int state_bits = 1;
    int temp = num_states - 1;
    while (temp >>= 1) state_bits++;  // Calculate bits needed to represent states
    
    if (conn_id == 0) {  // CLK input
        // Clock input - handled specially in derived classes
    }
    else if (conn_id == 1) {  // RST input
        byte reset_val = *data & 1;
        if (reset_val) {
            current_state = 0;
            next_state = 0;
        }
    }
    else if (conn_id >= 2 && conn_id < 2 + state_bits) {  // D inputs D0..D(state_bits-1)
        int bit_index = conn_id - 2;
        byte input_bit = *data & 1;
        
        // For a simple state machine, we'll use input to set next state
        // This is a simplified implementation
        if (bit_index < 8) {  // Safety check
            if (input_bit) {
                next_state |= (1 << bit_index);
            } else {
                next_state &= ~(1 << bit_index);
            }
            
            // Ensure next_state is within range
            if (next_state >= num_states) {
                next_state = num_states - 1;
            }
        }
    }
    
    return true;
}

// FsmController implementation
FsmController::FsmController(int states_count, bool mealy) 
    : StateMachine(states_count, mealy) {
    // Additional connectors for the controller
    SetName("FsmController");
}

bool FsmController::Tick() {
    // Detect clock edge
    bool clock_edge = (clock && !last_clock);
    
    last_clock = clock;
    
    // Apply reset if active
    if (reset) {
        current_state = 0;
        next_state = 0;
        return true;
    }
    
    // On clock edge, advance to next state
    if (clock_edge) {
        current_state = next_state;
    }
    
    return true;
}

bool FsmController::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Calculate state bits needed
        int state_bits = 1;
        int temp = num_states - 1;
        while (temp >>= 1) state_bits++;  // Calculate bits needed to represent states
        
        // Handle outputs
        if (conn_id >= 2 + state_bits && conn_id < 2 + 2 * state_bits) {  // Q outputs Q0..Q(state_bits-1)
            int bit_index = conn_id - (2 + state_bits);
            byte bit_val = (current_state >> bit_index) & 1;
            return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
        }
        else if (conn_id == 2 + 2 * state_bits) {  // StateChange output
            // For now, this is just a simple indication that something changed
            byte changed = HasChanged() ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &changed, 0, 1);
        }
    }
    return true;
}

bool FsmController::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
    ASSERT(data_bytes == 0 && data_bits == 1);
    
    // Calculate state bits needed
    int state_bits = 1;
    int temp = num_states - 1;
    while (temp >>= 1) state_bits++;  // Calculate bits needed to represent states
    
    if (conn_id == 0) {  // CLK input
        clock = *data & 1;
    }
    else if (conn_id == 1) {  // RST input
        reset = *data & 1;
        if (reset) {
            current_state = 0;
            next_state = 0;
        }
    }
    else if (conn_id >= 2 && conn_id < 2 + state_bits) {  // D inputs D0..D(state_bits-1)
        int bit_index = conn_id - 2;
        byte input_bit = *data & 1;
        
        // For a controller, these inputs could set the next state or trigger transitions
        if (bit_index < 8) {  // Safety check
            if (input_bit) {
                next_state |= (1 << bit_index);
            } else {
                next_state &= ~(1 << bit_index);
            }
            
            // Ensure next_state is within range
            if (next_state >= num_states) {
                next_state = num_states - 1;
            }
        }
    }
    
    return true;
}