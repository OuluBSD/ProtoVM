#include "VCF.h"
#include <cmath>
#include <algorithm>

VCF::VCF(FilterType type, FilterImplementation implementation, double cutoff_freq, double resonance)
    : filter_type(type)
    , implementation(implementation)
    , cutoff_frequency(cutoff_freq)
    , resonance(resonance)
    , control_voltage(0.0)
    , input_signal(0.0)
    , output(0.0)
    , envelope_amount(0.0)
{
    // Initialize state and delay arrays
    for (int i = 0; i < 4; i++) {
        state[i] = 0.0;
        delay[i] = 0.0;
    }
    tanh_output = 0.0;
}

bool VCF::Tick() {
    // Calculate the actual cutoff frequency based on control voltage and envelope
    double cutoff = cutoff_frequency;
    
    // Add control voltage effect
    cutoff *= pow(2.0, control_voltage * CV_SENSITIVITY);
    
    // Add envelope effect if applied
    cutoff *= (1.0 + envelope_amount * (1.0 - resonance)); // Simplified envelope tracking
    
    // Clamp cutoff to valid range
    cutoff = std::max(MIN_CUTOFF, std::min(MAX_CUTOFF, cutoff));
    
    // Apply the appropriate filter implementation
    switch (implementation) {
        case FilterImplementation::MOOG_LADDER:
            output = ProcessMoogLadderFilter(input_signal, cutoff, resonance);
            break;
            
        case FilterImplementation::SVF:
            output = ProcessStateVariableFilter(input_signal, cutoff, resonance);
            break;
            
        case FilterImplementation::ONE_POLE:
            output = ProcessOnePoleFilter(input_signal, cutoff);
            break;
            
        case FilterImplementation::BUTTERWORTH:
            output = ProcessButterworthFilter(input_signal, cutoff, resonance);
            break;
    }
    
    return true;
}

double VCF::ProcessMoogLadderFilter(double input, double cutoff, double res) {
    // Simplified Moog ladder filter implementation
    // This is a basic approximation - real implementation would be more complex
    double sample_rate = 44100.0;
    
    // Convert cutoff to radians
    double omega = 2.0 * M_PI * cutoff / sample_rate;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);
    
    // Calculate filter coefficient
    double alpha = sin_omega / 2.0;
    double a0 = 1.0 + alpha;
    
    // Apply resonance (feedback)
    double feedback = res * 2.0;  // Scale resonance appropriately
    
    // Process the ladder filter stages
    double v1 = (input - feedback * delay[3]) / a0;
    double v2 = (v1 + alpha * delay[0]) / a0;
    double v3 = (v2 + alpha * delay[1]) / a0;
    double v4 = (v3 + alpha * delay[2]) / a0;
    
    // Update delay elements
    delay[3] = delay[2];
    delay[2] = delay[1];
    delay[1] = delay[0];
    delay[0] = input;
    
    // Apply non-linear saturation (simplified)
    v4 = tanh(v4) * 0.9;  // Slight saturation for warmth
    
    return v4;
}

double VCF::ProcessStateVariableFilter(double input, double cutoff, double res) {
    // State Variable Filter implementation
    double sample_rate = 44100.0;
    
    // Calculate coefficients
    double g = tan(M_PI * cutoff / sample_rate);
    double k = 1.0 / res;  // Resonance factor
    
    // Update state variables
    state[0] = state[0] + g * (input - state[0] - k * state[1]);
    state[1] = state[1] + g * (state[0] - state[1]);
    
    // Calculate outputs based on filter type
    switch (filter_type) {
        case FilterType::LOWPASS:
            return state[1];
        case FilterType::HIGHPASS:
            return input - state[0] - k * state[1];
        case FilterType::BANDPASS:
            return state[0] - state[1];
        case FilterType::NOTCH:
            return input - state[0];
        case FilterType::ALLPASS:
            return input - 2.0 * k * state[1];
        default:
            return state[1]; // Default to lowpass
    }
}

double VCF::ProcessOnePoleFilter(double input, double cutoff) {
    // Simple first-order RC filter
    double sample_rate = 44100.0;
    
    // Calculate coefficient
    double omega = 2.0 * M_PI * cutoff / sample_rate;
    double alpha = omega / (omega + 1.0);
    
    // Update filter state
    state[0] = alpha * input + (1.0 - alpha) * state[0];
    
    // Return appropriate output based on type
    switch (filter_type) {
        case FilterType::LOWPASS:
            return state[0];
        case FilterType::HIGHPASS:
            return input - state[0];
        default:
            return state[0]; // Default to lowpass
    }
}

double VCF::ProcessButterworthFilter(double input, double cutoff, double res) {
    // Simplified Butterworth implementation
    // For a complete implementation, we'd implement proper nth-order Butterworth
    return ProcessStateVariableFilter(input, cutoff, res * 0.5); // Using SVF as approximation
}

void VCF::SetType(FilterType type) {
    this->filter_type = type;
}

void VCF::SetImplementation(FilterImplementation impl) {
    this->implementation = impl;
}

void VCF::SetCutoffFrequency(double freq) {
    this->cutoff_frequency = std::max(MIN_CUTOFF, std::min(MAX_CUTOFF, freq));
}

void VCF::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}

void VCF::SetControlVoltage(double cv) {
    this->control_voltage = cv;
}

void VCF::SetInput(double input) {
    this->input_signal = input;
}

void VCF::SetEnvelopeAmount(double amount) {
    this->envelope_amount = std::max(-1.0, std::min(1.0, amount));
}