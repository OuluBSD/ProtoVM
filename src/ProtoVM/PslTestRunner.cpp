#include "PslTestRunner.h"
#include <fstream>

bool PslTestRunner::RunTest(TestNode* test_node) {
    if (!test_node) {
        LOG("Error: null test node");
        return false;
    }
    
    LOG("Running test: " << test_node->name);
    LOG("Testing circuit: " << test_node->circuit_under_test);
    
    // Setup the circuit for testing
    if (!SetupCircuitForTest(test_node->circuit_under_test)) {
        LOG("Failed to setup circuit for test");
        return false;
    }
    
    // If the circuit has stimulus defined, process it
    if (!test_node->stimulus.empty()) {
        for (size_t i = 0; i < test_node->stimulus.size(); i++) {
            LOG("Applying stimulus step " << i);
            
            // Apply the stimulus to the circuit
            if (!ApplyStimulus(test_node->stimulus[i])) {
                LOG("Failed to apply stimulus");
                return false;
            }
            
            // Run simulation for one tick
            if (!RunSimulation(1)) {
                LOG("Simulation failed during test");
                return false;
            }
            
            // Capture outputs
            std::map<String, String> outputs = CaptureOutputs();
            
            // If there are expected values for this step, validate them
            if (i < test_node->expected.size()) {
                if (!ValidateOutputs(test_node->expected[i], outputs)) {
                    LOG("Test failed: outputs don't match expected values at step " << i);
                    return false;
                }
            }
        }
    } else {
        // Just run the simulation for a default number of ticks
        if (!RunSimulation(10)) { // Default test run
            LOG("Simulation failed during test");
            return false;
        }
    }
    
    LOG("Test " << test_node->name << " completed successfully");
    return true;
}

bool PslTestRunner::RunTestFromFile(const String& filename) {
    // Read the PSL test file
    FileIn file(filename);
    if (!file) {
        LOG("Error: Could not open test file: " << filename);
        return false;
    }
    
    String content = LoadFile(filename);
    if (content.IsEmpty()) {
        LOG("Error: Empty test file: " << filename);
        return false;
    }
    
    // Tokenize the content
    PslTokenizer tokenizer(content);
    std::vector<Token> tokens = tokenizer.Tokenize();
    
    // Parse the tokens
    PslParser parser;
    parser.SetTokens(tokens);
    std::vector<PslNode*> ast = parser.Parse();
    
    bool success = true;
    
    // Find and run any test nodes
    for (PslNode* node : ast) {
        if (TestNode* test_node = dynamic_cast<TestNode*>(node)) {
            if (!RunTest(test_node)) {
                success = false;
            }
            // Don't break - run all tests in the file
        }
    }
    
    // Clean up the AST
    for (PslNode* node : ast) {
        delete node;
    }
    
    return success;
}

bool PslTestRunner::ValidateOutputs(const std::map<String, String>& expected, 
                                    const std::map<String, String>& actual) {
    bool all_match = true;
    
    for (const auto& expected_pair : expected) {
        String signal_name = expected_pair.first;
        String expected_value = expected_pair.second;
        
        auto actual_it = actual.find(signal_name);
        if (actual_it == actual.end()) {
            LOG("Expected signal " << signal_name << " not found in actual outputs");
            all_match = false;
            continue;
        }
        
        String actual_value = actual_it->second;
        if (expected_value != actual_value) {
            LOG("Signal " << signal_name << " mismatch: expected " << expected_value 
                         << ", got " << actual_value);
            all_match = false;
        } else {
            LOG("Signal " << signal_name << " matches: " << actual_value);
        }
    }
    
    if (all_match) {
        LOG("All outputs matched expected values");
    } else {
        LOG("Some outputs did not match expected values");
    }
    
    return all_match;
}

bool PslTestRunner::SetupCircuitForTest(const String& circuit_name) {
    // Create a new machine for this test
    if (machine) {
        delete machine;
    }
    machine = new Machine();
    
    // Based on the circuit name, initialize the appropriate circuit
    if (circuit_name == "simple_nand") {
        // This would call the appropriate setup function
        // For now, we'll use a generic approach
        LOG("Setting up simple_nand circuit for testing");
        // This would need to call the actual setup function for this circuit
        // In a real implementation, we would have a registry of setup functions
    } else if (circuit_name == "cpu6502") {
        LOG("Setting up cpu6502 circuit for testing");
    } else {
        LOG("Unknown circuit for testing: " << circuit_name);
        return false;
    }
    
    // Initialize the machine
    if (!machine->Init()) {
        LOG("Failed to initialize machine for test");
        return false;
    }
    
    return true;
}

bool PslTestRunner::ApplyStimulus(const std::map<String, String>& stimulus) {
    // Apply the stimulus to the circuit
    // This would involve setting input values on the appropriate components
    for (const auto& stim : stimulus) {
        String signal_name = stim.first;
        String value = stim.second;
        
        LOG("Applying stimulus: " << signal_name << " = " << value);
        
        // In a real implementation, this would find the appropriate component pin
        // and set its value. For now, we'll just log it.
    }
    
    return true; // Placeholder - implementation would depend on circuit structure
}

std::map<String, String> PslTestRunner::CaptureOutputs() {
    std::map<String, String> outputs;
    
    // Capture the current state of outputs from the circuit
    // This would involve reading output values from the appropriate components
    LOG("Capturing outputs from circuit");
    
    // Placeholder implementation - in a real system we would read from the circuit components
    // For now, return an empty map or dummy values
    outputs["dummy_output"] = "0";
    
    return outputs;
}

bool PslTestRunner::RunSimulation(int ticks) {
    if (!machine) {
        LOG("No machine available for simulation");
        return false;
    }
    
    for (int i = 0; i < ticks; i++) {
        if (!machine->Tick()) {
            LOG("Simulation tick " << i << " failed");
            return false;
        } else {
            LOG("Simulation tick " << i << " completed");
        }
    }
    
    return true;
}