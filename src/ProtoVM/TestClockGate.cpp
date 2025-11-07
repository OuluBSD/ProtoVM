#include "ProtoVM.h"
#include "ClockGate.h"

// Test for the ClockGate component
void TestClockGate() {
    LOG("Starting ClockGate Test...");
    
    // Create a clock gate
    ClockGate clkGate;
    clkGate.SetName("TestClockGate");

    // Test initial state
    LOG("Initial state:");
    LOG("  Input clock: " << clkGate.GetInputClock());
    LOG("  Enable signal: " << clkGate.GetEnableSignal());
    LOG("  Output clock: " << clkGate.GetOutputClock());

    // Simulate various scenarios
    byte input_clock = 0;
    byte enable = 0;
    
    LOG("Test 1: Enable = 0 (clock gating enabled - should block clock)");
    enable = 0;
    clkGate.PutRaw(1, &enable, 0, 1);  // Set EN = 0
    
    for (int i = 0; i < 6; i++) {
        input_clock = i % 2;  // Alternate input clock
        clkGate.PutRaw(0, &input_clock, 0, 1);  // Set CLK_IN
        clkGate.Tick();
        
        LOG("  Step " << i << ": In=" << (int)input_clock 
             << ", EN=" << (int)enable 
             << ", Out=" << (int)clkGate.GetOutputClock());
    }
    
    LOG("\\nTest 2: Enable = 1 (clock gating disabled - should pass clock)");
    enable = 1;
    clkGate.PutRaw(1, &enable, 0, 1);  // Set EN = 1
    
    for (int i = 0; i < 6; i++) {
        input_clock = i % 2;  // Alternate input clock
        clkGate.PutRaw(0, &input_clock, 0, 1);  // Set CLK_IN
        clkGate.Tick();
        
        LOG("  Step " << i << ": In=" << (int)input_clock 
             << ", EN=" << (int)enable 
             << ", Out=" << (int)clkGate.GetOutputClock());
    }
    
    LOG("\\nTest 3: Transition from enabled to disabled");
    // Start with clock enabled
    enable = 1;
    input_clock = 1;  // High
    clkGate.PutRaw(1, &enable, 0, 1);  // EN = 1
    clkGate.PutRaw(0, &input_clock, 0, 1);  // CLK_IN = 1
    clkGate.Tick();
    LOG("  Before disable - In=" << (int)input_clock << ", EN=" << (int)enable << ", Out=" << (int)clkGate.GetOutputClock());
    
    // Now disable the clock gate
    enable = 0;
    clkGate.PutRaw(1, &enable, 0, 1);  // EN = 0 (should gate the clock)
    // Change input clock to test if it's blocked
    input_clock = 0;
    clkGate.PutRaw(0, &input_clock, 0, 1);  // Try to change CLK_IN
    clkGate.Tick();
    LOG("  After disable  - In=" << (int)input_clock << ", EN=" << (int)enable << ", Out=" << (int)clkGate.GetOutputClock());
    
    LOG("ClockGate test completed.");
}

// Entry point for the test
void Test81_ClockGate() {
    TestClockGate();
    LOG("Clock Gate test completed.");
}