#include "ProtoVM.h"
#include "PLL.h"

#include <Core/Core.h>
using namespace UPP;

// Test for the PLL component
void TestPLL() {
    LOG("Starting PLL Test...");
    
    // Create a PLL with 4x multiplication
    PLL pll(4);
    pll.SetName("TestPLL_X4");

    LOG("PLL Configuration:");
    LOG("  Multiplication factor: " << pll.GetMultiplicationFactor());
    LOG("  Initial locked status: " << pll.IsLocked());

    // Simulate the PLL operation
    byte input_clock = 0;
    byte reset = 0;
    
    LOG("Simulating PLL with 4x multiplication:");
    for (int i = 0; i < 30; i++) {
        // Simulate input clock - toggle every 5 ticks to simulate lower freq
        if (i % 5 == 0) {
            input_clock = !input_clock;
        }
        
        pll.PutRaw(0, &input_clock, 0, 1);  // Send to CLK_IN
        pll.PutRaw(1, &reset, 0, 1);        // Send reset signal (0 = no reset)
        
        pll.Tick();
        
        LOG("  Tick " << i << ": In=" << (int)input_clock 
             << ", Out=" << (int)pll.GetOutputClock() 
             << ", Counter=" << pll.GetOutputCounter()
             << ", Locked=" << pll.IsLocked());
    }
    
    LOG("\\nTesting PLL reset functionality:");
    reset = 1;  // Activate reset
    pll.PutRaw(1, &reset, 0, 1);
    pll.Tick();
    LOG("  After reset - Locked: " << pll.IsLocked() 
         << ", Output: " << (int)pll.GetOutputClock());
    
    reset = 0;  // Deactivate reset
    pll.PutRaw(1, &reset, 0, 1);
    for (int i = 0; i < 15; i++) {
        if (i % 5 == 0) {
            input_clock = !input_clock;
        }
        pll.PutRaw(0, &input_clock, 0, 1);
        pll.Tick();
        
        LOG("  Post-reset Tick " << i << ": Locked=" << pll.IsLocked() 
             << ", Out=" << (int)pll.GetOutputClock());
    }

    LOG("PLL test completed.");
}

// Entry point for the test
void Test82_PLL() {
    TestPLL();
    LOG("PLL test completed.");
}