#include "ProtoVM.h"
#include "ALU.h"
#include "StateMachine.h"
#include "SimpleCPU.h"

// Test for the inspection functionality
void TestInspect() {
    Pcb pcb;
    
    // Create test components
    ALU alu(4);  // 4-bit ALU
    alu.SetName("TestALU");
    
    StateMachine sm(4);  // 4-state machine
    sm.SetName("TestStateMachine");
    
    FsmController fsm(3);  // 3-state FSM controller
    fsm.SetName("TestFsmController");
    
    // Add components to the PCB
    pcb.Add(alu);
    pcb.Add(sm);
    pcb.Add(fsm);
    
    // Verify components were added
    LOG("Added " << pcb.GetNodeCount() << " components to PCB");
    
    // Test inspection functionality
    for (int i = 0; i < pcb.GetNodeCount(); i++) {
        ElectricNodeBase& node = pcb.GetNode(i);
        String className = node.GetClassName();
        String name = node.GetName();
        
        LOG("Component [" << i << "]: " << className << " (" << name << ")");
        LOG("  Connectors: " << node.GetConnectorCount());
        
        // Test connector access
        for (int j = 0; j < min(5, node.GetConnectorCount()); j++) {  // Limit to 5 for readability
            const ElectricNodeBase::Connector& conn = node.GetConnector(j);
            LOG("    [" << j << "] " << conn.name << " (" 
                << (conn.is_src ? "SRC" : "") << (conn.is_sink ? "SINK" : "") 
                << (conn.accept_multiconn ? "/MULTI" : "") << ")");
        }
        
        if (node.GetConnectorCount() > 5) {
            LOG("    ... and " << (node.GetConnectorCount() - 5) << " more connectors");
        }
    }
    
    LOG("Inspection test completed successfully!");
}

// Entry point for the test
void Test60_Inspect() {
    TestInspect();
    LOG("Component inspection test completed.");
}