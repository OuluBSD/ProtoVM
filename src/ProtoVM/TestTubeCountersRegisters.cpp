#include "ProtoVM.h"

// Test for tube-based counters and registers
void TestTubeCountersRegisters() {
    using namespace UPP;
    
    LOG("Testing Tube-based Counters and Registers...");
    
    // Create a test machine
    Machine machine;
    
    // Test Tube 4-bit Register
    TubeRegister4Bit reg;
    Pin reg_d3, reg_d2, reg_d1, reg_d0, reg_clk, reg_en, reg_clr, reg_q3, reg_q2, reg_q1, reg_q0;
    
    // Connect inputs and outputs
    reg_d3.AddSource("0").SetMultiConn();
    reg_d2.AddSource("0").SetMultiConn();
    reg_d1.AddSource("0").SetMultiConn();
    reg_d0.AddSource("0").SetMultiConn();
    reg_clk.AddSource("0").SetMultiConn();
    reg_en.AddSource("0").SetMultiConn();
    reg_clr.AddSource("0").SetMultiConn();
    reg_q3.AddSink("0");
    reg_q2.AddSink("0");
    reg_q1.AddSink("0");
    reg_q0.AddSink("0");
    
    // Connect inputs/outputs to register
    machine.CreateLink(reg_d3, 0, reg, 0);   // D3
    machine.CreateLink(reg_d2, 0, reg, 1);   // D2
    machine.CreateLink(reg_d1, 0, reg, 2);   // D1
    machine.CreateLink(reg_d0, 0, reg, 3);   // D0
    machine.CreateLink(reg_clk, 0, reg, 4);  // Clock
    machine.CreateLink(reg_en, 0, reg, 5);   // Enable
    machine.CreateLink(reg_clr, 0, reg, 6);  // Clear
    machine.CreateLink(reg, 7, reg_q3, 0);   // Q3
    machine.CreateLink(reg, 8, reg_q2, 0);   // Q2
    machine.CreateLink(reg, 9, reg_q1, 0);   // Q1
    machine.CreateLink(reg, 10, reg_q0, 0);  // Q0
    
    // First, ensure enable is high and clear is low
    reg_en.SetReference(true);
    reg_clr.SetReference(false);
    machine.Tick();
    
    // Load a value into the register: 1010 (A in hex)
    reg_d3.SetReference(true);   // 1
    reg_d2.SetReference(false);  // 0
    reg_d1.SetReference(true);   // 1
    reg_d0.SetReference(false);  // 0
    
    // Apply a rising edge to clock to load the value
    reg_clk.SetReference(false);  // Clock low
    machine.Tick();
    
    reg_clk.SetReference(true);   // Clock rising edge
    machine.Tick();
    
    // Check the output
    byte q3_val = 0, q2_val = 0, q1_val = 0, q0_val = 0;
    reg_q3.PutRaw(0, &q3_val, 0, 1);
    reg_q2.PutRaw(0, &q2_val, 0, 1);
    reg_q1.PutRaw(0, &q1_val, 0, 1);
    reg_q0.PutRaw(0, &q0_val, 0, 1);
    
    LOG("Register: loaded 1010 -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 1 && q2_val == 0 && q1_val == 1 && q0_val == 0);  // Should be 1010
    
    // Test clear functionality
    reg_clr.SetReference(true);  // Clear
    machine.Tick();
    
    reg_q3.PutRaw(0, &q3_val, 0, 1);
    reg_q2.PutRaw(0, &q2_val, 0, 1);
    reg_q1.PutRaw(0, &q1_val, 0, 1);
    reg_q0.PutRaw(0, &q0_val, 0, 1);
    
    LOG("Register: after clear -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 0 && q1_val == 0 && q0_val == 0);  // Should be 0000
    
    LOG("Tube 4-bit register tests passed!");
    
    // Test Tube 4-bit Binary Counter
    TubeBinaryCounter4Bit counter;
    Pin cnt_clk, cnt_en, cnt_clr, cnt_q3, cnt_q2, cnt_q1, cnt_q0;
    
    // Connect inputs and outputs
    cnt_clk.AddSource("0").SetMultiConn();
    cnt_en.AddSource("0").SetMultiConn();
    cnt_clr.AddSource("0").SetMultiConn();
    cnt_q3.AddSink("0");
    cnt_q2.AddSink("0");
    cnt_q1.AddSink("0");
    cnt_q0.AddSink("0");
    
    // Connect inputs/outputs to counter
    machine.CreateLink(cnt_clk, 0, counter, 0);  // Clock
    machine.CreateLink(cnt_en, 0, counter, 1);   // Enable
    machine.CreateLink(cnt_clr, 0, counter, 2);  // Clear
    machine.CreateLink(counter, 3, cnt_q3, 0);   // Q3
    machine.CreateLink(counter, 4, cnt_q2, 0);   // Q2
    machine.CreateLink(counter, 5, cnt_q1, 0);   // Q1
    machine.CreateLink(counter, 6, cnt_q0, 0);   // Q0
    
    // First, ensure enable is high and clear is low
    cnt_en.SetReference(true);
    cnt_clr.SetReference(false);
    machine.Tick();
    
    // Clear the counter
    cnt_clr.SetReference(true);
    machine.Tick();
    cnt_clr.SetReference(false);
    machine.Tick();
    
    // Now test counting sequence: 0000 -> 0001 -> 0010 -> 0011 -> 0100
    // Apply 5 clock pulses and check each count
    
    // Count 0 (after clear)
    cnt_q3.PutRaw(0, &q3_val, 0, 1);
    cnt_q2.PutRaw(0, &q2_val, 0, 1);
    cnt_q1.PutRaw(0, &q1_val, 0, 1);
    cnt_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("Counter: after clear -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 0 && q1_val == 0 && q0_val == 0);  // Should be 0000
    
    // Count 1: Apply rising edge
    cnt_clk.SetReference(false);
    machine.Tick();
    cnt_clk.SetReference(true);
    machine.Tick();
    
    cnt_q3.PutRaw(0, &q3_val, 0, 1);
    cnt_q2.PutRaw(0, &q2_val, 0, 1);
    cnt_q1.PutRaw(0, &q1_val, 0, 1);
    cnt_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("Counter: count 1 -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 0 && q1_val == 0 && q0_val == 1);  // Should be 0001
    
    // Count 2: Apply rising edge
    cnt_clk.SetReference(false);
    machine.Tick();
    cnt_clk.SetReference(true);
    machine.Tick();
    
    cnt_q3.PutRaw(0, &q3_val, 0, 1);
    cnt_q2.PutRaw(0, &q2_val, 0, 1);
    cnt_q1.PutRaw(0, &q1_val, 0, 1);
    cnt_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("Counter: count 2 -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 0 && q1_val == 1 && q0_val == 0);  // Should be 0010
    
    // Count 3: Apply rising edge
    cnt_clk.SetReference(false);
    machine.Tick();
    cnt_clk.SetReference(true);
    machine.Tick();
    
    cnt_q3.PutRaw(0, &q3_val, 0, 1);
    cnt_q2.PutRaw(0, &q2_val, 0, 1);
    cnt_q1.PutRaw(0, &q1_val, 0, 1);
    cnt_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("Counter: count 3 -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 0 && q1_val == 1 && q0_val == 1);  // Should be 0011
    
    // Count 4: Apply rising edge
    cnt_clk.SetReference(false);
    machine.Tick();
    cnt_clk.SetReference(true);
    machine.Tick();
    
    cnt_q3.PutRaw(0, &q3_val, 0, 1);
    cnt_q2.PutRaw(0, &q2_val, 0, 1);
    cnt_q1.PutRaw(0, &q1_val, 0, 1);
    cnt_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("Counter: count 4 -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 1 && q1_val == 0 && q0_val == 0);  // Should be 0100
    
    // Test counter wrap-around from 15 to 0
    // Count to 15 first (apply 11 more clock pulses from 4)
    for (int i = 0; i < 11; i++) {
        cnt_clk.SetReference(false);
        machine.Tick();
        cnt_clk.SetReference(true);
        machine.Tick();
    }
    
    // Now we should be at 15 (1111)
    cnt_q3.PutRaw(0, &q3_val, 0, 1);
    cnt_q2.PutRaw(0, &q2_val, 0, 1);
    cnt_q1.PutRaw(0, &q1_val, 0, 1);
    cnt_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("Counter: before wrap -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 1 && q2_val == 1 && q1_val == 1 && q0_val == 1);  // Should be 1111
    
    // Apply one more clock pulse to wrap to 0
    cnt_clk.SetReference(false);
    machine.Tick();
    cnt_clk.SetReference(true);
    machine.Tick();
    
    cnt_q3.PutRaw(0, &q3_val, 0, 1);
    cnt_q2.PutRaw(0, &q2_val, 0, 1);
    cnt_q1.PutRaw(0, &q1_val, 0, 1);
    cnt_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("Counter: after wrap -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 0 && q1_val == 0 && q0_val == 0);  // Should be 0000
    
    LOG("Tube 4-bit binary counter tests passed!");
    
    // Test Tube 4-bit Counter with Load functionality
    TubeCounter4Bit loadCounter;
    Pin lc_d3, lc_d2, lc_d1, lc_d0, lc_clk, lc_en, lc_clr, lc_load;
    Pin lc_q3, lc_q2, lc_q1, lc_q0;
    
    // Connect inputs and outputs
    lc_d3.AddSource("0").SetMultiConn();
    lc_d2.AddSource("0").SetMultiConn();
    lc_d1.AddSource("0").SetMultiConn();
    lc_d0.AddSource("0").SetMultiConn();
    lc_clk.AddSource("0").SetMultiConn();
    lc_en.AddSource("0").SetMultiConn();
    lc_clr.AddSource("0").SetMultiConn();
    lc_load.AddSource("0").SetMultiConn();
    lc_q3.AddSink("0");
    lc_q2.AddSink("0");
    lc_q1.AddSink("0");
    lc_q0.AddSink("0");
    
    // Connect inputs/outputs to load counter
    machine.CreateLink(lc_d3, 0, loadCounter, 0);   // D3
    machine.CreateLink(lc_d2, 0, loadCounter, 1);   // D2
    machine.CreateLink(lc_d1, 0, loadCounter, 2);   // D1
    machine.CreateLink(lc_d0, 0, loadCounter, 3);   // D0
    machine.CreateLink(lc_clk, 0, loadCounter, 4);  // Clock
    machine.CreateLink(lc_en, 0, loadCounter, 5);   // Enable
    machine.CreateLink(lc_clr, 0, loadCounter, 6);  // Clear
    machine.CreateLink(lc_load, 0, loadCounter, 7); // Load
    machine.CreateLink(loadCounter, 8, lc_q3, 0);   // Q3
    machine.CreateLink(loadCounter, 9, lc_q2, 0);   // Q2
    machine.CreateLink(loadCounter, 10, lc_q1, 0);  // Q1
    machine.CreateLink(loadCounter, 11, lc_q0, 0);  // Q0
    
    // First, ensure enable is high and clear/load are low
    lc_en.SetReference(true);
    lc_clr.SetReference(false);
    lc_load.SetReference(false);
    machine.Tick();
    
    // Clear the counter
    lc_clr.SetReference(true);
    machine.Tick();
    lc_clr.SetReference(false);
    machine.Tick();
    
    // Check that counter is cleared
    lc_q3.PutRaw(0, &q3_val, 0, 1);
    lc_q2.PutRaw(0, &q2_val, 0, 1);
    lc_q1.PutRaw(0, &q1_val, 0, 1);
    lc_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("LoadCounter: after clear -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 0 && q2_val == 0 && q1_val == 0 && q0_val == 0);  // Should be 0000
    
    // Now load a value: 1100 (C in hex)
    lc_d3.SetReference(true);   // 1
    lc_d2.SetReference(true);   // 1
    lc_d1.SetReference(false);  // 0
    lc_d0.SetReference(false);  // 0
    
    // Apply load signal with a clock pulse
    lc_load.SetReference(true);
    lc_clk.SetReference(false);
    machine.Tick();
    
    lc_clk.SetReference(true);   // Rising edge with Load active
    machine.Tick();
    
    lc_q3.PutRaw(0, &q3_val, 0, 1);
    lc_q2.PutRaw(0, &q2_val, 0, 1);
    lc_q1.PutRaw(0, &q1_val, 0, 1);
    lc_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("LoadCounter: after load 1100 -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 1 && q2_val == 1 && q1_val == 0 && q0_val == 0);  // Should be 1100
    
    // Now disable load and increment the counter
    lc_load.SetReference(false);
    lc_clk.SetReference(false);
    machine.Tick();
    
    lc_clk.SetReference(true);   // Rising edge - should increment
    machine.Tick();
    
    lc_q3.PutRaw(0, &q3_val, 0, 1);
    lc_q2.PutRaw(0, &q2_val, 0, 1);
    lc_q1.PutRaw(0, &q1_val, 0, 1);
    lc_q0.PutRaw(0, &q0_val, 0, 1);
    LOG("LoadCounter: after increment -> Q=" << (int)q3_val << (int)q2_val << (int)q1_val << (int)q0_val);
    ASSERT(q3_val == 1 && q2_val == 1 && q1_val == 0 && q0_val == 1);  // Should be 1101
    
    LOG("Tube 4-bit counter with load tests passed!");
    
    LOG("All tube-based counters and registers tests passed!");
}

CONSOLE_APP_MAIN {
    TestTubeCountersRegisters();
}