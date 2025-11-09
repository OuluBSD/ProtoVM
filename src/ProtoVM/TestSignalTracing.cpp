#include "ProtoVM.h"
#include "Machine.h"
#include "Cli.h"

#include <Core/Core.h>
using namespace UPP;

// Test for signal tracing functionality
void TestSignalTracing() {
    LOG("Starting Signal Tracing Test...");

    // Create a simple test circuit with a counter and an ALU
    Machine mach;

    Pcb& pcb = mach.AddPcb();

    // Create a simple clock component to drive our circuit
    struct Clock : public ElcBase {
        int tick_count = 0;
        bool clock_state = false;
        
        Clock() {
            AddSource("CLK").SetMultiConn();
        }
        
        bool Tick() override {
            tick_count++;
            clock_state = !clock_state;  // Toggle every tick
            SetChanged(true);
            return true;
        }
        
        bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
            if (type == WRITE && conn_id == 0) {  // CLK output
                byte clk_bit = clock_state ? 1 : 0;
                return dest.PutRaw(dest_conn_id, &clk_bit, 0, 1);
            }
            return true;
        }
        
        bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
            return true;  // Clock doesn't accept inputs
        }
    };

    Clock& clock = pcb.Add<Clock>("TestClock");

    // Create a simple counter
    struct Counter : public ElcBase {
        int count = 0;
        
        Counter() {
            AddSink("CLK");
            AddSource("COUNT0").SetMultiConn();
            AddSource("COUNT1").SetMultiConn();
            AddSource("COUNT2").SetMultiConn();
            AddSource("COUNT3").SetMultiConn();
        }
        
        bool Tick() override {
            // Simple 4-bit counter - increment on clock rising edge
            // For this test, we'll increment every tick
            count = (count + 1) & 0x0F;  // 4-bit counter
            SetChanged(true);
            return true;
        }
        
        bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
            if (type == WRITE) {
                if (conn_id >= 1 && conn_id <= 4) {  // COUNT outputs 0-3
                    int bit_idx = conn_id - 1;
                    byte bit_val = (count >> bit_idx) & 1;
                    return dest.PutRaw(dest_conn_id, &bit_val, 0, 1);
                }
            }
            return true;
        }
        
        bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
            if (conn_id == 0) {  // CLK input
                // Could implement clock edge detection here
            }
            return true;
        }
    };

    Counter& counter = pcb.Add<Counter>("TestCounter");

    // Connect clock to counter
    clock["CLK"] >> counter["CLK"];

    // Create dummy sink components to connect the counter outputs to (to satisfy connectivity requirements)
    struct DummySink : public ElcBase {
        DummySink() {
            AddSink("IN").SetRequired(false);
        }
        
        bool Tick() override {
            return true;
        }
        
        bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
            return true;
        }
        
        bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
            // Receive data from counter outputs
            return true;
        }
    };

    DummySink& dummy0 = pcb.Add<DummySink>("Dummy0");
    DummySink& dummy1 = pcb.Add<DummySink>("Dummy1");
    DummySink& dummy2 = pcb.Add<DummySink>("Dummy2");
    DummySink& dummy3 = pcb.Add<DummySink>("Dummy3");

    // Connect counter outputs to dummy sinks to satisfy connectivity requirements
    counter["COUNT0"] >> dummy0["IN"];
    counter["COUNT1"] >> dummy1["IN"];
    counter["COUNT2"] >> dummy2["IN"];
    counter["COUNT3"] >> dummy3["IN"];

    // Initialize the machine
    if (!mach.Init()) {
        LOG("Failed to initialize machine for signal tracing test");
        return;
    }

    LOG("Machine initialized successfully for signal tracing test");

    // Use CLI to add signal traces
    Cli cli;
    cli.SetMachine(&mach);
    
    // Add a trace for the counter outputs using the public API
    LOG("Adding signal trace for TestCounter COUNT0");
    cli.AddSignalTrace("TestCounter", "COUNT0", 0);
    
    LOG("Adding signal trace for TestCounter COUNT1");
    cli.AddSignalTrace("TestCounter", "COUNT1", 0);

    // Run simulation for 10 ticks to generate some signal transitions
    LOG("Running simulation for 10 ticks to generate signal transitions...");
    for (int i = 0; i < 10; i++) {
        LOG("Processing tick " << i);
        if (!mach.Tick()) {
            LOG("Simulation failed at tick " << i);
            return;
        }
    }

    LOG("Simulation completed. Signal transitions recorded: " << mach.GetSignalTransitionCount());

    // Now show the signal log using the public API
    LOG("Displaying signal transition log:");
    cli.ShowSignalTraceLog();

    LOG("Signal tracing test completed.");
}

// Entry point for the test
void Test90_SignalTracing() {
    TestSignalTracing();
    LOG("Signal tracing test completed successfully.");
}