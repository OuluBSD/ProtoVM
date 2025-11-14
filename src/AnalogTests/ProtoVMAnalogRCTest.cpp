#include "ProtoVM/ProtoVM.h"
#include "ProtoVM/AnalogSimulation.h"
#include "ProtoVM/AnalogComponents.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// Test circuit for Resistor-Capacitor (RC) combination
class ProtoVMAnalogRCTest {
public:
    ProtoVMAnalogRCTest() : 
        time_step(0.001),  // 1ms time step
        simulation_time(0.0),
        test_duration(0.05) {  // 50ms test duration
    }

    bool RunTest() {
        std::cout << "ProtoVM Analog RC Circuit Test" << std::endl;
        std::cout << "==============================" << std::endl;

        // Create analog simulation environment
        AnalogSimulation analog_sim;
        analog_sim.SetTimeStep(time_step);

        // Create a resistor and capacitor in series
        AnalogResistor* resistor = new AnalogResistor(1000.0);  // 1kΩ resistor
        resistor->SetName("RC_TestResistor_1k");
        
        AnalogCapacitor* capacitor = new AnalogCapacitor(1e-6);  // 1μF capacitor
        capacitor->SetName("RC_TestCapacitor_1uF");
        
        // Register components with the analog simulation
        analog_sim.RegisterAnalogComponent(resistor);
        analog_sim.RegisterAnalogComponent(capacitor);

        // Create a simple RC circuit:
        // Voltage source -> Resistor -> Capacitor -> Ground
        // We'll simulate this by controlling the voltages applied to components
        
        // Initially 0V across both components
        resistor->SetAnalogValue(0, 0.0);  // Input to resistor
        resistor->SetAnalogValue(1, 0.0);  // Output from resistor (connected to cap)
        capacitor->SetAnalogValue(0, 0.0); // Input to capacitor (connected to resistor)
        capacitor->SetAnalogValue(1, 0.0); // Ground for capacitor

        double rc_time_constant = resistor->GetResistance() * capacitor->GetCapacitance();
        std::cout << "Testing RC circuit with 1kΩ resistor and 1μF capacitor" << std::endl;
        std::cout << "RC time constant: " << rc_time_constant << "s = " << rc_time_constant * 1000 << "ms" << std::endl;
        std::cout << "\nTime\tR Input\tR Output\tC Output\tExpected (V)\tError (V)" << std::endl;
        std::cout << "----\t-------\t--------\t--------\t------------\t---------" << std::endl;

        int steps = static_cast<int>(test_duration / time_step);
        for (int i = 0; i < steps; i++) {
            // Apply step voltage to input at the beginning
            if (i == 0) {
                resistor->SetAnalogValue(0, 5.0);  // Apply 5V step
            }

            // Simulate the analog behavior
            if (!analog_sim.Tick()) {
                std::cerr << "Analog simulation failed at step " << i << std::endl;
                return false;
            }

            // Get voltages at different points
            double resistor_input = resistor->GetAnalogValue(0);
            double resistor_output = resistor->GetAnalogValue(1);
            double capacitor_output = capacitor->GetAnalogValue(0);  // Same as resistor output in series
            
            // Calculate expected voltage for RC charging: V(t) = V0 * (1 - e^(-t/RC))
            double expected_voltage = 5.0 * (1.0 - exp(-simulation_time / rc_time_constant));
            double error = std::abs(capacitor_output - expected_voltage);

            // Print results
            std::cout << std::fixed << std::setprecision(4) 
                      << simulation_time << "\t"
                      << resistor_input << "\t"
                      << resistor_output << "\t\t"
                      << capacitor_output << "\t\t"
                      << expected_voltage << "\t\t"
                      << error << std::endl;

            simulation_time += time_step;
        }

        std::cout << "\nRC circuit test completed successfully!" << std::endl;
        std::cout << "Final capacitor voltage: " << std::fixed << std::setprecision(4) 
                  << capacitor->GetAnalogValue(0) << "V" << std::endl;
        
        // Cleanup
        delete resistor;
        delete capacitor;
        
        return true;
    }

private:
    double time_step;
    double simulation_time;
    double test_duration;
};

int main() {
    ProtoVMAnalogRCTest test;
    if (test.RunTest()) {
        return 0;
    } else {
        std::cerr << "RC circuit test failed!" << std::endl;
        return 1;
    }
}