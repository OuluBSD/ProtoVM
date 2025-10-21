#include "ProtoVM.h"

// Simple flip-flop test - toggles state every tick
struct FlipFlopTest : Chip {
    bool state = false;
    
    FlipFlopTest() {
        // Add a dummy sink pin to satisfy the connection checker
        AddSink("dummy").SetRequired(false);
    }
    
    bool Tick() override {
        // Toggle state on each tick - this simulates a simple T flip-flop
        state = !state;
        SetChanged(true);
        LOG("FlipFlopTest: Toggled state to " << (state ? "HIGH" : "LOW"));
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        // The dummy pin may be used in process operations - we can ignore it
        LOG("FlipFlopTest: Process called");
        return true;  // Return true to acknowledge successful processing
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        // The dummy pin receives data from ground - we can ignore it
        // We just need to acknowledge the write to make the system happy
        LOG("FlipFlopTest: Received data on dummy pin");
        return true;  // Return true to acknowledge successful receipt
    }
};

void SetupTest0_FlipFlop(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    FlipFlopTest& ff = b.Add<FlipFlopTest>("flipflop");
    
    try {
        // Connect the dummy sink pin to ground (source) to satisfy connectivity requirements
        Pin& ground = b.Add<Pin>("ground").SetReference(0);  // is_high = 0 => acts as source
        ground >> ff["dummy"];  // source >> sink
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}