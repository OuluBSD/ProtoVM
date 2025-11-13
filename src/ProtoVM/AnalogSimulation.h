#ifndef _ProtoVM_AnalogSimulation_h_
#define _ProtoVM_AnalogSimulation_h_

#include "AnalogCommon.h"
#include "AnalogComponents.h"
#include "AnalogSemiconductors.h"
#include "Machine.h"
#include <vector>

// Analog simulation system that works alongside digital simulation
class AnalogSimulation {
public:
    AnalogSimulation();
    
    // Add an analog component to the simulation
    void RegisterAnalogComponent(AnalogNodeBase* component);
    
    // Run the analog portion of the simulation
    bool Tick();
    
    // Solve the system of equations for the analog components
    bool SolveAnalogSystem();
    
    // Calculate the Jacobian matrix for Newton-Raphson method
    bool CalculateJacobian();
    
    // Solve linear system of equations
    bool SolveLinearSystem();
    
    // Perform Newton-Raphson iteration to solve non-linear circuits
    bool NewtonRaphsonIteration();
    
    // Set simulation parameters
    void SetTimeStep(double dt);
    void SetMaxIterations(int max_iter);
    void SetTolerance(double tol);
    
    // Get simulation parameters
    double GetTimeStep() const { return time_step; }
    int GetMaxIterations() const { return max_iterations; }
    double GetTolerance() const { return tolerance; }
    
private:
    std::vector<AnalogNodeBase*> analog_components;
    
    // Simulation parameters
    double time_step;
    int max_iterations;
    double tolerance;
    
    // System state (voltages at each node)
    std::vector<double> node_voltages;
    
    // Jacobian matrix for solving non-linear systems
    std::vector<std::vector<double>> jacobian;
    
    // Solution vectors
    std::vector<double> residuals;
    std::vector<double> corrections;
    
    // Node to component mapping for circuit analysis
    std::vector<std::vector<AnalogNodeBase*>> node_connections;
    
    // Initialize the node voltage vector based on components
    void InitializeNodeVoltages();
    
    // Build the system of equations from the circuit
    void BuildSystemEquations();
    
    // Update all component values after solving
    void UpdateComponentValues();
};

#endif