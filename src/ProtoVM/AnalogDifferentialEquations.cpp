#include "AnalogDifferentialEquations.h"
#include <cmath>
#include <algorithm>

AnalogDifferentialEquation::AnalogDifferentialEquation(DiffEqType type, const DifferentialEquationParams& params)
    : type(type)
    , params(params)
    , input_signal(0.0)
    , output(0.0)
{
    // Initialize state variables if empty
    if (this->params.state_vars.empty()) {
        switch (type) {
            case DiffEqType::RC_CIRCUIT:
                this->params.state_vars = {0.0};  // Voltage across capacitor
                break;
            case DiffEqType::RL_CIRCUIT:
                this->params.state_vars = {0.0};  // Current through inductor
                break;
            case DiffEqType::RLC_CIRCUIT:
                this->params.state_vars = {0.0, 0.0};  // Voltage across capacitor, current through inductor
                break;
            case DiffEqType::VAN_DER_POL_OSC:
                this->params.state_vars = {0.0, 0.0};  // Position, velocity
                this->params.coefficients.resize(1, 1.0);  // Mu parameter
                break;
            case DiffEqType::DUFFING_OSC:
                this->params.state_vars = {0.0, 0.0};  // Position, velocity
                this->params.coefficients.resize(2, 1.0);  // Alpha and beta parameters
                break;
            case DiffEqType::LORENZ_ATTRACTOR:
                this->params.state_vars = {0.1, 0.0, 0.0};  // X, Y, Z
                this->params.coefficients.resize(3, 1.0);  // Sigma, rho, beta parameters
                break;
            case DiffEqType::RABBIT_PREDATOR:
                this->params.state_vars = {1.0, 1.0};  // Prey, predator populations
                this->params.coefficients.resize(4, 1.0);  // Alpha, beta, gamma, delta parameters
                break;
            case DiffEqType::CUSTOM:
            default:
                this->params.state_vars = {0.0};
                break;
        }
    }
    
    // Initialize derivatives
    this->params.derivatives.resize(this->params.state_vars.size(), 0.0);
}

bool AnalogDifferentialEquation::Tick() {
    // Update simulation time
    params.simulation_time += params.time_step;
    
    // Solve the differential equation
    SolveRK4();  // Use the more accurate Runge-Kutta method by default
    
    // Set output to the first state variable (common case)
    if (!params.state_vars.empty()) {
        output = params.state_vars[0];
        
        // For some systems, we might want a different output
        switch (type) {
            case DiffEqType::RLC_CIRCUIT:
                // For RLC, voltage across capacitor is typically the output
                output = params.state_vars[0];
                break;
            case DiffEqType::LORENZ_ATTRACTOR:
                // For Lorenz, we might want the X component as output
                output = params.state_vars[0];
                break;
            default:
                output = params.state_vars[0];
                break;
        }
    } else {
        output = 0.0;
    }
    
    return true;
}

void AnalogDifferentialEquation::SetType(DiffEqType type) {
    this->type = type;
    
    // Reset state variables based on new type
    if (params.state_vars.empty()) {
        switch (type) {
            case DiffEqType::RC_CIRCUIT:
                params.state_vars = {0.0};
                break;
            case DiffEqType::RL_CIRCUIT:
                params.state_vars = {0.0};
                break;
            case DiffEqType::RLC_CIRCUIT:
                params.state_vars = {0.0, 0.0};
                break;
            case DiffEqType::VAN_DER_POL_OSC:
                params.state_vars = {0.0, 0.0};
                params.coefficients.resize(1, 1.0);
                break;
            case DiffEqType::DUFFING_OSC:
                params.state_vars = {0.0, 0.0};
                params.coefficients.resize(2, 1.0);
                break;
            case DiffEqType::LORENZ_ATTRACTOR:
                params.state_vars = {0.1, 0.0, 0.0};
                params.coefficients.resize(3, 1.0);
                break;
            case DiffEqType::RABBIT_PREDATOR:
                params.state_vars = {1.0, 1.0};
                params.coefficients.resize(4, 1.0);
                break;
            case DiffEqType::CUSTOM:
            default:
                params.state_vars = {0.0};
                break;
        }
        
        params.derivatives.resize(params.state_vars.size(), 0.0);
    }
}

void AnalogDifferentialEquation::SetInput(double input) {
    this->input_signal = input;
}

void AnalogDifferentialEquation::SetParams(const DifferentialEquationParams& params) {
    this->params = params;
    
    // Ensure derivatives vector size matches state variables
    this->params.derivatives.resize(this->params.state_vars.size(), 0.0);
}

std::vector<double> AnalogDifferentialEquation::ComputeDerivatives(double t, const std::vector<double>& state) {
    switch (type) {
        case DiffEqType::RC_CIRCUIT:
            return ComputeRCDerivatives(t, state);
        case DiffEqType::RL_CIRCUIT:
            return ComputeRLDerivatives(t, state);
        case DiffEqType::RLC_CIRCUIT:
            return ComputeRLCDerivatives(t, state);
        case DiffEqType::VAN_DER_POL_OSC:
            return ComputeVanDerPolDerivatives(t, state);
        case DiffEqType::DUFFING_OSC:
            return ComputeDuffingDerivatives(t, state);
        case DiffEqType::LORENZ_ATTRACTOR:
            return ComputeLorenzDerivatives(t, state);
        case DiffEqType::RABBIT_PREDATOR:
            return ComputeRabbitPredatorDerivatives(t, state);
        case DiffEqType::CUSTOM:
            return ComputeCustomDerivatives(t, state);
        default:
            return std::vector<double>(state.size(), 0.0);
    }
}

std::vector<double> AnalogDifferentialEquation::ComputeRCDerivatives(double t, const std::vector<double>& state) {
    std::vector<double> derivatives(1, 0.0);
    
    // RC circuit differential equation: dVc/dt = (Vin - Vc)/(R*C)
    // For simulation purposes, we'll use typical values if not provided in coefficients
    double R = 1000.0;  // 1kOhm
    double C = 1e-6;    // 1uF
    if (params.coefficients.size() >= 2) {
        R = params.coefficients[0];
        C = params.coefficients[1];
    }
    
    double RC = R * C;
    derivatives[0] = (input_signal - state[0]) / RC;
    
    return derivatives;
}

std::vector<double> AnalogDifferentialEquation::ComputeRLDerivatives(double t, const std::vector<double>& state) {
    std::vector<double> derivatives(1, 0.0);
    
    // RL circuit differential equation: dI/dt = (Vin - R*I)/L
    double R = 10.0;    // 10 Ohm
    double L = 0.1;     // 100mH
    if (params.coefficients.size() >= 2) {
        R = params.coefficients[0];
        L = params.coefficients[1];
    }
    
    derivatives[0] = (input_signal - R * state[0]) / L;
    
    return derivatives;
}

std::vector<double> AnalogDifferentialEquation::ComputeRLCDerivatives(double t, const std::vector<double>& state) {
    // State[0] = voltage across capacitor, State[1] = current through inductor
    std::vector<double> derivatives(2, 0.0);
    
    // For RLC circuit:
    // dVc/dt = Ic/C = -I/C  (current flows out of capacitor)
    // dI/dt = (Vin - I*R - Vc)/L
    double R = 10.0;    // 10 Ohm
    double L = 0.1;     // 100mH
    double C = 1e-6;    // 1uF
    if (params.coefficients.size() >= 3) {
        R = params.coefficients[0];
        L = params.coefficients[1];
        C = params.coefficients[2];
    }
    
    derivatives[0] = -state[1] / C;
    derivatives[1] = (input_signal - state[1] * R - state[0]) / L;
    
    return derivatives;
}

std::vector<double> AnalogDifferentialEquation::ComputeVanDerPolDerivatives(double t, const std::vector<double>& state) {
    // Van der Pol oscillator: d²x/dt² - μ(1 - x²)(dx/dt) + x = 0
    // Converted to system: dx/dt = y, dy/dt = μ((1-x²)y - x)
    // State[0] = x, State[1] = y (velocity)
    std::vector<double> derivatives(2, 0.0);
    
    double mu = 1.0;
    if (params.coefficients.size() > 0) {
        mu = params.coefficients[0];
    }
    
    derivatives[0] = state[1];  // dx/dt = y
    derivatives[1] = mu * ((1.0 - state[0] * state[0]) * state[1] - state[0]);  // dy/dt
    
    return derivatives;
}

std::vector<double> AnalogDifferentialEquation::ComputeDuffingDerivatives(double t, const std::vector<double>& state) {
    // Duffing oscillator: d²x/dt² + δ*dx/dt + α*x + β*x³ = γ*cos(ω*t)
    // Converted to system: dx/dt = v, dv/dt = γ*cos(ω*t) - δ*v - α*x - β*x³
    // State[0] = x, State[1] = v (velocity)
    std::vector<double> derivatives(2, 0.0);
    
    double alpha = 1.0, beta = 1.0, delta = 0.1, gamma = 0.3, omega = 1.2;
    if (params.coefficients.size() >= 2) {
        alpha = params.coefficients[0];
        beta = params.coefficients[1];
    }
    if (params.coefficients.size() >= 5) {
        delta = params.coefficients[2];
        gamma = params.coefficients[3];
        omega = params.coefficients[4];
    }
    
    derivatives[0] = state[1];  // dx/dt = v
    derivatives[1] = gamma * cos(omega * t) - delta * state[1] - alpha * state[0] - beta * state[0] * state[0] * state[0];  // dv/dt
    
    return derivatives;
}

std::vector<double> AnalogDifferentialEquation::ComputeLorenzDerivatives(double t, const std::vector<double>& state) {
    // Lorenz system: dx/dt = σ*(y-x), dy/dt = x*(ρ-z) - y, dz/dt = x*y - β*z
    // State[0] = x, State[1] = y, State[2] = z
    std::vector<double> derivatives(3, 0.0);
    
    double sigma = 10.0, rho = 28.0, beta = 8.0/3.0;
    if (params.coefficients.size() >= 3) {
        sigma = params.coefficients[0];
        rho = params.coefficients[1];
        beta = params.coefficients[2];
    }
    
    derivatives[0] = sigma * (state[1] - state[0]);
    derivatives[1] = state[0] * (rho - state[2]) - state[1];
    derivatives[2] = state[0] * state[1] - beta * state[2];
    
    return derivatives;
}

std::vector<double> AnalogDifferentialEquation::ComputeRabbitPredatorDerivatives(double t, const std::vector<double>& state) {
    // Lotka-Volterra equations: dx/dt = α*x - β*x*y, dy/dt = δ*x*y - γ*y
    // x = prey population, y = predator population
    // State[0] = x (prey), State[1] = y (predator)
    std::vector<double> derivatives(2, 0.0);
    
    double alpha = 1.0, beta = 0.1, gamma = 1.0, delta = 0.1;
    if (params.coefficients.size() >= 4) {
        alpha = params.coefficients[0];
        beta = params.coefficients[1];
        gamma = params.coefficients[2];
        delta = params.coefficients[3];
    }
    
    derivatives[0] = alpha * state[0] - beta * state[0] * state[1];  // Prey growth and predation
    derivatives[1] = delta * state[0] * state[1] - gamma * state[1];  // Predation benefit and predator death
    
    return derivatives;
}

std::vector<double> AnalogDifferentialEquation::ComputeCustomDerivatives(double t, const std::vector<double>& state) {
    // For custom equations, users would define their own implementation
    // This is a placeholder that returns zero derivatives
    return std::vector<double>(state.size(), 0.0);
}

void AnalogDifferentialEquation::SolveRK4() {
    // Runge-Kutta 4th order solver
    size_t n = params.state_vars.size();
    std::vector<double> k1(n), k2(n), k3(n), k4(n);
    std::vector<double> temp_state(n);
    
    // k1 = h * f(t, y)
    std::vector<double> derivatives = ComputeDerivatives(params.simulation_time, params.state_vars);
    for (size_t i = 0; i < n; i++) {
        k1[i] = params.time_step * derivatives[i];
    }
    
    // k2 = h * f(t + h/2, y + k1/2)
    for (size_t i = 0; i < n; i++) {
        temp_state[i] = params.state_vars[i] + k1[i] / 2.0;
    }
    derivatives = ComputeDerivatives(params.simulation_time + params.time_step / 2.0, temp_state);
    for (size_t i = 0; i < n; i++) {
        k2[i] = params.time_step * derivatives[i];
    }
    
    // k3 = h * f(t + h/2, y + k2/2)
    for (size_t i = 0; i < n; i++) {
        temp_state[i] = params.state_vars[i] + k2[i] / 2.0;
    }
    derivatives = ComputeDerivatives(params.simulation_time + params.time_step / 2.0, temp_state);
    for (size_t i = 0; i < n; i++) {
        k3[i] = params.time_step * derivatives[i];
    }
    
    // k4 = h * f(t + h, y + k3)
    for (size_t i = 0; i < n; i++) {
        temp_state[i] = params.state_vars[i] + k3[i];
    }
    derivatives = ComputeDerivatives(params.simulation_time + params.time_step, temp_state);
    for (size_t i = 0; i < n; i++) {
        k4[i] = params.time_step * derivatives[i];
    }
    
    // Update state: y_new = y + (k1 + 2*k2 + 2*k3 + k4) / 6
    for (size_t i = 0; i < n; i++) {
        params.state_vars[i] += (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]) / 6.0;
    }
}

void AnalogDifferentialEquation::SolveEuler() {
    // Simple Euler solver: y_new = y + h * f(t, y)
    std::vector<double> derivatives = ComputeDerivatives(params.simulation_time, params.state_vars);
    
    for (size_t i = 0; i < params.state_vars.size(); i++) {
        params.state_vars[i] += params.time_step * derivatives[i];
    }
}