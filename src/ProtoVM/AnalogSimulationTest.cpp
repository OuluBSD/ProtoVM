#include "ProtoVM/ProtoVM.h"
#include "ProtoVM/AnalogSimulation.h"
#include "ProtoVM/AnalogComponents.h"
#include <iostream>
#include <iomanip>
#include <cmath>

// This test demonstrates a simple analog simulation with actual component behavior
void RunAnalogResistorCapacitorSimulation() {
    std::cout << "ProtoVM Analog RC Simulation Test" << std::endl;
    std::cout << "=================================" << std::endl;

    // Create a machine for the simulation
    Machine mach;

    // Create a PCB for the test
    Pcb& pcb = mach.AddPcb();

    // Create analog components
    AnalogResistor* resistor = new AnalogResistor(1000.0);  // 1kΩ
    AnalogCapacitor* capacitor = new AnalogCapacitor(1e-6);  // 1μF

    // Initialize component values
    resistor->SetAnalogValue(0, 5.0);  // 5V input
    resistor->SetAnalogValue(1, 0.0);  // Initially 0V output
    capacitor->SetAnalogValue(0, 0.0); // Initially 0V across capacitor
    capacitor->SetAnalogValue(1, 0.0); // Ground

    // Register with the machine's analog simulation system
    mach.RegisterAnalogComponent(resistor);
    mach.RegisterAnalogComponent(capacitor);

    // Initialize the machine
    if (!mach.Init()) {
        std::cout << "Failed to initialize the machine" << std::endl;
        delete resistor;
        delete capacitor;
        return;
    }

    std::cout << "Testing combined resistor-capacitor behavior with simulation:" << std::endl;
    std::cout << "Resistor: 1kΩ, Capacitor: 1μF, RC time constant = 1ms" << std::endl;
    std::cout << "\nTime(s)\tResistor In\tResistor Out\tCapacitor V\tExpected V\tError" << std::endl;
    std::cout << "------\t----------\t-----------\t-----------\t----------\t-----" << std::endl;

    // Run the simulation for a few ticks to observe the RC behavior
    double time_step = 0.001;  // 1ms time step
    double simulation_time = 0.0;
    int steps = 10;  // 10ms test duration

    for (int i = 0; i < steps; i++) {
        // Run analog simulation to update component behaviors
        mach.RunAnalogSimulation();

        // Run digital simulation
        if (!mach.Tick()) {
            std::cout << "Simulation halted at tick " << i << std::endl;
            break;
        }

        // Get the current values
        double resistor_input = resistor->GetAnalogValue(0);
        double resistor_output = resistor->GetAnalogValue(1);
        double capacitor_voltage = capacitor->GetAnalogValue(0);  // Same as resistor output in series

        // Calculate expected voltage for RC charging: V(t) = V0 * (1 - e^(-t/RC))
        double rc_time_constant = 1000.0 * 1e-6;  // 1ms
        double expected_voltage = 5.0 * (1.0 - exp(-simulation_time / rc_time_constant));
        double error = std::abs(capacitor_voltage - expected_voltage);

        // Print results
        std::cout << std::fixed << std::setprecision(4) 
                  << simulation_time << "\t"
                  << resistor_input << "\t\t"
                  << resistor_output << "\t\t"
                  << capacitor_voltage << "\t\t"
                  << expected_voltage << "\t\t"
                  << error << std::endl;

        simulation_time += time_step;
    }

    std::cout << "\nAnalog RC simulation test completed successfully!" << std::endl;

    // Cleanup
    delete resistor;
    delete capacitor;
}

void SetupAnalogResistorCapacitorSimulation(Machine& mach) {
    std::cout << "Setting up Analog RC Simulation Test Circuit..." << std::endl;
    std::cout << "This test demonstrates actual analog simulation behavior" << std::endl;
    std::cout << "Components:" << std::endl;
    std::cout << "  - Analog Resistor: 1kΩ" << std::endl;
    std::cout << "  - Analog Capacitor: 1μF" << std::endl;
    std::cout << "  - Demonstrates combined RC behavior" << std::endl;
}