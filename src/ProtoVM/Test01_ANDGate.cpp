#include "ProtoVM.h"

// Clock generator for testing - renamed to avoid conflicts
struct ANDGateTestClockGen : Chip {
    int tick_count = 0;
    int clock_half_period = 2;  // Toggle every N ticks
    
    void SetHalfPeriod(int half_period) {
        clock_half_period = half_period;
    }
    
    ANDGateTestClockGen() {
        AddSource("CLK_OUT").SetRequired(false);  // Clock output
    }
    
    bool Tick() override {
        // Toggle clock state every half_period ticks
        if (tick_count % clock_half_period == 0) {
            // For this simple implementation, we'll alternate more predictably
            // This will be called each simulation tick
        }
        tick_count++;
        
        // For simple alternating pattern, we'll return different values based on tick_count
        SetChanged(true);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE && conn_id == 0) { // CLK_OUT
            // Generate output based on our internal counter - simple alternating
            byte output_data = ((tick_count / clock_half_period) % 2) ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
        }
        return false;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        // ClockGen doesn't receive inputs, so just return true
        return true;
    }
};

// Simple D Flip-Flop test component
struct DFlipFlop : Chip {
    bool q = false;      // Q output
    bool q_bar = true;   // Q-bar output (inverted)
    bool d = false;      // D input
    bool clk = false;    // Clock input
    bool prev_clk = false; // Previous clock state for edge detection
    
    DFlipFlop() {
        AddSink("D").SetRequired(false);      // Data input
        AddSink("CLK").SetRequired(false);    // Clock input
        AddSource("Q").SetRequired(false);    // Q output
        AddSource("Q_BAR").SetRequired(false); // Q-bar output
        AddSink("dummy").SetRequired(false);  // Dummy pin to satisfy connectivity
    }
    
    bool Tick() override {
        // Detect rising edge on clock
        if (clk && !prev_clk) {
            // On rising clock edge, update Q with D value
            q = d;
            q_bar = !d;
            LOG("DFlipFlop: Rising edge - D=" << d << ", Q=" << q << ", Q_BAR=" << q_bar);
        }
        prev_clk = clk;
        SetChanged(true);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE) {
            byte output_data = 0;
            switch (conn_id) {
                case 2: // Q output
                    output_data = q ? 1 : 0;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                case 3: // Q_BAR output
                    output_data = q_bar ? 1 : 0;
                    return dest.PutRaw(dest_conn_id, &output_data, 1, 0);
                default:
                    LOG("DFlipFlop: unhandled connection ID: " << conn_id);
                    return false;
            }
        }
        return false;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        switch (conn_id) {
            case 0: // D input
                d = data ? (*data != 0) : false;
                break;
            case 1: // CLK input
                clk = data ? (*data != 0) : false;
                break;
            case 4: // Dummy pin - ignore
                break;
            default:
                LOG("DFlipFlop: unhandled input connection ID: " << conn_id);
                return true;  // Still return true to acknowledge
        }
        SetChanged(true);
        return true;
    }
};

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
    
    Pin& ground = b.Add<Pin>("ground").SetReference(0);  // Logic 0
    Pin& vcc = b.Add<Pin>("vcc").SetReference(1);        // Logic 1
    
    // Create D flip-flops to provide changing inputs
    DFlipFlop& ff1 = b.Add<DFlipFlop>("ff1");
    
    // Create two AND gates to test
    ANDGateTest& gate1 = b.Add<ANDGateTest>("andgate1");
    ANDGateTest& gate2 = b.Add<ANDGateTest>("andgate2");
    
    // Create clock generators with different periods
    ANDGateTestClockGen& clk_gen1 = b.Add<ANDGateTestClockGen>("clk_gen1");
    clk_gen1.SetHalfPeriod(2);  // Fast clock (period 4)
    
    ANDGateTestClockGen& clk_gen2 = b.Add<ANDGateTestClockGen>("clk_gen2");
    clk_gen2.SetHalfPeriod(3);  // Slower clock (period 6)
    
    try {
        // Connect different clocks to the flip-flop and one AND gate
        clk_gen1["CLK_OUT"] >> ff1["CLK"];
        
        // Set up the flip-flop with a constant HIGH input
        vcc >> ff1["D"];
        
        // Connect clock and VCC to second AND gate inputs directly
        // For gate1: use flip-flop output and a direct VCC
        ff1["Q"] >> gate1["A"];    // Q from FF (will change from 0->1 at first tick)
        vcc >> gate1["B"];         // Constant HIGH
        
        // For gate2: use both clock signals
        clk_gen1["CLK_OUT"] >> gate2["A"];  // Fast clock signal
        clk_gen2["CLK_OUT"] >> gate2["B"];  // Slower clock signal
        
        // Connect dummy pins to ground to satisfy connectivity requirements
        ground >> ff1["dummy"];
        ground >> gate1["dummy"];
        ground >> gate2["dummy"];
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}