#include "ProtoVM.h"

// Simple 4-bit binary counter test
struct Counter4Bit : Chip {
    uint8 value = 0;
    bool clk_state = false;
    
    Counter4Bit() {
        AddSink("CLK").SetRequired(false);    // Clock input
        AddSource("Q0").SetRequired(false);   // Bit 0 output
        AddSource("Q1").SetRequired(false);   // Bit 1 output
        AddSource("Q2").SetRequired(false);   // Bit 2 output
        AddSource("Q3").SetRequired(false);   // Bit 3 output
        AddSink("dummy").SetRequired(false);  // Dummy pin for connectivity
    }
    
    bool Tick() override {
        LOG("Counter4Bit: Value = " << (int)value);
        SetChanged(true);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE) {
            byte output_data = 0;
            switch (conn_id) {
                case 1: // Q0 output
                    output_data = (value >> 0) & 1;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                case 2: // Q1 output
                    output_data = (value >> 1) & 1;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                case 3: // Q2 output
                    output_data = (value >> 2) & 1;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                case 4: // Q3 output
                    output_data = (value >> 3) & 1;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                default:
                    LOG("Counter4Bit: unhandled connection ID: " << conn_id);
                    return false;
            }
        }
        return false;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        switch (conn_id) {
            case 0: { // CLK input
                bool new_clk_state = data && (*data != 0);
                if (new_clk_state && !clk_state) {
                    // Rising edge detected - increment counter
                    value = (value + 1) & 0xF; // 4-bit counter wraps at 16
                    LOG("Counter4Bit: Incremented to " << (int)value);
                }
                clk_state = new_clk_state;
                break;
            }
            case 5: // Dummy pin - ignore
                break;
            default:
                LOG("Counter4Bit: unhandled input connection ID: " << conn_id);
                return true;  // Still return true to acknowledge
        }
        SetChanged(true);
        return true;
    }
};

void SetupTest2_Counter(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    Pin& ground = b.Add<Pin>("ground").SetReference(0);
    
    Counter4Bit& counter = b.Add<Counter4Bit>("counter");
    
    try {
        // Connect dummy pin to satisfy connectivity requirements
        ground >> counter["dummy"];
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}