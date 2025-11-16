#include "VirtualAnalogModel.h"
#include <cmath>
#include <algorithm>

VirtualAnalogModel::VirtualAnalogModel(VirtualAnalogType type, const AnalogModelParams& params)
    : type(type)
    , params(params)
    , input_signal(0.0)
    , control_voltage(0.0)
    , output(0.0)
    , diff_eq_solver(DiffEqType::CUSTOM)
{
    // Initialize model based on type
    switch (type) {
        case VirtualAnalogType::MOOG_LADDER_FILTER:
            // Initialize for Moog ladder filter
            state_variables.resize(4, 0.0);  // 4 stages of the ladder
            // Default parameters: cutoff freq, resonance, temperature effects
            if (this->params.circuit_params.size() < 3) {
                this->params.circuit_params.resize(3, 1.0);
                this->params.circuit_params[0] = 1000.0;  // Default cutoff: 1kHz
                this->params.circuit_params[1] = 0.5;     // Default resonance
                this->params.circuit_params[2] = 1.0;     // Temperature coefficient
            }
            break;
            
        case VirtualAnalogType::DIODE_LADDER_FILTER:
            // Initialize for diode ladder filter
            state_variables.resize(4, 0.0);  // 4 stages
            if (this->params.circuit_params.size() < 3) {
                this->params.circuit_params.resize(3, 1.0);
                this->params.circuit_params[0] = 1000.0;  // Default cutoff: 1kHz
                this->params.circuit_params[1] = 0.5;     // Default resonance
                this->params.circuit_params[2] = 1.0;     // Non-linear coefficient
            }
            break;
            
        case VirtualAnalogType::STATE_VARIABLE_FILTER:
            // Initialize for state variable filter
            state_variables.resize(2, 0.0);  // 2 state variables (bandpass and highpass)
            if (this->params.circuit_params.size() < 3) {
                this->params.circuit_params.resize(3, 1.0);
                this->params.circuit_params[0] = 1000.0;  // Default cutoff: 1kHz
                this->params.circuit_params[1] = 0.5;     // Default Q
                this->params.circuit_params[2] = 1.0;     // Damping
            }
            break;
            
        case VirtualAnalogType::TRANSISTOR_OSCILLATOR:
            // Initialize for transistor oscillator
            state_variables.resize(2, 0.0);  // Frequency and phase
            if (this->params.circuit_params.size() < 2) {
                this->params.circuit_params.resize(2, 1.0);
                this->params.circuit_params[0] = 440.0;   // Default frequency: A440
                this->params.circuit_params[1] = 1.0;     // Waveform shape
            }
            break;
            
        case VirtualAnalogType::OPERATIONAL_AMPLIFIER:
            // Initialize for op-amp model
            state_variables.resize(1, 0.0);  // Output state
            if (this->params.circuit_params.size() < 3) {
                this->params.circuit_params.resize(3, 1.0);
                this->params.circuit_params[0] = 10.0;    // Default gain
                this->params.circuit_params[1] = 1e6;     // Open-loop gain
                this->params.circuit_params[2] = 1.0;     // Slew rate
            }
            break;
            
        case VirtualAnalogType::VINTAGE_DELAY:
            // Initialize for vintage delay (BBD model)
            state_variables.resize(1024, 0.0);  // Buffer for delay line, 1024 samples max
            if (this->params.circuit_params.size() < 3) {
                this->params.circuit_params.resize(3, 1.0);
                this->params.circuit_params[0] = 0.5;     // Default delay time (fraction of max)
                this->params.circuit_params[1] = 0.3;     // Default feedback
                this->params.circuit_params[2] = 0.8;     // Default wet/dry mix
            }
            break;
            
        case VirtualAnalogType::CUSTOM_ANALOG_MODEL:
        default:
            state_variables.resize(1, 0.0);
            break;
    }
}

bool VirtualAnalogModel::Tick() {
    switch (type) {
        case VirtualAnalogType::MOOG_LADDER_FILTER:
            ProcessMoogLadderFilter();
            break;
            
        case VirtualAnalogType::DIODE_LADDER_FILTER:
            ProcessDiodeLadderFilter();
            break;
            
        case VirtualAnalogType::STATE_VARIABLE_FILTER:
            ProcessStateVariableFilter();
            break;
            
        case VirtualAnalogType::TRANSISTOR_OSCILLATOR:
            ProcessTransistorOscillator();
            break;
            
        case VirtualAnalogType::OPERATIONAL_AMPLIFIER:
            ProcessOperationalAmplifier();
            break;
            
        case VirtualAnalogType::VINTAGE_DELAY:
            ProcessVintageDelay();
            break;
            
        case VirtualAnalogType::CUSTOM_ANALOG_MODEL:
            ProcessCustomAnalogModel();
            break;
    }
    
    return true;
}

void VirtualAnalogModel::SetType(VirtualAnalogType type) {
    this->type = type;
    
    // Reinitialize based on new type
    switch (type) {
        case VirtualAnalogType::MOOG_LADDER_FILTER:
            state_variables.resize(4, 0.0);
            if (params.circuit_params.size() < 3) {
                params.circuit_params.resize(3, 1.0);
                params.circuit_params[0] = 1000.0;
                params.circuit_params[1] = 0.5;
                params.circuit_params[2] = 1.0;
            }
            break;
            
        case VirtualAnalogType::DIODE_LADDER_FILTER:
            state_variables.resize(4, 0.0);
            if (params.circuit_params.size() < 3) {
                params.circuit_params.resize(3, 1.0);
                params.circuit_params[0] = 1000.0;
                params.circuit_params[1] = 0.5;
                params.circuit_params[2] = 1.0;
            }
            break;
            
        case VirtualAnalogType::STATE_VARIABLE_FILTER:
            state_variables.resize(2, 0.0);
            if (params.circuit_params.size() < 3) {
                params.circuit_params.resize(3, 1.0);
                params.circuit_params[0] = 1000.0;
                params.circuit_params[1] = 0.5;
                params.circuit_params[2] = 1.0;
            }
            break;
            
        case VirtualAnalogType::TRANSISTOR_OSCILLATOR:
            state_variables.resize(2, 0.0);
            if (params.circuit_params.size() < 2) {
                params.circuit_params.resize(2, 1.0);
                params.circuit_params[0] = 440.0;
                params.circuit_params[1] = 1.0;
            }
            break;
            
        case VirtualAnalogType::OPERATIONAL_AMPLIFIER:
            state_variables.resize(1, 0.0);
            if (params.circuit_params.size() < 3) {
                params.circuit_params.resize(3, 1.0);
                params.circuit_params[0] = 10.0;
                params.circuit_params[1] = 1e6;
                params.circuit_params[2] = 1.0;
            }
            break;
            
        case VirtualAnalogType::VINTAGE_DELAY:
            state_variables.resize(1024, 0.0);
            if (params.circuit_params.size() < 3) {
                params.circuit_params.resize(3, 1.0);
                params.circuit_params[0] = 0.5;
                params.circuit_params[1] = 0.3;
                params.circuit_params[2] = 0.8;
            }
            break;
            
        case VirtualAnalogType::CUSTOM_ANALOG_MODEL:
        default:
            state_variables.resize(1, 0.0);
            break;
    }
}

void VirtualAnalogModel::SetInput(double input) {
    this->input_signal = input;
}

void VirtualAnalogModel::SetControlVoltage(double cv) {
    this->control_voltage = cv;
}

void VirtualAnalogModel::SetParams(const AnalogModelParams& params) {
    this->params = params;
    
    // Resize state if needed based on circuit params
    switch (type) {
        case VirtualAnalogType::MOOG_LADDER_FILTER:
        case VirtualAnalogType::DIODE_LADDER_FILTER:
            state_variables.resize(4, 0.0);
            break;
        case VirtualAnalogType::STATE_VARIABLE_FILTER:
            state_variables.resize(2, 0.0);
            break;
        case VirtualAnalogType::TRANSISTOR_OSCILLATOR:
            state_variables.resize(2, 0.0);
            break;
        case VirtualAnalogType::OPERATIONAL_AMPLIFIER:
            state_variables.resize(1, 0.0);
            break;
        case VirtualAnalogType::VINTAGE_DELAY:
            state_variables.resize(1024, 0.0);
            break;
        case VirtualAnalogType::CUSTOM_ANALOG_MODEL:
        default:
            state_variables.resize(1, 0.0);
            break;
    }
}

// Parameter getters and setters
void VirtualAnalogModel::SetCutoffFrequency(double freq) {
    if (!params.circuit_params.empty()) {
        params.circuit_params[0] = std::max(20.0, std::min(20000.0, freq));  // Limit to audible range
    }
}

double VirtualAnalogModel::GetCutoffFrequency() const {
    return params.circuit_params.size() > 0 ? params.circuit_params[0] : 1000.0;
}

void VirtualAnalogModel::SetResonance(double res) {
    if (params.circuit_params.size() > 1) {
        params.circuit_params[1] = std::max(0.1, std::min(10.0, res));  // Reasonable resonance range
    }
}

double VirtualAnalogModel::GetResonance() const {
    return params.circuit_params.size() > 1 ? params.circuit_params[1] : 0.5;
}

void VirtualAnalogModel::SetOscillatorFrequency(double freq) {
    if (!params.circuit_params.empty()) {
        params.circuit_params[0] = std::max(0.1, std::min(20000.0, freq));  // Limit to reasonable range
    }
}

double VirtualAnalogModel::GetOscillatorFrequency() const {
    return params.circuit_params.size() > 0 ? params.circuit_params[0] : 440.0;
}

// Model processing implementations
void VirtualAnalogModel::ProcessMoogLadderFilter() {
    // Classic Moog ladder filter implementation using approximated transistor behavior
    double cutoff = params.circuit_params[0];
    double resonance = params.circuit_params[1];
    
    // Adjust cutoff by control voltage
    cutoff *= (1.0 + control_voltage * 0.1);  // 10% change per volt of CV
    cutoff = std::max(20.0, std::min(20000.0, cutoff));  // Clamp to audio range
    
    // Calculate filter coefficient
    double f = 2.0 * sin(M_PI * cutoff / params.sample_rate);
    
    // Apply resonance with feedback (clamping to prevent instability)
    resonance = std::min(resonance, 4.0); // Prevent oscillation
    
    // The classic Moog ladder filter algorithm
    // Apply non-linear saturation to model transistor soft clipping
    double input = TanhSaturation(input_signal);
    
    // Four one-pole filters in series with feedback
    state_variables[0] = state_variables[0] + f * (input - state_variables[0] - resonance * (state_variables[3] - state_variables[2]));
    state_variables[1] = state_variables[1] + f * (state_variables[0] - state_variables[1]);
    state_variables[2] = state_variables[2] + f * (state_variables[1] - state_variables[2]);
    state_variables[3] = state_variables[3] + f * (state_variables[2] - state_variables[3]);
    
    // Apply saturation to final stage to model amplifier saturation
    output = TanhSaturation(state_variables[3]);
}

void VirtualAnalogModel::ProcessDiodeLadderFilter() {
    // Diode ladder filter implementation
    double cutoff = params.circuit_params[0];
    double resonance = params.circuit_params[1];
    
    // Adjust cutoff by control voltage
    cutoff *= (1.0 + control_voltage * 0.1);
    cutoff = std::max(20.0, std::min(20000.0, cutoff));
    
    // Calculate frequency factor
    double f = 2.0 * sin(M_PI * cutoff / params.sample_rate);
    
    // Apply resonance with feedback
    resonance = std::min(resonance, 4.0);
    
    // Diode ladder implementation
    // Using a simplified model of the diode ladder filter
    double input = input_signal;
    
    // Apply feedback
    input -= resonance * state_variables[3];
    
    // Four stages of filtering with non-linear elements
    for (int i = 0; i < 4; i++) {
        // Apply diode non-linearity
        input = DiodeResponse(input);
        
        // One-pole filter stage
        state_variables[i] = state_variables[i] + f * (input - state_variables[i]);
        
        // Output of this stage becomes input to next stage
        input = state_variables[i];
    }
    
    output = state_variables[3];
}

void VirtualAnalogModel::ProcessStateVariableFilter() {
    // State Variable Filter implementation
    double cutoff = params.circuit_params[0];
    double Q = params.circuit_params[1];
    
    // Adjust cutoff by control voltage
    cutoff *= (1.0 + control_voltage * 0.1);
    cutoff = std::max(20.0, std::min(20000.0, cutoff));
    
    // Calculate filter coefficients
    double g = tan(M_PI * cutoff / params.sample_rate);
    double k = 1.0 / Q;
    
    // State variable filter algorithm
    // Update the integrator states
    double HP = input_signal - k * state_variables[0] - state_variables[1];  // Highpass
    double BP = state_variables[0] + g * HP;  // Bandpass
    double LP = state_variables[1] + g * BP;  // Lowpass
    
    // Update states
    state_variables[0] = BP;  // Bandpass stored in first state
    state_variables[1] = LP;  // Lowpass stored in second state
    
    // Output based on filter type (for now always lowpass)
    output = LP;
}

void VirtualAnalogModel::ProcessTransistorOscillator() {
    // Classic analog oscillator with transistor non-linearities
    double freq = params.circuit_params[0];
    double waveform = params.circuit_params[1];
    
    // Adjust frequency by control voltage
    freq *= (1.0 + control_voltage * 0.1);
    freq = std::max(0.1, std::min(20000.0, freq));
    
    // Calculate phase increment
    double phase_inc = 2.0 * M_PI * freq / params.sample_rate;
    
    // Update phase
    state_variables[0] += phase_inc;
    if (state_variables[0] > 2.0 * M_PI) {
        state_variables[0] -= 2.0 * M_PI;
    }
    
    // Generate waveform with transistor-like non-linearities
    double phase = state_variables[0];
    double sample = 0.0;
    
    switch (static_cast<int>(waveform)) {
        case 0: // Sine (with slight harmonic distortion)
            sample = sin(phase);
            // Add small amount of harmonic distortion to simulate analog non-linearity
            sample += 0.1 * sin(3.0 * phase) + 0.05 * sin(5.0 * phase);
            break;
        case 1: // Sawtooth with saturation
            sample = (2.0 * phase / (2.0 * M_PI) - 1.0);
            // Apply soft saturation to simulate analog clipping
            sample = TanhSaturation(sample);
            break;
        case 2: // Square with soft edges (realistic analog square)
            sample = sin(phase) >= 0 ? 1.0 : -1.0;
            // Apply some rounding to simulate analog switching behavior
            sample = CubicSaturation(sample * 2.0) / 2.0;
            break;
        case 3: // Triangle (more complex harmonic content)
            if (phase < M_PI) {
                sample = (2.0 * phase / M_PI - 1.0);
            } else {
                sample = (1.0 - 2.0 * (phase - M_PI) / M_PI);
            }
            // Add slight harmonic content
            sample += 0.05 * sin(3.0 * phase) + 0.02 * sin(5.0 * phase);
            break;
        default: // Default to a sawtooth with saturation
            sample = (2.0 * phase / (2.0 * M_PI) - 1.0);
            sample = TanhSaturation(sample);
            break;
    }
    
    // Apply final saturation
    output = TanhSaturation(sample);
}

void VirtualAnalogModel::ProcessOperationalAmplifier() {
    // Op-amp model with non-linear characteristics
    double gain = params.circuit_params[0];
    double open_loop_gain = params.circuit_params[1];
    double slew_rate = params.circuit_params[2];
    
    // Calculate the desired output based on input and gain
    double ideal_output = input_signal * gain;
    
    // Apply op-amp limitations: saturation and slew rate
    // First, apply output saturation
    double saturation_level = 10.0;  // Typical op-amp output range
    if (ideal_output > saturation_level) {
        ideal_output = saturation_level;
    } else if (ideal_output < -saturation_level) {
        ideal_output = -saturation_level;
    }
    
    // Calculate maximum change allowed by slew rate
    double max_change = slew_rate * (1.0 / params.sample_rate);
    
    // Apply slew rate limiting
    if (ideal_output > (state_variables[0] + max_change)) {
        state_variables[0] += max_change;
    } else if (ideal_output < (state_variables[0] - max_change)) {
        state_variables[0] -= max_change;
    } else {
        state_variables[0] = ideal_output;
    }
    
    // Apply soft saturation to model real op-amp behavior
    output = TanhSaturation(state_variables[0] / saturation_level) * saturation_level;
}

void VirtualAnalogModel::ProcessVintageDelay() {
    // Vintage BBD (Bucket Brigade Device) delay model
    double delay_time = params.circuit_params[0]; // Fraction of max buffer
    double feedback = params.circuit_params[1];   // Feedback amount
    double mix = params.circuit_params[2];        // Wet/dry mix
    
    // Calculate delay in samples
    int max_delay = static_cast<int>(state_variables.size());
    int delay_samples = static_cast<int>(delay_time * max_delay * 0.5); // 0.5 to keep reasonable max
    delay_samples = std::max(1, std::min(delay_samples, max_delay - 1));
    
    // Calculate write and read positions
    int write_pos = static_cast<int>(state_variables[0]) + 1;
    if (write_pos >= max_delay) write_pos = 0;
    
    // Update the delay buffer
    state_variables[write_pos] = input_signal + feedback * state_variables[(write_pos + max_delay - delay_samples) % max_delay];
    
    // Calculate the delayed output
    int read_pos = (write_pos + max_delay - delay_samples) % max_delay;
    double delayed = state_variables[read_pos];
    
    // Update the write position in the state
    state_variables[0] = write_pos;
    
    // Mix with original signal
    output = mix * delayed + (1.0 - mix) * input_signal;
    
    // Apply characteristic BBD noise and frequency response
    // BBDs have a characteristic high-frequency roll-off
    output *= 0.99; // Very slight attenuation per sample
}

void VirtualAnalogModel::ProcessCustomAnalogModel() {
    // Placeholder for custom analog models
    output = input_signal;
}

// Helper functions for modeling analog behavior
double VirtualAnalogModel::TransistorResponse(double base_voltage, double collector_voltage) {
    // Simplified Ebers-Moll model for bipolar junction transistor
    // This is a very simplified version for audio modeling purposes
    double vt = 0.026;  // Thermal voltage at room temperature
    double ic = 0.001 * (exp(base_voltage / vt) - 1);  // Collector current approximation
    
    // Collector voltage affects current (Early effect)
    ic *= (1.0 + collector_voltage / 50.0);  // Rough Early effect model
    
    return ic;
}

double VirtualAnalogModel::DiodeResponse(double voltage) {
    // Simplified diode model
    double vt = 0.026;  // Thermal voltage
    double is = 1e-12;  // Saturation current
    
    if (voltage > 0.1) {
        // Forward bias
        return is * (exp(voltage / vt) - 1);
    } else if (voltage < -1.0) {
        // Reverse bias (leakage)
        return -is * 0.1;  // Small leakage
    } else {
        // Near zero - polynomial approximation
        return voltage * 0.5;
    }
}

double VirtualAnalogModel::OpAmpResponse(double input, double feedback) {
    // Simplified op-amp model
    double open_loop_gain = 100000.0;  // Typical open-loop gain
    
    // Calculate output with feedback
    double output = input * open_loop_gain / (1.0 + open_loop_gain * feedback);
    
    // Apply output saturation
    if (output > 10.0) output = 10.0;
    else if (output < -10.0) output = -10.0;
    
    return output;
}

// Non-linear functions modeling analog behavior
double VirtualAnalogModel::TanhSaturation(double input, double saturation_level) {
    // Apply hyperbolic tangent saturation to model soft clipping
    return tanh(input * saturation_level) / saturation_level;
}

double VirtualAnalogModel::CubicSaturation(double input) {
    // Apply cubic non-linearity
    double x = input;
    return x - (x * x * x) / 3.0;  // Cubic shaping curve
}

double VirtualAnalogModel::ExponentialResponse(double input) {
    // Exponential response for modeling transistor behavior
    if (input > 0) {
        return exp(input) - 1;
    } else {
        return -exp(-input) + 1;
    }
}