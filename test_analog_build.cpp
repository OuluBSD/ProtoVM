#include <iostream>
#include "src/ProtoVMCLI/AnalogModel.h"
#include "src/ProtoVMCLI/AnalogBlockExtractor.h"
#include "src/ProtoVMCLI/AnalogSolver.h"
#include "src/ProtoVMCLI/CircuitFacade.h"
#include "src/ProtoVMCLI/JsonIO.h"
#include "src/ProtoVMCLI/CircuitGraph.h"
#include <vector>

int main() {
    std::cout << "Testing minimal compilation of analog modules..." << std::endl;
    
    // Test that we can create basic analog model
    ProtoVMCLI::AnalogBlockModel model;
    model.id = "TEST";
    std::cout << "Created AnalogBlockModel with id: " << model.id.ToStd() << std::endl;
    
    // Test that we can reference the analog solver config
    ProtoVMCLI::AnalogSolverConfig config;
    config.sample_rate_hz = 48000.0;
    std::cout << "Created AnalogSolverConfig with sample rate: " << config.sample_rate_hz << std::endl;
    
    std::cout << "All includes compiled successfully!" << std::endl;
    
    return 0;
}