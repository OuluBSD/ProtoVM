#include "ProtoVM.h"

void Test4BitAdder(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create a 4-bit adder component
    AdderSubtractor4Bit& adder = b.Add<AdderSubtractor4Bit>("Adder4Bit");
    
    // Create input drivers for A (4 bits) - A = 0101 (5)
    Pin& a3 = b.Add<Pin>("A3").SetReference(0);  // 0 (source)
    Pin& a2 = b.Add<Pin>("A2").SetReference(1);  // 1 (source) 
    Pin& a1 = b.Add<Pin>("A1").SetReference(0);  // 0 (source)
    Pin& a0 = b.Add<Pin>("A0").SetReference(1);  // 1 (source)
    
    // Create input drivers for B (4 bits) - B = 0011 (3)
    Pin& b3 = b.Add<Pin>("B3").SetReference(0);  // 0 (source)
    Pin& b2 = b.Add<Pin>("B2").SetReference(0);  // 0 (source)
    Pin& b1 = b.Add<Pin>("B1").SetReference(1);  // 1 (source)
    Pin& b0 = b.Add<Pin>("B0").SetReference(1);  // 1 (source)
    
    // Create input driver for subtraction control (addition mode)
    Pin& sub = b.Add<Pin>("SUB").SetReference(0);  // 0 = addition (source)
    
    // Create input driver for carry in
    Pin& cin = b.Add<Pin>("CIN").SetReference(0);  // 0 = no carry (source)
    
    // Create bidirectional pins for outputs to "monitor" them (act as sinks)
    Pin& s3 = b.Add<Pin>("S3");  // Bidirectional (will act as sink for adder output)
    Pin& s2 = b.Add<Pin>("S2");  // Bidirectional
    Pin& s1 = b.Add<Pin>("S1");  // Bidirectional
    Pin& s0 = b.Add<Pin>("S0");  // Bidirectional
    Pin& cout_out = b.Add<Pin>("COUT");  // Bidirectional
    
    try {
        // Connect A inputs to the adder
        a3["0"] >> adder["A3"];
        a2["0"] >> adder["A2"];
        a1["0"] >> adder["A1"];
        a0["0"] >> adder["A0"];
        
        // Connect B inputs to the adder
        b3["0"] >> adder["B3"];
        b2["0"] >> adder["B2"];
        b1["0"] >> adder["B1"];
        b0["0"] >> adder["B0"];
        
        // Connect subtraction control
        sub["0"] >> adder["Sub"];
        
        // Connect carry in
        cin["0"] >> adder["Cin"];
        
        // Connect sum outputs
        adder["S3"] >> s3["bi"];
        adder["S2"] >> s2["bi"];
        adder["S1"] >> s1["bi"];
        adder["S0"] >> s0["bi"];
        
        // Connect carry output
        adder["Cout"] >> cout_out["bi"];
        
        LOG("4-bit Adder test circuit created - A=5 (0101), B=3 (0011), Add mode");
        LOG("Expected result: Sum = 8 (1000), Carry = 0");
    }
    catch (Exc e) {
        LOG("Error connecting 4-bit adder circuit: " << e);
    }
}