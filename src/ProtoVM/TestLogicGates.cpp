#include "ProtoVM.h"

// Function to test basic logic gates functionality
void TestBasicLogicGates(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create reference pins
    Pin& ground = b.Add<Pin>("ground");  // Logic 0
    ground.SetReference(0);
    Pin& vcc = b.Add<Pin>("vcc");        // Logic 1
    vcc.SetReference(1);
    
    // Create test pins for inputs and outputs
    Pin& input_a = b.Add<Pin>("input_a");
    Pin& input_b = b.Add<Pin>("input_b");
    Pin& output_nand = b.Add<Pin>("output_nand");
    Pin& output_nor = b.Add<Pin>("output_nor");
    Pin& output_xor = b.Add<Pin>("output_xor");
    Pin& output_xnor = b.Add<Pin>("output_xnor");
    Pin& output_not = b.Add<Pin>("output_not");
    
    // Create all basic logic gates
    ElcNand& nand_gate = b.Add<ElcNand>("nand_gate");
    ElcNor& nor_gate = b.Add<ElcNor>("nor_gate");
    ElcXor& xor_gate = b.Add<ElcXor>("xor_gate");
    ElcXnor& xnor_gate = b.Add<ElcXnor>("xnor_gate");
    ElcNot& not_gate = b.Add<ElcNot>("not_gate");
    
    try {
        // Connect inputs to the gates
        input_a["bi"] >> nand_gate["I0"];
        input_b["bi"] >> nand_gate["I1"];
        
        input_a["bi"] >> nor_gate["I0"];
        input_b["bi"] >> nor_gate["I1"];
        
        input_a["bi"] >> xor_gate["I0"];
        input_b["bi"] >> xor_gate["I1"];
        
        input_a["bi"] >> xnor_gate["I0"];
        input_b["bi"] >> xnor_gate["I1"];
        
        input_a["bi"] >> not_gate["I"];
        
        // Connect outputs to monitor pins
        nand_gate["O"] >> output_nand["bi"];
        nor_gate["O"] >> output_nor["bi"];
        xor_gate["O"] >> output_xor["bi"];
        xnor_gate["O"] >> output_xnor["bi"];
        not_gate["O"] >> output_not["bi"];
        
        // Connect all components to maintain electrical continuity
        // Ground all inputs initially and connect outputs to VCC for monitoring
        ground["0"] >> input_a["bi"];
        ground["0"] >> input_b["bi"];
        output_nand["bi"] >> vcc["0"];
        output_nor["bi"] >> vcc["0"];
        output_xor["bi"] >> vcc["0"];
        output_xnor["bi"] >> vcc["0"];
        output_not["bi"] >> vcc["0"];
    }
    catch (Exc e) {
        LOG("error in TestBasicLogicGates: " << e);
    }
}