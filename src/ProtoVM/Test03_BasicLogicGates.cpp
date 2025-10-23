#include "ProtoVM.h"

// Test circuit for basic logic gates: NAND, NOR, XOR, XNOR - simple connected version
void SetupTest3_BasicLogicGates(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create reference pins
    Pin& ground = b.Add<Pin>("ground");  // Logic 0
    ground.SetReference(0);
    Pin& vcc = b.Add<Pin>("vcc");        // Logic 1
    vcc.SetReference(1);
    
    // Create a single one of my new logic gates to test
    ElcNor& nor_gate = b.Add<ElcNor>("nor_gate");
    
    try {
        // Connect inputs to ground to satisfy connectivity requirements
        ground["0"] >> nor_gate["I0"];
        ground["0"] >> nor_gate["I1"];
        
        // Connect output to vcc to satisfy connectivity requirements
        nor_gate["O"] >> vcc["0"];
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