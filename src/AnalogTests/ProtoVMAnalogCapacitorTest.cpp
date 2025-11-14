#include "ProtoVM/ProtoVM.h"
#include "ProtoVM/AnalogSimulation.h"
#include "ProtoVM/AnalogComponents.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// Test circuit for AnalogCapacitor
class ProtoVMAnalogCapacitorTest {
public:
    ProtoVMAnalogCapacitorTest() : 
        time_step(0.001),  // 1ms time step
        simulation_time(0.0),
        test_duration(0.05) {  // 50ms test duration
    }

    bool RunTest() {
        std::cout << "ProtoVM Analog Capacitor Test" << std::endl;
        std::cout << "=============================" << std::endl;

        // Create analog simulation environment
        AnalogSimulation analog_sim;
        analog_sim.SetTimeStep(time_step);

        // Create a capacitor to test
        AnalogCapacitor* capacitor = new AnalogCapacitor(1e-3);  // 1mF capacitor
        capacitor->SetName("TestCapacitor_1mF");
        
        // Register the capacitor with the analog simulation
        analog_sim.RegisterAnalogComponent(capacitor);

        // Test charging with a step input voltage of 5V
        // Initially 0V across the capacitor
        capacitor->SetAnalogValue(0, 0.0);  // Start at 0V
        capacitor->SetAnalogValue(1, 0.0);  // Ground

        std::cout << "Testing capacitor charging with 5V step input, 1mF capacitance" << std::endl;
        std::cout << "RC time constant: " << capacitor->GetCapacitance() * 1000.0 << "s (with 1kΩ equivalent)" << std::endl;
        std::cout << "\nTime\tTerminal +\tTerminal -\tCap Voltage\tExpected (V)\tError (V)" << std::endl;
        std::cout << "----\t----------\t----------\t-----------\t------------\t---------" << std::endl;

        int steps = static_cast<int>(test_duration / time_step);
        for (int i = 0; i < steps; i++) {
            // Apply charging voltage at terminal 0
            if (i == 0) {
                // Apply step voltage at the beginning
                capacitor->SetAnalogValue(0, 5.0);
            }

            // Simulate the analog behavior
            if (!analog_sim.Tick()) {
                std::cerr << "Analog simulation failed at step " << i << std::endl;
                return false;
            }

            // Get the capacitor voltage
            double voltage_pos = capacitor->GetAnalogValue(0);
            double voltage_neg = capacitor->GetAnalogValue(1);
            double capacitor_voltage = voltage_pos - voltage_neg;

            // Calculate expected voltage for RC charging: V(t) = V0 * (1 - e^(-t/RC))
            // Assuming an equivalent resistance of 1kΩ for this test
            double equivalent_resistance = 1000.0;  // 1kΩ
            double rc_time_constant = equivalent_resistance * capacitor->GetCapacitance();  // 1 second for 1mF and 1kΩ
            double expected_voltage = 5.0 * (1.0 - exp(-simulation_time / rc_time_constant));
            double error = std::abs(capacitor_voltage - expected_voltage);

            // Print results
            std::cout << std::fixed << std::setprecision(4) 
                      << simulation_time << "\t"
                      << voltage_pos << "\t\t"
                      << voltage_neg << "\t\t"
                      << capacitor_voltage << "\t\t"
                      << expected_voltage << "\t\t"
                      << error << std::endl;

            simulation_time += time_step;
        }

        std::cout << "\nCapacitor test completed successfully!" << std::endl;
        
        // Cleanup
        delete capacitor;
        
        return true;
    }

private:
    double time_step;
    double simulation_time;
    double test_duration;
};

int main() {
    ProtoVMAnalogCapacitorTest test;
    if (test.RunTest()) {
        return 0;
    } else {
        std::cerr << "Capacitor test failed!" << std::endl;
        return 1;
    }
}