#include "ProtoVM.h"

// Test for tube-based logic gates
void TestTubeLogicGates() {
    using namespace UPP;
    
    // Create a test machine
    Machine machine;
    
    // Test Tube NOT gate
    TubeNot not_gate;
    Pin not_input, not_output;
    
    // Connect input and output
    not_input.AddSource("0").SetMultiConn();
    not_output.AddSink("0");
    
    // Connect input -> NOT gate -> output
    machine.CreateLink(not_input, 0, not_gate, 0);  // Input to NOT gate
    machine.CreateLink(not_gate, 1, not_output, 0); // NOT gate output to output
    
    // Test NOT gate with input = 0 (should output 1)
    not_input.SetReference(false);  // Set input to 0
    machine.Tick();
    
    // Get the output
    byte output_value = 0;
    not_output.PutRaw(0, &output_value, 0, 1);
    LOG("NOT gate: input=0, output=" << (int)output_value);
    ASSERT(output_value == 1);  // NOT(0) should be 1
    
    // Test NOT gate with input = 1 (should output 0)
    not_input.SetReference(true);  // Set input to 1
    machine.Tick();
    
    not_output.PutRaw(0, &output_value, 0, 1);
    LOG("NOT gate: input=1, output=" << (int)output_value);
    ASSERT(output_value == 0);  // NOT(1) should be 0
    
    LOG("Tube NOT gate tests passed!");
    
    // Test Tube NAND gate
    TubeNand nand_gate;
    Pin nand_input0, nand_input1, nand_output;
    
    // Connect inputs and output
    nand_input0.AddSource("0").SetMultiConn();
    nand_input1.AddSource("0").SetMultiConn();
    nand_output.AddSink("0");
    
    // Connect inputs -> NAND gate -> output
    machine.CreateLink(nand_input0, 0, nand_gate, 0);  // Input 0 to NAND gate
    machine.CreateLink(nand_input1, 0, nand_gate, 1);  // Input 1 to NAND gate
    machine.CreateLink(nand_gate, 2, nand_output, 0);  // NAND gate output to output
    
    // Test NAND gate with inputs (0,0) - should output 1
    nand_input0.SetReference(false);
    nand_input1.SetReference(false);
    machine.Tick();
    
    nand_output.PutRaw(0, &output_value, 0, 1);
    LOG("NAND gate: (0,0) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // NAND(0,0) should be 1
    
    // Test NAND gate with inputs (0,1) - should output 1
    nand_input0.SetReference(false);
    nand_input1.SetReference(true);
    machine.Tick();
    
    nand_output.PutRaw(0, &output_value, 0, 1);
    LOG("NAND gate: (0,1) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // NAND(0,1) should be 1
    
    // Test NAND gate with inputs (1,0) - should output 1
    nand_input0.SetReference(true);
    nand_input1.SetReference(false);
    machine.Tick();
    
    nand_output.PutRaw(0, &output_value, 0, 1);
    LOG("NAND gate: (1,0) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // NAND(1,0) should be 1
    
    // Test NAND gate with inputs (1,1) - should output 0
    nand_input0.SetReference(true);
    nand_input1.SetReference(true);
    machine.Tick();
    
    nand_output.PutRaw(0, &output_value, 0, 1);
    LOG("NAND gate: (1,1) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // NAND(1,1) should be 0
    
    LOG("Tube NAND gate tests passed!");
    
    // Test Tube AND gate
    TubeAnd and_gate;
    Pin and_input0, and_input1, and_output;
    
    // Connect inputs and output
    and_input0.AddSource("0").SetMultiConn();
    and_input1.AddSource("0").SetMultiConn();
    and_output.AddSink("0");
    
    // Connect inputs -> AND gate -> output
    machine.CreateLink(and_input0, 0, and_gate, 0);  // Input 0 to AND gate
    machine.CreateLink(and_input1, 0, and_gate, 1);  // Input 1 to AND gate
    machine.CreateLink(and_gate, 2, and_output, 0);  // AND gate output to output
    
    // Test AND gate with inputs (0,0) - should output 0
    and_input0.SetReference(false);
    and_input1.SetReference(false);
    machine.Tick();
    
    and_output.PutRaw(0, &output_value, 0, 1);
    LOG("AND gate: (0,0) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // AND(0,0) should be 0
    
    // Test AND gate with inputs (0,1) - should output 0
    and_input0.SetReference(false);
    and_input1.SetReference(true);
    machine.Tick();
    
    and_output.PutRaw(0, &output_value, 0, 1);
    LOG("AND gate: (0,1) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // AND(0,1) should be 0
    
    // Test AND gate with inputs (1,0) - should output 0
    and_input0.SetReference(true);
    and_input1.SetReference(false);
    machine.Tick();
    
    and_output.PutRaw(0, &output_value, 0, 1);
    LOG("AND gate: (1,0) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // AND(1,0) should be 0
    
    // Test AND gate with inputs (1,1) - should output 1
    and_input0.SetReference(true);
    and_input1.SetReference(true);
    machine.Tick();
    
    and_output.PutRaw(0, &output_value, 0, 1);
    LOG("AND gate: (1,1) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // AND(1,1) should be 1
    
    LOG("Tube AND gate tests passed!");
    
    // Test Tube OR gate
    TubeOr or_gate;
    Pin or_input0, or_input1, or_output;
    
    // Connect inputs and output
    or_input0.AddSource("0").SetMultiConn();
    or_input1.AddSource("0").SetMultiConn();
    or_output.AddSink("0");
    
    // Connect inputs -> OR gate -> output
    machine.CreateLink(or_input0, 0, or_gate, 0);  // Input 0 to OR gate
    machine.CreateLink(or_input1, 0, or_gate, 1);  // Input 1 to OR gate
    machine.CreateLink(or_gate, 2, or_output, 0);  // OR gate output to output
    
    // Test OR gate with inputs (0,0) - should output 0
    or_input0.SetReference(false);
    or_input1.SetReference(false);
    machine.Tick();
    
    or_output.PutRaw(0, &output_value, 0, 1);
    LOG("OR gate: (0,0) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // OR(0,0) should be 0
    
    // Test OR gate with inputs (0,1) - should output 1
    or_input0.SetReference(false);
    or_input1.SetReference(true);
    machine.Tick();
    
    or_output.PutRaw(0, &output_value, 0, 1);
    LOG("OR gate: (0,1) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // OR(0,1) should be 1
    
    // Test OR gate with inputs (1,0) - should output 1
    or_input0.SetReference(true);
    or_input1.SetReference(false);
    machine.Tick();
    
    or_output.PutRaw(0, &output_value, 0, 1);
    LOG("OR gate: (1,0) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // OR(1,0) should be 1
    
    // Test OR gate with inputs (1,1) - should output 1
    or_input0.SetReference(true);
    or_input1.SetReference(true);
    machine.Tick();
    
    or_output.PutRaw(0, &output_value, 0, 1);
    LOG("OR gate: (1,1) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // OR(1,1) should be 1
    
    LOG("Tube OR gate tests passed!");
    
    // Test Tube NOR gate
    TubeNor nor_gate;
    Pin nor_input0, nor_input1, nor_output;
    
    // Connect inputs and output
    nor_input0.AddSource("0").SetMultiConn();
    nor_input1.AddSource("0").SetMultiConn();
    nor_output.AddSink("0");
    
    // Connect inputs -> NOR gate -> output
    machine.CreateLink(nor_input0, 0, nor_gate, 0);  // Input 0 to NOR gate
    machine.CreateLink(nor_input1, 0, nor_gate, 1);  // Input 1 to NOR gate
    machine.CreateLink(nor_gate, 2, nor_output, 0);  // NOR gate output to output
    
    // Test NOR gate with inputs (0,0) - should output 1
    nor_input0.SetReference(false);
    nor_input1.SetReference(false);
    machine.Tick();
    
    nor_output.PutRaw(0, &output_value, 0, 1);
    LOG("NOR gate: (0,0) -> output=" << (int)output_value);
    ASSERT(output_value == 1);  // NOR(0,0) should be 1
    
    // Test NOR gate with inputs (0,1) - should output 0
    nor_input0.SetReference(false);
    nor_input1.SetReference(true);
    machine.Tick();
    
    nor_output.PutRaw(0, &output_value, 0, 1);
    LOG("NOR gate: (0,1) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // NOR(0,1) should be 0
    
    // Test NOR gate with inputs (1,0) - should output 0
    nor_input0.SetReference(true);
    nor_input1.SetReference(false);
    machine.Tick();
    
    nor_output.PutRaw(0, &output_value, 0, 1);
    LOG("NOR gate: (1,0) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // NOR(1,0) should be 0
    
    // Test NOR gate with inputs (1,1) - should output 0
    nor_input0.SetReference(true);
    nor_input1.SetReference(true);
    machine.Tick();
    
    nor_output.PutRaw(0, &output_value, 0, 1);
    LOG("NOR gate: (1,1) -> output=" << (int)output_value);
    ASSERT(output_value == 0);  // NOR(1,1) should be 0
    
    LOG("Tube NOR gate tests passed!");
    
    LOG("All tube logic gate tests passed!");
}

CONSOLE_APP_MAIN {
    TestTubeLogicGates();
}