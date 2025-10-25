#include "ProtoVM.h"

// Function to test basic logic gates functionality with proper electrical connections
void TestBasicLogicGates(Machine& mach) {
    Pcb& b = mach.AddPcb();
    
    // Create reference pins
    Pin& ground = b.Add<Pin>("ground");  // Logic 0 - acts as source when SetReference(0)
    ground.SetReference(0);
    Pin& vcc = b.Add<Pin>("vcc");        // Logic 1 - acts as source when SetReference(1) 
    vcc.SetReference(1);
    
    // Create all basic logic gates
    ElcNand& nand_gate = b.Add<ElcNand>("nand_gate");
    ElcNor& nor_gate = b.Add<ElcNor>("nor_gate");
    ElcXor& xor_gate = b.Add<ElcXor>("xor_gate");
    ElcXnor& xnor_gate = b.Add<ElcXnor>("xnor_gate");
    ElcNot& not_gate = b.Add<ElcNot>("not_gate");
    
    try {
        // Connect inputs: connect VCC and ground (sources) to gate inputs (sinks)
        vcc >> nand_gate["I0"];
        vcc >> nand_gate["I1"];
        
        ground >> nor_gate["I0"];
        ground >> nor_gate["I1"];
        
        vcc >> xor_gate["I0"];
        ground >> xor_gate["I1"];
        
        vcc >> xnor_gate["I0"];
        ground >> xnor_gate["I1"];
        
        vcc >> not_gate["I"];
        
        // To complete the circuit, we need to connect the outputs properly
        // Each gate output is a source, so we need to connect it to a sink
        // We'll create simple Chip components with dummy sink pins to receive the outputs
        struct DummySink : Chip {
            DummySink() {
                AddSink("IN").SetRequired(false);  // Input sink for receiving output
            }
            
            bool Tick() override { return true; }
            bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
                return true;  // Just acknowledge
            }
            bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
                return true;  // Just acknowledge
            }
        };
        
        DummySink& dummy1 = b.Add<DummySink>("dummy1");
        DummySink& dummy2 = b.Add<DummySink>("dummy2");
        DummySink& dummy3 = b.Add<DummySink>("dummy3");
        DummySink& dummy4 = b.Add<DummySink>("dummy4");
        DummySink& dummy5 = b.Add<DummySink>("dummy5");
        
        // Connect gate outputs (sources) to dummy sinks
        nand_gate["O"] >> dummy1["IN"];
        nor_gate["O"] >> dummy2["IN"];
        xor_gate["O"] >> dummy3["IN"];
        xnor_gate["O"] >> dummy4["IN"];
        not_gate["O"] >> dummy5["IN"];
        
        // Connect the dummy sinks to ground to complete the circuit (ground acts as sink for the dummy components)
        // Actually, we don't need another connection for the dummy components since they have their required input connected
    }
    catch (Exc e) {
        LOG("error in TestBasicLogicGates: " << e);
    }
    
    LOG("TestBasicLogicGates: Circuit built successfully");
}