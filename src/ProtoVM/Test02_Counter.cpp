#include "ProtoVM.h"

// Simple 4-bit binary counter test
struct Counter4Bit : Chip {
    uint8 value = 0;
    bool prev_clk_state = false;
    bool has_changed = true;
    
    Counter4Bit() {
        AddSink("CLK").SetRequired(false);    // Clock input
        AddSource("Q0").SetRequired(false);   // Bit 0 output
        AddSource("Q1").SetRequired(false);   // Bit 1 output
        AddSource("Q2").SetRequired(false);   // Bit 2 output
        AddSource("Q3").SetRequired(false);   // Bit 3 output
        AddSink("RST").SetRequired(false);    // Reset input (active high)
        AddSink("dummy").SetRequired(false);  // Dummy pin for connectivity
    }
    
    bool Tick() override {
        // Check if any inputs changed in the previous tick
        // For now just logging the value
        LOG("Counter4Bit: Value = " << (int)value << ", has_changed = " << has_changed);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE) {
            byte output_data = 0;
            switch (conn_id) {
                case 1: // Q0 output
                    output_data = (value >> 0) & 1;
                    has_changed = true;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                case 2: // Q1 output
                    output_data = (value >> 1) & 1;
                    has_changed = true;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                case 3: // Q2 output
                    output_data = (value >> 2) & 1;
                    has_changed = true;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                case 4: // Q3 output
                    output_data = (value >> 3) & 1;
                    has_changed = true;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                default:
                    LOG("Counter4Bit: unhandled connection ID: " << conn_id);
                    return false;
            }
        }
        return false;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        bool input_val = data && (*data != 0);
        
        switch (conn_id) {
            case 0: { // CLK input
                // Look for rising edge (0 -> 1 transition)
                if (input_val && !prev_clk_state) {
                    // Rising clock edge detected - increment counter
                    value = (value + 1) & 0x0F; // 4-bit counter wraps at 16
                    LOG("Counter4Bit: Clock rising edge - incremented to " << (int)value);
                    has_changed = true;
                }
                prev_clk_state = input_val;
                break;
            }
            case 5: { // RST input
                if (input_val) {
                    // Reset is active high - reset the counter
                    if (value != 0) {
                        value = 0;
                        LOG("Counter4Bit: Reset signal received - reset to " << (int)value);
                        has_changed = true;
                    }
                }
                break;
            }
            case 6: // Dummy pin - ignore
                break;
            default:
                LOG("Counter4Bit: unhandled input connection ID: " << conn_id);
                return true;  // Still return true to acknowledge
        }
        
        SetChanged(has_changed);
        has_changed = false;  // Reset for next tick
        return true;
    }
};

// Clock generator for testing
struct CounterTestClockGen : Chip {
    int tick_count = 0;
    int clock_half_period = 2;  // Toggle every N ticks
    bool clock_state = false;
    
    void SetHalfPeriod(int half_period) {
        clock_half_period = half_period;
    }
    
    CounterTestClockGen() {
        AddSource("CLK_OUT").SetRequired(false);  // Clock output
    }
    
    bool Tick() override {
        // Toggle clock state every half_period ticks
        if (tick_count % clock_half_period == 0) {
            clock_state = !clock_state;
        }
        tick_count++;
        
        SetChanged(true);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE && conn_id == 0) { // CLK_OUT
            byte output_data = clock_state ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
        }
        return false;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        // ClockGen doesn't receive inputs, so just return true
        return true;
    }
};

void SetupTest2_Counter(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    Pin& ground = b.Add<Pin>("ground").SetReference(0);
    
    Counter4Bit& counter = b.Add<Counter4Bit>("counter");
    CounterTestClockGen& clk_gen = b.Add<CounterTestClockGen>("clk_gen");  // Add without constructor parameter
    clk_gen.SetHalfPeriod(2);  // Set half period to 2 ticks (so full period is 4 ticks)
    
    try {
        // Connect dummy pin to ground to satisfy connectivity requirements
        ground >> counter["dummy"];
        
        // Connect clock generator to counter's clock input
        clk_gen["CLK_OUT"] >> counter["CLK"];
        
        // Keep reset low (not active) to allow counting
        ground >> counter["RST"];
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}