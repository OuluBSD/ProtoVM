#include "ProtoVM/ProtoVM.h"
#include "ProtoVM/RCOscillator.h"
#include "ProtoVM/AnalogSimulation.h"
#include <iostream>

using namespace Upp;

// Test function to create and run a mixed-signal oscillator circuit
int TestMixedSignalOscillator() {
    std::cout << "Testing Mixed-Signal Oscillator Simulation..." << std::endl;
    
    // Create a machine (the main simulation controller)
    Machine machine;
    
    // Create a PCB for our circuit
    Pcb& pcb = machine.AddPcb();
    pcb.SetName("OscillatorTest");
    
    try {
        // Create an RC oscillator
        RCOscillator* oscillator = new RCOscillator(10000.0, 10000.0, 1e-6, 5.0); // 10kΩ, 10kΩ, 1μF, 5V
        oscillator->SetName("RC_Oscillator");
        
        // Add the oscillator to the PCB
        pcb.AddNode(*oscillator);
        
        // Register the analog component with the machine
        machine.RegisterAnalogComponent(oscillator);
        
        std::cout << "Created RC oscillator with 10kΩ resistors, 1μF capacitor, 5V supply" << std::endl;
        std::cout << "Expected frequency: approximately " 
                  << 1.0 / (0.7 * (10000.0 + 2*10000.0) * 1e-6) << " Hz" << std::endl;
        
        // Initialize the machine
        if (!machine.Init()) {
            std::cout << "Failed to initialize machine" << std::endl;
            return 1;
        }
        
        std::cout << "Machine initialized successfully" << std::endl;
        
        // Run the simulation for a few ticks to observe the oscillator behavior
        std::cout << "Running simulation for 10000 ticks..." << std::endl;
        
        int output_changes = 0;
        double prev_output = 0.0;
        
        for (int i = 0; i < 10000; i++) {
            if (!machine.Tick()) {
                std::cout << "Simulation tick failed at tick " << i << std::endl;
                return 1;
            }
            
            // Check the oscillator output
            double current_output = oscillator->GetOutputVoltage();
            if (std::abs(current_output - prev_output) > 0.1) {
                // Output changed significantly, count as a transition
                output_changes++;
                prev_output = current_output;
                
                if (i < 20 || i % 1000 == 0) { // Print first 20 transitions and every 1000 ticks
                    std::cout << "Tick " << i << ": Output changed to " << current_output << "V" << std::endl;
                }
            }
        }
        
        std::cout << "Simulation completed!" << std::endl;
        std::cout << "Total output transitions: " << output_changes << std::endl;
        
        // Estimate the frequency based on transitions
        if (output_changes > 0) {
            // Each cycle has 2 transitions (high->low and low->high)
            int cycles = output_changes / 2;
            double estimated_freq = (double)cycles / (10000 * (1.0/44100.0)); // Assuming 44.1kHz simulation rate
            std::cout << "Estimated frequency: " << estimated_freq << " Hz" << std::endl;
        }
        
        std::cout << "Mixed-signal oscillator test completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cout << "Exception in oscillator test: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Unknown exception in oscillator test" << std::endl;
        return 1;
    }
}

int main() {
    std::cout << "ProtoVM Mixed-Signal Simulation Test" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    int result = TestMixedSignalOscillator();
    
    if (result == 0) {
        std::cout << std::endl << "All tests passed!" << std::endl;
    } else {
        std::cout << std::endl << "Tests failed!" << std::endl;
    }
    
    return result;
}