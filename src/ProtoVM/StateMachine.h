#ifndef _ProtoVM_StateMachine_h_
#define _ProtoVM_StateMachine_h_

#include "ProtoVM.h"

// State machine component for digital control logic
// Supports configurable states and transitions based on input conditions
class StateMachine : public ElcBase {
protected:
    // State representation
    int current_state = 0;           // Current state of the machine
    int next_state = 0;              // Next state (computed during tick)
    int num_states = 0;              // Total number of states
    bool is_mealy_machine = false;   // True for Mealy, false for Moore
    
    // Transition conditions (simplified implementation)
    // In a real implementation, this would use more complex condition structures
    Vector<Vector<byte>> transition_table;  // [from_state][to_state] -> condition/inputs
    Vector<byte> output_table;              // Output for each state
    
    // Input state (to track what the input is currently)
    byte input_state = 0;
    byte input_mask = 0;  // To know which inputs are used

public:
    StateMachine(int states_count = 2, bool mealy = false);
    
    // Override base class methods
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;

    // Configuration methods
    void SetTransition(int from_state, int to_state, byte condition);
    void SetOutputForState(int state, byte output);
    int GetCurrentState() const { return current_state; }
    void Reset() { current_state = 0; }
};

// Specialized FSM (Finite State Machine) component with clock and reset
class FsmController : public StateMachine {
private:
    bool clock = false;      // Current clock state
    bool reset = false;      // Reset signal
    bool last_clock = false; // Last clock state for edge detection

public:
    FsmController(int states_count = 2, bool mealy = false);
    
    bool Tick() override;
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif