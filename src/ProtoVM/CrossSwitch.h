#ifndef _ProtoVM_CrossSwitch_h_
#define _ProtoVM_CrossSwitch_h_

#include "ProtoVM.h"

// Cross Switch component for telecommunications applications
// Provides crosspoint switching between multiple input and output lines
class CrossSwitch : public ElcBase {
private:
    int num_inputs;                    // Number of input lines
    int num_outputs;                   // Number of output lines
    Vector<Vector<bool>> crosspoint_matrix;  // Matrix indicating which input connects to which output
    Vector<bool> input_states;         // Current states of all inputs
    Vector<bool> output_states;        // Current states of all outputs
    bool control_enabled;              // Whether control inputs are enabled

public:
    // Constructor takes the number of inputs and outputs
    CrossSwitch(int num_inputs = 8, int num_outputs = 8);
    
    int GetNumInputs() const { return num_inputs; }
    int GetNumOutputs() const { return num_outputs; }
    
    // Set connection between input i and output j
    void SetConnection(int input, int output, bool connected);
    bool GetConnection(int input, int output) const;
    
    // Connect all inputs to corresponding outputs (e.g., input 0 to output 0, etc.)
    void SetThroughConnection();
    
    // Disconnect all connections
    void ClearAllConnections();
    
    // Enable/disable the switch
    void EnableControl() { control_enabled = true; }
    void DisableControl() { control_enabled = false; }
    bool IsControlEnabled() const { return control_enabled; }
    
    virtual bool Tick() override;
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
};

#endif