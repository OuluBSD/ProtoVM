#include "ProtoVM.h"

// Simple AND gate test
struct ANDGateTest : Chip {
    byte input_a = 0;
    byte input_b = 0;
    byte output = 0;
    
    ANDGateTest() {
        AddSink("A").SetRequired(false);      // Input A
        AddSink("B").SetRequired(false);      // Input B
        AddSource("Y").SetRequired(false);    // Output Y = A AND B
        AddSink("dummy").SetRequired(false); // Dummy pin to satisfy connectivity
    }
    
    bool Tick() override {
        output = (input_a && input_b) ? 1 : 0;
        SetChanged(true);
        LOG("ANDGateTest: A=" << (int)input_a << ", B=" << (int)input_b << " -> Y=" << (int)output);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE) {
            byte output_data = 0;
            switch (conn_id) {
                case 2: // Y output
                    output_data = output;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                default:
                    LOG("ANDGateTest: unhandled connection ID: " << conn_id);
                    return false;
            }
        }
        return false;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        switch (conn_id) {
            case 0: // Input A
                input_a = data ? *data : 0;
                break;
            case 1: // Input B
                input_b = data ? *data : 0;
                break;
            case 3: // Dummy pin - ignore
                break;
            default:
                LOG("ANDGateTest: unhandled input connection ID: " << conn_id);
                return true;  // Still return true to acknowledge
        }
        // Recalculate output based on new inputs
        output = (input_a && input_b) ? 1 : 0;
        SetChanged(true);
        return true;
    }
};

void SetupTest1_ANDGate(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    Pin& ground = b.Add<Pin>("ground").SetReference(0);
    
    ANDGateTest& gate = b.Add<ANDGateTest>("andgate");
    
    try {
        // Connect dummy pin to satisfy connectivity requirements
        ground >> gate["dummy"];
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}