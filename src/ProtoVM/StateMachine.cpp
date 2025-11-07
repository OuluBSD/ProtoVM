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
    // Default behavior for StateMachine - immediate transitions based on inputs
    // Evaluate possible transitions from the current state
    
    next_state = current_state;  // Default to staying in the same state
    
    // Check if any transitions are possible from the current state
    for (int to_state = 0; to_state < num_states; to_state++) {
        // Check if there's a valid transition condition
        byte condition = transition_table[current_state][to_state];
        // If condition is not 0xFF (no transition), check if it matches input
        if (condition != 0xFF) {
            // Check if the input state matches the required condition
            // For now, using simple comparison - a more complex condition system could be implemented
            if ((input_state & input_mask) == (condition & input_mask)) {
                // Valid transition found
                next_state = to_state;
                break;  // Take the first valid transition
            }
        }
    }
    
    // Calculate state bits needed to determine if we need to propagate changes
    int state_bits = 1;
    int temp = num_states - 1;
    while (temp >>= 1) state_bits++;
    
    // Set changed flag if state output has changed
    bool changed = (current_state != next_state);
    
    // If there was a state change, update the current state 
    if (changed) {
        current_state = next_state;
    }
    
    SetChanged(changed);
    
    return true;
}

bool StateMachine::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Calculate state bits needed
        int state_bits = 1;
        int temp = num_states - 1;
        while (temp >>= 1) state_bits++;  // Calculate bits needed to represent states

        // Handle outputs
        if (conn_id >= 10 && conn_id < 10 + state_bits) {  // Q outputs Q0..Q(state_bits-1)
            int bit_index = conn_id - 10;
            byte bit_val = (current_state >> bit_index) & 1;
            return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
        }
        else if (conn_id == 10 + state_bits) {  // StateChange output
            // For now, this is just a simple indication that something changed
            byte changed = HasChanged() ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &changed, 0, 1);
        }
        else if (conn_id == 10 + state_bits + 1) {  // OUT output
            // Output based on current state (Moore machine) or current state + input (Mealy)
            byte output;
            if (is_mealy_machine) {
                // For Mealy machine, output depends on current state AND input
                // For simplicity, we'll just use the output for the current state
                output = output_table[current_state];
            } else {
                // For Moore machine, output depends only on current state
                output = output_table[current_state];
            }
            return dest.PutRaw(dest_conn_id, &output, 0, 1);
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
    else if (conn_id >= 2 && conn_id < 10) {  // IN inputs IN0..IN7 (8 input bits)
        int bit_index = conn_id - 2;
        byte input_bit = *data & 1;

        // Update input state based on the input bit
        if (input_bit) {
            input_state |= (1 << bit_index);
        } else {
            input_state &= ~(1 << bit_index);
        }
        
        // Update the input mask to indicate which inputs are used
        input_mask |= (1 << bit_index);
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
    // Detect clock edge (rising edge)
    bool clock_edge = (clock && !last_clock);
    last_clock = clock;

    // Apply reset if active
    if (reset) {
        current_state = 0;
        next_state = 0;
        return true;
    }

    // At each tick, evaluate possible transitions based on inputs
    // This computes the next_state but doesn't apply it until the clock edge
    int new_next = current_state;  // Default to staying in the same state
    
    // Check if any transitions are possible from the current state
    for (int to_state = 0; to_state < num_states; to_state++) {
        // Check if there's a valid transition condition
        byte condition = transition_table[current_state][to_state];
        // If condition is not 0xFF (no transition), check if it matches input
        if (condition != 0xFF) {
            // Check if the input state matches the required condition
            // For now, using simple comparison - a more complex condition system could be implemented
            if ((input_state & input_mask) == (condition & input_mask)) {
                // Valid transition found
                new_next = to_state;
                break;  // Take the first valid transition
            }
        }
    }
    
    next_state = new_next;

    // On clock edge, advance to next state
    if (clock_edge) {
        current_state = next_state;
    }

    // Calculate state bits needed to determine if we need to propagate changes
    int state_bits = 1;
    int temp = num_states - 1;
    while (temp >>= 1) state_bits++;
    
    // Set changed flag if state output has changed
    bool changed = (current_state != new_next);  // Will change on next clock edge
    SetChanged(changed);

    return true;
}

bool FsmController::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
    if (type == WRITE) {
        // Calculate state bits needed
        int state_bits = 1;
        int temp = num_states - 1;
        while (temp >>= 1) state_bits++;  // Calculate bits needed to represent states

        // Handle outputs
        if (conn_id >= 10 && conn_id < 10 + state_bits) {  // Q outputs Q0..Q(state_bits-1)
            int bit_index = conn_id - 10;
            byte bit_val = (current_state >> bit_index) & 1;
            return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
        }
        else if (conn_id == 10 + state_bits) {  // StateChange output
            // For now, this is just a simple indication that something changed
            byte changed = HasChanged() ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &changed, 0, 1);
        }
        else if (conn_id == 10 + state_bits + 1) {  // OUT output
            // Output based on current state (Moore machine) or current state + input (Mealy)
            byte output;
            if (is_mealy_machine) {
                // For Mealy machine, output depends on current state AND input
                // For simplicity, we'll just use the output for the current state
                output = output_table[current_state];
            } else {
                // For Moore machine, output depends only on current state
                output = output_table[current_state];
            }
            return dest.PutRaw(dest_conn_id, &output, 0, 1);
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
    else if (conn_id >= 2 && conn_id < 10) {  // IN inputs IN0..IN7 (8 input bits)
        int bit_index = conn_id - 2;
        byte input_bit = *data & 1;

        // Update input state based on the input bit
        if (input_bit) {
            input_state |= (1 << bit_index);
        } else {
            input_state &= ~(1 << bit_index);
        }
        
        // Update the input mask to indicate which inputs are used
        input_mask |= (1 << bit_index);
    }

    return true;
}