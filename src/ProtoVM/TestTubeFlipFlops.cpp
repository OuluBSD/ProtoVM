#include "ProtoVM.h"

// Test for tube-based flip-flops
void TestTubeFlipFlops() {
    using namespace UPP;
    
    LOG("Testing Tube-based Flip-Flops...");
    
    // Create a test machine
    Machine machine;
    
    // Test Tube SR Latch
    TubeSRLatch sr_latch;
    Pin s_input, r_input, q_output, qn_output;
    
    // Connect inputs and outputs
    s_input.AddSource("0").SetMultiConn();
    r_input.AddSource("0").SetMultiConn();
    q_output.AddSink("0");
    qn_output.AddSink("0");
    
    // Connect inputs -> SR Latch -> outputs
    machine.CreateLink(s_input, 0, sr_latch, 0);  // S input
    machine.CreateLink(r_input, 0, sr_latch, 1);  // R input
    machine.CreateLink(sr_latch, 2, q_output, 0); // Q output
    machine.CreateLink(sr_latch, 3, qn_output, 0); // ~Q output
    
    // Test SR Latch: S=0, R=0 (hold state)
    s_input.SetReference(false);
    r_input.SetReference(false);
    machine.Tick();
    
    byte q_val = 0, qn_val = 0;
    q_output.PutRaw(0, &q_val, 0, 1);
    qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("SR Latch: S=0, R=0 -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    
    // Test SR Latch: S=1, R=0 (set)
    s_input.SetReference(true);
    r_input.SetReference(false);
    machine.Tick();
    
    q_output.PutRaw(0, &q_val, 0, 1);
    qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("SR Latch: S=1, R=0 -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 1 && qn_val == 0);  // Q should be 1, ~Q should be 0
    
    // Test SR Latch: S=0, R=1 (reset)
    s_input.SetReference(false);
    r_input.SetReference(true);
    machine.Tick();
    
    q_output.PutRaw(0, &q_val, 0, 1);
    qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("SR Latch: S=0, R=1 -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 0 && qn_val == 1);  // Q should be 0, ~Q should be 1
    
    // Test SR Latch: S=0, R=0 (hold state after reset)
    s_input.SetReference(false);
    r_input.SetReference(false);
    machine.Tick();
    
    q_output.PutRaw(0, &q_val, 0, 1);
    qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("SR Latch: S=0, R=0 -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 0 && qn_val == 1);  // Should still be in reset state
    
    LOG("Tube SR Latch tests passed!");
    
    // Test Tube D Flip-Flop
    TubeDFlipFlop d_ff;
    Pin d_input, clk_input, en_input, clr_input, d_q_output, d_qn_output;
    
    // Connect inputs and outputs
    d_input.AddSource("0").SetMultiConn();
    clk_input.AddSource("0").SetMultiConn();
    en_input.AddSource("0").SetMultiConn();
    clr_input.AddSource("0").SetMultiConn();
    d_q_output.AddSink("0");
    d_qn_output.AddSink("0");
    
    // Connect inputs -> D Flip-Flop -> outputs
    machine.CreateLink(d_input, 0, d_ff, 0);     // D input
    machine.CreateLink(clk_input, 0, d_ff, 1);   // Clock input
    machine.CreateLink(en_input, 0, d_ff, 2);    // Enable input
    machine.CreateLink(clr_input, 0, d_ff, 3);   // Clear input
    machine.CreateLink(d_ff, 4, d_q_output, 0);  // Q output
    machine.CreateLink(d_ff, 5, d_qn_output, 0); // ~Q output
    
    // Initially, set enable high and clear low to allow normal operation
    en_input.SetReference(true);
    clr_input.SetReference(false);
    machine.Tick();
    
    // Test D Flip-Flop: D=1, with rising clock edge
    d_input.SetReference(true);
    clk_input.SetReference(false);  // Clock low
    machine.Tick();
    
    clk_input.SetReference(true);   // Clock rising edge
    machine.Tick();
    
    d_q_output.PutRaw(0, &q_val, 0, 1);
    d_qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("D Flip-Flop: D=1, rising edge -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 1 && qn_val == 0);  // Q should follow D on rising edge
    
    // Test D Flip-Flop: D=0, with rising clock edge
    d_input.SetReference(false);
    clk_input.SetReference(false);  // Clock low
    machine.Tick();
    
    clk_input.SetReference(true);   // Clock rising edge
    machine.Tick();
    
    d_q_output.PutRaw(0, &q_val, 0, 1);
    d_qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("D Flip-Flop: D=0, rising edge -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 0 && qn_val == 1);  // Q should follow D on rising edge
    
    // Test clear functionality
    clr_input.SetReference(true);   // Clear should reset output
    machine.Tick();
    
    d_q_output.PutRaw(0, &q_val, 0, 1);
    d_qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("D Flip-Flop: Clear active -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 0 && qn_val == 1);  // Should be cleared regardless of D
    
    LOG("Tube D Flip-Flop tests passed!");
    
    // Test Tube JK Flip-Flop
    TubeJKFlipFlop jk_ff;
    Pin j_input, k_input, jk_clk_input, jk_en_input, jk_clr_input, jk_q_output, jk_qn_output;
    
    // Connect inputs and outputs
    j_input.AddSource("0").SetMultiConn();
    k_input.AddSource("0").SetMultiConn();
    jk_clk_input.AddSource("0").SetMultiConn();
    jk_en_input.AddSource("0").SetMultiConn();
    jk_clr_input.AddSource("0").SetMultiConn();
    jk_q_output.AddSink("0");
    jk_qn_output.AddSink("0");
    
    // Connect inputs -> JK Flip-Flop -> outputs
    machine.CreateLink(j_input, 0, jk_ff, 0);       // J input
    machine.CreateLink(k_input, 0, jk_ff, 1);       // K input
    machine.CreateLink(jk_clk_input, 0, jk_ff, 2);  // Clock input
    machine.CreateLink(jk_en_input, 0, jk_ff, 3);   // Enable input
    machine.CreateLink(jk_clr_input, 0, jk_ff, 4);  // Clear input
    machine.CreateLink(jk_ff, 5, jk_q_output, 0);   // Q output
    machine.CreateLink(jk_ff, 6, jk_qn_output, 0);  // ~Q output
    
    // Initially, set enable high and clear low to allow normal operation
    jk_en_input.SetReference(true);
    jk_clr_input.SetReference(false);
    machine.Tick();
    
    // Test JK Flip-Flop: J=0, K=0 (hold state)
    j_input.SetReference(false);
    k_input.SetReference(false);
    jk_clk_input.SetReference(false);  // Clock low
    machine.Tick();
    
    jk_clk_input.SetReference(true);   // Clock rising edge
    machine.Tick();
    
    jk_q_output.PutRaw(0, &q_val, 0, 1);
    jk_qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("JK Flip-Flop: J=0, K=0, rising edge -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    
    // Test JK Flip-Flop: J=1, K=0 (set)
    j_input.SetReference(true);
    k_input.SetReference(false);
    jk_clk_input.SetReference(false);  // Clock low
    machine.Tick();
    
    jk_clk_input.SetReference(true);   // Clock rising edge
    machine.Tick();
    
    jk_q_output.PutRaw(0, &q_val, 0, 1);
    jk_qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("JK Flip-Flop: J=1, K=0, rising edge -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 1 && qn_val == 0);  // Should set
    
    // Test JK Flip-Flop: J=0, K=1 (reset)
    j_input.SetReference(false);
    k_input.SetReference(true);
    jk_clk_input.SetReference(false);  // Clock low
    machine.Tick();
    
    jk_clk_input.SetReference(true);   // Clock rising edge
    machine.Tick();
    
    jk_q_output.PutRaw(0, &q_val, 0, 1);
    jk_qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("JK Flip-Flop: J=0, K=1, rising edge -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 0 && qn_val == 1);  // Should reset
    
    // Set Q=1 first, then test toggle
    j_input.SetReference(true);
    k_input.SetReference(false);
    jk_clk_input.SetReference(false);  // Clock low
    machine.Tick();
    
    jk_clk_input.SetReference(true);   // Clock rising edge (set Q to 1)
    machine.Tick();
    
    // Now test toggle: J=1, K=1 (toggle)
    j_input.SetReference(true);
    k_input.SetReference(true);
    jk_clk_input.SetReference(false);  // Clock low
    machine.Tick();
    
    jk_clk_input.SetReference(true);   // Clock rising edge (should toggle)
    machine.Tick();
    
    jk_q_output.PutRaw(0, &q_val, 0, 1);
    jk_qn_output.PutRaw(0, &qn_val, 0, 1);
    LOG("JK Flip-Flop: J=1, K=1, rising edge -> Q=" << (int)q_val << ", ~Q=" << (int)qn_val);
    ASSERT(q_val == 0 && qn_val == 1);  // Should toggle from 1 to 0
    
    LOG("Tube JK Flip-Flop tests passed!");
    
    LOG("All tube flip-flop tests passed!");
}

CONSOLE_APP_MAIN {
    TestTubeFlipFlops();
}