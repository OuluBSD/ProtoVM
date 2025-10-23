#include "ProtoVM.h"

// Simple test for 4-bit register functionality
void Test4BitRegister(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create reference pins
    Pin& ground = b.Add<Pin>("ground");
    ground.SetReference(0);
    Pin& vcc = b.Add<Pin>("vcc");
    vcc.SetReference(1);
    
    // Create a 4-bit register
    Register4Bit& reg4bit = b.Add<Register4Bit>("reg4bit");
    
    // Connect inputs to ground initially (all zeros)
    ground["0"] >> reg4bit["D3"];
    ground["0"] >> reg4bit["D2"];
    ground["0"] >> reg4bit["D1"];
    ground["0"] >> reg4bit["D0"];
    ground["0"] >> reg4bit["Ck"];
    ground["0"] >> reg4bit["En"];
    ground["0"] >> reg4bit["Clr"];
    
    // Connect outputs to vcc for electrical continuity
    reg4bit["Q3"] >> vcc["0"];
    reg4bit["Q2"] >> vcc["0"];
    reg4bit["Q1"] >> vcc["0"];
    reg4bit["Q0"] >> vcc["0"];
    
    LOG("4-bit register test initialized with all inputs = 0");
    LOG("Initial state: D[3:0] = 0000, Ck = 0, En = 0, Clr = 0");
}
