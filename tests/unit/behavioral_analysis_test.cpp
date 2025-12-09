#include "ProtoVM.h"
#include "../src/ProtoVMCLI/BehavioralAnalysis.h"
#include "../src/ProtoVMCLI/BlockAnalysis.h"
#include "../src/ProtoVMCLI/CircuitGraph.h"
#include "../src/ProtoVMCLI/SessionTypes.h"
#include <iostream>

using namespace Upp;
using namespace ProtoVMCLI;

void TestBehavioralInferenceBasic() {
    std::cout << "Testing basic behavioral inference functionality...\n";
    
    // Create a simple block instance to test with
    BlockInstance block;
    block.id = "B1";
    block.kind = BlockKind::Adder;
    
    // Add some ports to the block
    block.ports.push_back(BlockPort("A", "in", {"C1:A", "C2:A"}));
    block.ports.push_back(BlockPort("B", "in", {"C1:B", "C2:B"}));
    block.ports.push_back(BlockPort("SUM", "out", {"C3:SUM"}));
    block.ports.push_back(BlockPort("CIN", "in", {"C0:CIN"}));
    block.ports.push_back(BlockPort("COUT", "out", {"C3:COUT"}));
    
    // Create a circuit graph (empty for this test)
    CircuitGraph graph;
    
    // Test the behavioral analysis
    BehavioralAnalysis analysis;
    auto result = analysis.InferBehaviorForBlock(block, graph);
    
    if (result.ok) {
        std::cout << "✓ Behavioral inference succeeded\n";
        std::cout << "  Subject ID: " << result.data.subject_id << "\n";
        std::cout << "  Subject Kind: " << result.data.subject_kind << "\n";
        std::cout << "  Behavior Kind: ";
        
        switch(result.data.behavior_kind) {
            case BehaviorKind::Adder: std::cout << "Adder\n"; break;
            case BehaviorKind::Mux: std::cout << "Mux\n"; break;
            case BehaviorKind::Register: std::cout << "Register\n"; break;
            case BehaviorKind::Unknown: std::cout << "Unknown\n"; break;
            case BehaviorKind::CombinationalLogic: std::cout << "CombinationalLogic\n"; break;
            case BehaviorKind::Subtractor: std::cout << "Subtractor\n"; break;
            case BehaviorKind::Comparator: std::cout << "Comparator\n"; break;
            case BehaviorKind::EqualityComparator: std::cout << "EqualityComparator\n"; break;
            case BehaviorKind::InequalityComparator: std::cout << "InequalityComparator\n"; break;
            case BehaviorKind::Decoder: std::cout << "Decoder\n"; break;
            case BehaviorKind::Encoder: std::cout << "Encoder\n"; break;
            case BehaviorKind::Counter: std::cout << "Counter\n"; break;
            case BehaviorKind::StateMachine: std::cout << "StateMachine\n"; break;
            default: std::cout << "Other\n"; break;
        }
        
        std::cout << "  Bit Width: " << result.data.bit_width << "\n";
        std::cout << "  Description: " << result.data.description << "\n";
        std::cout << "  Port Count: " << result.data.ports.size() << "\n";
        
        for (const auto& port : result.data.ports) {
            std::cout << "    Port: " << port.port_name << " -> " << port.role << "\n";
        }
        
        // Verify expected behavior
        if (result.data.behavior_kind == BehaviorKind::Adder) {
            std::cout << "✓ Correctly identified as Adder\n";
        } else {
            std::cout << "✗ Expected Adder but got different behavior type\n";
        }
        
        if (result.data.description.find("adder") != std::string::npos) {
            std::cout << "✓ Description contains 'adder'\n";
        } else {
            std::cout << "✗ Description doesn't mention 'adder'\n";
        }
    } else {
        std::cout << "✗ Behavioral inference failed: " << result.error_message << "\n";
    }
    
    std::cout << "\n";
}

void TestBehavioralInferenceMux() {
    std::cout << "Testing multiplexer behavioral inference...\n";
    
    // Create a mux block instance
    BlockInstance block;
    block.id = "B2";
    block.kind = BlockKind::Mux;
    
    // Add ports for a 2:1 mux
    block.ports.push_back(BlockPort("IN0", "in", {"C4:IN0"}));
    block.ports.push_back(BlockPort("IN1", "in", {"C4:IN1"}));
    block.ports.push_back(BlockPort("SEL", "in", {"C5:SEL"}));
    block.ports.push_back(BlockPort("OUT", "out", {"C6:OUT"}));
    
    // Create a circuit graph
    CircuitGraph graph;
    
    // Test the behavioral analysis
    BehavioralAnalysis analysis;
    auto result = analysis.InferBehaviorForBlock(block, graph);
    
    if (result.ok) {
        std::cout << "✓ Mux behavioral inference succeeded\n";
        std::cout << "  Behavior Kind: ";
        
        switch(result.data.behavior_kind) {
            case BehaviorKind::Mux: std::cout << "Mux\n"; break;
            default: std::cout << "Other\n"; break;
        }
        
        std::cout << "  Description: " << result.data.description << "\n";
        
        if (result.data.behavior_kind == BehaviorKind::Mux) {
            std::cout << "✓ Correctly identified as Mux\n";
        } else {
            std::cout << "✗ Expected Mux but got different behavior type\n";
        }
        
        if (result.data.description.find("multiplexer") != std::string::npos) {
            std::cout << "✓ Description contains 'multiplexer'\n";
        } else {
            std::cout << "✗ Description doesn't mention 'multiplexer'\n";
        }
    } else {
        std::cout << "✗ Mux behavioral inference failed: " << result.error_message << "\n";
    }
    
    std::cout << "\n";
}

void TestBehavioralInferenceRegister() {
    std::cout << "Testing register behavioral inference...\n";
    
    // Create a register block instance
    BlockInstance block;
    block.id = "B3";
    block.kind = BlockKind::Register;
    
    // Add ports for a register
    block.ports.push_back(BlockPort("D", "in", {"C7:D"}));
    block.ports.push_back(BlockPort("CLK", "in", {"C8:CLK"}));
    block.ports.push_back(BlockPort("Q", "out", {"C9:Q"}));
    
    // Create a circuit graph
    CircuitGraph graph;
    
    // Test the behavioral analysis
    BehavioralAnalysis analysis;
    auto result = analysis.InferBehaviorForBlock(block, graph);
    
    if (result.ok) {
        std::cout << "✓ Register behavioral inference succeeded\n";
        std::cout << "  Behavior Kind: ";
        
        switch(result.data.behavior_kind) {
            case BehaviorKind::Register: std::cout << "Register\n"; break;
            default: std::cout << "Other\n"; break;
        }
        
        std::cout << "  Description: " << result.data.description << "\n";
        
        if (result.data.behavior_kind == BehaviorKind::Register) {
            std::cout << "✓ Correctly identified as Register\n";
        } else {
            std::cout << "✗ Expected Register but got different behavior type\n";
        }
    } else {
        std::cout << "✗ Register behavioral inference failed: " << result.error_message << "\n";
    }
    
    std::cout << "\n";
}

void TestEmptyBlock() {
    std::cout << "Testing empty block behavioral inference...\n";
    
    // Create an empty block instance
    BlockInstance block;
    block.id = "B4";
    block.kind = BlockKind::GenericComb;  // Generic combinational block
    
    // Create a circuit graph
    CircuitGraph graph;
    
    // Test the behavioral analysis
    BehavioralAnalysis analysis;
    auto result = analysis.InferBehaviorForBlock(block, graph);
    
    if (result.ok) {
        std::cout << "✓ Empty block behavioral inference succeeded\n";
        std::cout << "  Behavior Kind: ";
        
        switch(result.data.behavior_kind) {
            case BehaviorKind::CombinationalLogic: std::cout << "CombinationalLogic\n"; break;
            default: std::cout << "Other\n"; break;
        }
        
        std::cout << "  Description: " << result.data.description << "\n";
        
        if (result.data.behavior_kind == BehaviorKind::CombinationalLogic) {
            std::cout << "✓ Correctly identified as CombinationalLogic\n";
        } else {
            std::cout << "✗ Expected CombinationalLogic but got different behavior type\n";
        }
    } else {
        std::cout << "✗ Empty block behavioral inference failed: " << result.error_message << "\n";
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "ProtoVM Behavioral Analysis Unit Tests\n";
    std::cout << "========================================\n\n";
    
    TestBehavioralInferenceBasic();
    TestBehavioralInferenceMux();
    TestBehavioralInferenceRegister();
    TestEmptyBlock();
    
    std::cout << "Behavioral analysis tests completed.\n";
    
    return 0;
}