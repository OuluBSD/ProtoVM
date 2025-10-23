#include "ProtoVM.h"

// Test circuit for all basic logic gates: NAND, NOR, XOR, XNOR, NOT
void SetupTest3_BasicLogicGates(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create reference pins
    Pin& ground = b.Add<Pin>("ground");  // Logic 0
    ground.SetReference(0);
    Pin& vcc = b.Add<Pin>("vcc");        // Logic 1
    vcc.SetReference(1);
    
    // Create all basic logic gates
    ElcNand& nand_gate = b.Add<ElcNand>("nand_gate");
    ElcNor& nor_gate = b.Add<ElcNor>("nor_gate");
    ElcXor& xor_gate = b.Add<ElcXor>("xor_gate");
    ElcXnor& xnor_gate = b.Add<ElcXnor>("xnor_gate");
    ElcNot& not_gate = b.Add<ElcNot>("not_gate");
    
    try {
        // Connect inputs for each gate
        // NAND gate: connect both inputs to ground initially
        ground["0"] >> nand_gate["I0"];
        ground["0"] >> nand_gate["I1"];
        
        // NOR gate: connect both inputs to ground initially
        ground["0"] >> nor_gate["I0"];
        ground["0"] >> nor_gate["I1"];
        
        // XOR gate: connect both inputs to ground initially
        ground["0"] >> xor_gate["I0"];
        ground["0"] >> xor_gate["I1"];
        
        // XNOR gate: connect both inputs to ground initially
        ground["0"] >> xnor_gate["I0"];
        ground["0"] >> xnor_gate["I1"];
        
        // NOT gate: connect input to ground initially
        ground["0"] >> not_gate["I"];
        
        // Connect outputs to vcc to satisfy connectivity requirements
        nand_gate["O"] >> vcc["0"];
        nor_gate["O"] >> vcc["0"];
        xor_gate["O"] >> vcc["0"];
        xnor_gate["O"] >> vcc["0"];
        not_gate["O"] >> vcc["0"];
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}

// Test circuit for multiplexer and demultiplexer components - simple connected version 
void SetupTest4_MuxDemux(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create reference pins
    Pin& ground = b.Add<Pin>("ground");  // Logic 0
    ground.SetReference(0);
    Pin& vcc = b.Add<Pin>("vcc");        // Logic 1
    vcc.SetReference(1);
    
    // Create a single mux to test
    Mux2to1& mux2to1 = b.Add<Mux2to1>("mux2to1");
    
    try {
        // Connect inputs to ground to satisfy connectivity requirements
        ground["0"] >> mux2to1["I0"];
        ground["0"] >> mux2to1["I1"];
        ground["0"] >> mux2to1["SEL"];
        
        // Connect output to vcc to satisfy connectivity requirements
        mux2to1["O"] >> vcc["0"];
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}

// Test circuit for decoder and encoder components - simple connected version
void SetupTest5_DecoderEncoder(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create reference pins
    Pin& ground = b.Add<Pin>("ground");  // Logic 0
    ground.SetReference(0);
    Pin& vcc = b.Add<Pin>("vcc");        // Logic 1
    vcc.SetReference(1);
    
    // Create a single decoder to test
    Decoder2to4& decoder2to4 = b.Add<Decoder2to4>("decoder2to4");
    
    try {
        // Connect inputs to ground to satisfy connectivity requirements
        ground["0"] >> decoder2to4["A0"];
        ground["0"] >> decoder2to4["A1"];
        ground["0"] >> decoder2to4["EN"];
        
        // Connect outputs to vcc to satisfy connectivity requirements
        decoder2to4["Y0"] >> vcc["0"];
        decoder2to4["Y1"] >> vcc["0"];
        decoder2to4["Y2"] >> vcc["0"];
        decoder2to4["Y3"] >> vcc["0"];
    }
    catch (Exc e) {
        LOG("error: " << e);
    }
}