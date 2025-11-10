#include "ProtoVM.h"
#include "ClockDivider.h"

#include <Core/Core.h>
using namespace UPP;

// Test for the ClockDivider component
void TestClockDivider() {
    LOG("Starting ClockDivider Test...");
    
    // Create a divide-by-4 clock divider
    ClockDivider clkDiv(4);
    clkDiv.SetName("TestClockDivider_DIV4");

    // Test the initial state
    LOG("Initial state:");
    LOG("  Division factor: " + AsString(clkDiv.GetDivisionFactor()));
    LOG("  Counter: " + AsString(clkDiv.GetCounter()));
    LOG("  Output clock: " + AsString(clkDiv.GetOutputClock()));

    // Simulate input clock ticks to test the divider
    byte clock_state = 0;
    
    LOG("Simulating 20 input clock cycles (divide by 4):");
    for (int i = 0; i < 20; i++) {
        // Toggle input clock every other tick to simulate a clock signal
        if (i % 2 == 0) {
            clock_state = !clock_state;  // Toggle clock state
        }
        
        // Send the clock signal to the divider
        clkDiv.PutRaw(0, &clock_state, 0, 1);  // Send to CLK_IN (conn_id = 0)
        
        // Process the tick
        clkDiv.Tick();
        
        LOG("  Tick " << AsString(i) << ": In=" << AsString((int)clock_state) 
             << ", Out=" << (int)clkDiv.GetOutputClock() 
             << ", Counter=" << clkDiv.GetCounter());
    }

    LOG("ClockDivider test completed.");
}

// Entry point for the test
void Test80_ClockDivider() {
    TestClockDivider();
    LOG("Clock Divider test completed.");
}