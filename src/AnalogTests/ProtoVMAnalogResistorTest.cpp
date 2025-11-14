#include "ProtoVM/ProtoVM.h"
#include "ProtoVM/AnalogSimulation.h"
#include "ProtoVM/AnalogComponents.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// Test circuit for AnalogResistor
class ProtoVMAnalogResistorTest {
public:
    ProtoVMAnalogResistorTest() : 
        time_step(0.001),  // 1ms time step
        simulation_time(0.0),
        test_duration(0.05) {  // 50ms test duration
    }

    bool RunTest() {
        std::cout << "ProtoVM Analog Resistor Test" << std::endl;
        std::cout << "============================" << std::endl;

        // Create analog simulation environment
        AnalogSimulation analog_sim;
        analog_sim.SetTimeStep(time_step);

        // Create a voltage source (simplified - we'll directly set values)
        // and a resistor to test
        AnalogResistor* resistor = new AnalogResistor(1000.0);  // 1kΩ resistor
        resistor->SetName("TestResistor_1k");
        
        // Register the resistor with the analog simulation
        analog_sim.RegisterAnalogComponent(resistor);

        // Test with a step input voltage of 5V
        resistor->SetAnalogValue(0, 5.0);  // Apply 5V at terminal A
        resistor->SetAnalogValue(1, 0.0);  // Ground terminal B

        std::cout << "Testing resistor with 5V input, 1kΩ resistance" << std::endl;
        std::cout << "Expected current: 5mA (according to Ohm's Law: I = V/R = 5V/1000Ω)" << std::endl;
        std::cout << "\nTime\tTerminal A\tTerminal B\tCurrent(mA)\tPower(mW)" << std::endl;
        std::cout << "----\t----------\t----------\t-----------\t---------" << std::endl;

        int steps = static_cast<int>(test_duration / time_step);
        for (int i = 0; i < steps; i++) {
            // Simulate the analog behavior
            if (!analog_sim.Tick()) {
                std::cerr << "Analog simulation failed at step " << i << std::endl;
                return false;
            }

            // Calculate current through the resistor
            double voltage_a = resistor->GetAnalogValue(0);
            double voltage_b = resistor->GetAnalogValue(1);
            double voltage_diff = voltage_a - voltage_b;
            double current = voltage_diff / resistor->GetResistance();
            double power = voltage_diff * current;

            // Print results
            std::cout << std::fixed << std::setprecision(4) 
                      << simulation_time << "\t"
                      << voltage_a << "\t\t"
                      << voltage_b << "\t\t"
                      << current * 1000 << "\t\t"  // Convert to mA
                      << power * 1000 << std::endl;  // Convert to mW

            simulation_time += time_step;
        }

        std::cout << "\nResistor test completed successfully!" << std::endl;
        
        // Cleanup
        delete resistor;
        
        return true;
    }

private:
    double time_step;
    double simulation_time;
    double test_duration;
};

int main() {
    ProtoVMAnalogResistorTest test;
    if (test.RunTest()) {
        return 0;
    } else {
        std::cerr << "Resistor test failed!" << std::endl;
        return 1;
    }
}