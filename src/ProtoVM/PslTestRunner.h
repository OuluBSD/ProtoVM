#ifndef _ProtoVM_PslTestRunner_h_
#define _ProtoVM_PslTestRunner_h_

#include "ProtoVM.h"
#include "PslParser.h"

class PslTestRunner {
private:
    Machine* machine;
    
public:
    PslTestRunner() : machine(nullptr) {}
    
    // Run a test defined in PSL
    bool RunTest(TestNode* test_node);
    
    // Run a test from PSL file
    bool RunTestFromFile(const String& filename);
    
    // Validate expected outputs against actual outputs
    bool ValidateOutputs(const std::map<String, String>& expected, 
                         const std::map<String, String>& actual);
    
    // Setup the circuit for testing
    bool SetupCircuitForTest(const String& circuit_name);
    
    // Apply stimulus to the circuit
    bool ApplyStimulus(const std::map<String, String>& stimulus);
    
    // Capture outputs from the circuit
    std::map<String, String> CaptureOutputs();
    
    // Run the simulation for a specified number of ticks
    bool RunSimulation(int ticks = 1);
};

#endif