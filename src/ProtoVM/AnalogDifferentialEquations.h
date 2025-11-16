#ifndef _ProtoVM_AnalogDifferentialEquations_h_
#define _ProtoVM_AnalogDifferentialEquations_h_

#include "AnalogCommon.h"
#include <vector>
#include <functional>

// Enum for different types of differential equation models
enum class DiffEqType {
    RC_CIRCUIT,           // RC low-pass filter
    RL_CIRCUIT,           // RL circuit
    RLC_CIRCUIT,          // RLC resonant circuit
    VAN_DER_POL_OSC,      // Van der Pol oscillator
    DUFFING_OSC,          // Duffing oscillator
    LORENZ_ATTRACTOR,     // Lorenz system (chaotic oscillator)
    RABBIT_PREDATOR,      // Lotka-Volterra equations
    CUSTOM              // User-defined differential equation
};

// Structure to hold differential equation parameters
struct DifferentialEquationParams {
    std::vector<double> coefficients;  // Coefficients for the equation
    std::vector<double> state_vars;    // Current values of state variables
    std::vector<double> derivatives;   // Current derivatives of state variables
    double time_step;                  // Simulation time step
    double simulation_time;            // Current simulation time
    
    DifferentialEquationParams() : time_step(1.0/44100.0), simulation_time(0.0) {}
};

class AnalogDifferentialEquation : public AnalogNodeBase {
public:
    typedef AnalogDifferentialEquation CLASSNAME;

    AnalogDifferentialEquation(DiffEqType type = DiffEqType::RC_CIRCUIT, 
                              const DifferentialEquationParams& params = DifferentialEquationParams());
    virtual ~AnalogDifferentialEquation() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AnalogDifferentialEquation"; }

    void SetType(DiffEqType type);
    DiffEqType GetType() const { return type; }

    void SetInput(double input);
    double GetInput() const { return input_signal; }

    double GetOutput() const { return output; }

    void SetParams(const DifferentialEquationParams& params);
    const DifferentialEquationParams& GetParams() const { return params; }

    // Runge-Kutta 4th order solver
    void SolveRK4();

    // Euler solver (simpler but less accurate)
    void SolveEuler();

    // Get internal state for debugging/visualization
    const std::vector<double>& GetState() const { return params.state_vars; }

private:
    DiffEqType type;
    DifferentialEquationParams params;
    double input_signal;
    double output;
    
    // Compute derivatives for given state and time
    std::vector<double> ComputeDerivatives(double t, const std::vector<double>& state);

    // Specific differential equation models
    std::vector<double> ComputeRCDerivatives(double t, const std::vector<double>& state);
    std::vector<double> ComputeRLDerivatives(double t, const std::vector<double>& state);
    std::vector<double> ComputeRLCDerivatives(double t, const std::vector<double>& state);
    std::vector<double> ComputeVanDerPolDerivatives(double t, const std::vector<double>& state);
    std::vector<double> ComputeDuffingDerivatives(double t, const std::vector<double>& state);
    std::vector<double> ComputeLorenzDerivatives(double t, const std::vector<double>& state);
    std::vector<double> ComputeRabbitPredatorDerivatives(double t, const std::vector<double>& state);
    std::vector<double> ComputeCustomDerivatives(double t, const std::vector<double>& state);
};

#endif