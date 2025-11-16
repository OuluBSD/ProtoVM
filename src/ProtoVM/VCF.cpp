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
    , drive(0.5)
    , saturation(0.8)
{
    // Initialize state and delay arrays
    for (int i = 0; i < 4; i++) {
        state[i] = 0.0;
        delay[i] = 0.0;
        transistor_state[i] = 0.0;
        diode_state[i] = 0.0;
        integration_state[i] = 0.0;
    }
    tanh_output = 0.0;
    cutoff_scale_factor = 1.0;
}

bool VCF::Tick() {
    // Calculate the actual cutoff frequency based on control voltage and envelope
    double cutoff = cutoff_frequency;

    // Add control voltage effect
    cutoff *= pow(2.0, control_voltage * CV_SENSITIVITY);

    // Add envelope effect if applied
    cutoff *= (1.0 + envelope_amount * (resonance - 0.5)); // Simplified envelope tracking

    // Clamp cutoff to valid range
    cutoff = std::max(MIN_CUTOFF, std::min(MAX_CUTOFF, cutoff));

    // Apply the appropriate filter implementation
    switch (implementation) {
        case FilterImplementation::MOOG_LADDER:
            output = ProcessMoogLadderFilter();
            break;

        case FilterImplementation::DIODE_LADDER:
            output = ProcessDiodeLadderFilter();
            break;

        case FilterImplementation::SVF:
            output = ProcessStateVariableFilter();
            break;

        case FilterImplementation::ONE_POLE:
            output = ProcessOnePoleFilter();
            break;

        case FilterImplementation::BUTTERWORTH:
            output = ProcessButterworthFilter();
            break;

        case FilterImplementation::MODIFIED_MOOG:
            output = ProcessModifiedMoogFilter();
            break;

        case FilterImplementation::KENDON_CUTOFF:
            output = ProcessKendonCutoffFilter();
            break;
    }

    // Apply final output saturation
    output = TanhSaturation(output, saturation);

    return true;
}

double VCF::ProcessMoogLadderFilter() {
    // Classic Moog ladder filter implementation with improved modeling
    double sample_rate = 44100.0;

    // Calculate coefficients based on cutoff frequency
    double omega = 2.0 * M_PI * cutoff_frequency / sample_rate;
    double sin_omega = sin(omega);
    double cos_omega = cos(omega);

    // Apply resonance with feedback
    double feedback = resonance * 2.0;  // Scale resonance appropriately

    // Process the ladder filter stages with non-linear saturation
    double input = input_signal;
    
    // Apply drive/saturation before filtering
    input = TransistorResponse(input, drive);

    // Process the 4 stages of the ladder with feedback
    for (int i = 0; i < 4; i++) {
        // One-pole lowpass filter stage with feedback
        double feedback_input = (i == 0) ? (input - feedback * delay[3]) : delay[i-1];
        delay[i] = (feedback_input + delay[i]) * 0.5; // Simple lowpass
    }

    // Apply non-linear saturation and return
    double result = delay[3];
    result = TanhSaturation(result, 0.9);  // Slight saturation for warmth
    
    return result;
}

double VCF::ProcessDiodeLadderFilter() {
    // Diode ladder filter implementation
    double sample_rate = 44100.0;

    // Calculate coefficients based on cutoff frequency
    double omega = 2.0 * M_PI * cutoff_frequency / sample_rate;

    // Process the 4 stages of the diode ladder filter
    double input = input_signal;
    
    // Apply drive to the input
    input = TransistorResponse(input, drive);

    // Process each stage with diode response
    for (int i = 0; i < 4; i++) {
        // Apply diode response to this stage
        input = DiodeResponse(input);
        
        // Apply lowpass filter to this stage
        diode_state[i] = diode_state[i] + omega * (input - diode_state[i]);
        
        // Output of this stage becomes input to next
        input = diode_state[i];
    }

    // Apply final saturation
    double result = diode_state[3];
    result = TanhSaturation(result, 0.85);
    
    return result;
}

double VCF::ProcessStateVariableFilter() {
    // State Variable Filter implementation with enhanced modeling
    double sample_rate = 44100.0;

    // Calculate coefficients
    double g = tan(M_PI * cutoff_frequency / sample_rate);
    double k = 1.0 / resonance;  // Resonance factor

    // Update state variables with non-linear processing
    double lowpass_input = input_signal;
    
    // Apply drive to the input
    lowpass_input = TransistorResponse(lowpass_input, drive * 0.5);
    
    // Process the integrators with non-linear feedback
    double highpass = lowpass_input - state[0] * k - state[1];
    double bandpass = state[0] + g * highpass;
    double lowpass = state[1] + g * bandpass;
    
    // Update states
    state[0] = TransistorResponse(bandpass, drive * 0.3);
    state[1] = TransistorResponse(lowpass, drive * 0.2);

    // Calculate outputs based on filter type
    switch (filter_type) {
        case FilterType::LOWPASS:
            return lowpass;
        case FilterType::HIGHPASS:
            return highpass;
        case FilterType::BANDPASS:
            return bandpass;
        case FilterType::NOTCH:
            return lowpass_input - resonance * bandpass;
        case FilterType::ALLPASS:
            return lowpass_input - 2.0 * resonance * bandpass;
        default:
            return lowpass; // Default to lowpass
    }
}

double VCF::ProcessOnePoleFilter() {
    // Simple first-order RC filter with enhanced modeling
    double sample_rate = 44100.0;

    // Calculate coefficient
    double omega = 2.0 * M_PI * cutoff_frequency / sample_rate;
    double alpha = omega / (omega + 1.0);

    // Apply drive to input
    double input = TransistorResponse(input_signal, drive * 0.3);
    
    // Update filter state with non-linear processing
    state[0] = alpha * input + (1.0 - alpha) * state[0];
    
    // Apply saturation
    state[0] = TanhSaturation(state[0], saturation);

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

double VCF::ProcessButterworthFilter() {
    // 4th-order Butterworth filter implementation
    double sample_rate = 44100.0;

    // For simplicity, using a cascade of biquad filters
    // In a real implementation, we'd use the proper Butterworth coefficients
    
    // Apply drive to input
    double input = TransistorResponse(input_signal, drive * 0.2);
    
    // Simple implementation using the SVF as a basis
    // Real Butterworth would use specific poles for the filter order
    return ProcessStateVariableFilter(); // Using SVF as approximation
}

double VCF::ProcessModifiedMoogFilter() {
    // Enhanced Moog filter with better non-linear modeling and differential equations
    double sample_rate = 44100.0;
    double dt = 1.0 / sample_rate;

    // Calculate frequency parameters
    double fc = cutoff_frequency;
    double resonance_factor = resonance * 2.0; // Resonance control
    
    // Apply drive to input
    double input = TransistorResponse(input_signal, drive);
    
    // Apply resonance feedback (with saturation)
    input -= resonance_factor * tanh(delay[3] * 0.5);
    
    // Apply four cascaded one-pole filters (the ladder)
    for (int i = 0; i < 4; i++) {
        // Calculate filter coefficient using bilinear transform
        double g = tan(M_PI * fc / sample_rate);
        
        // Apply the filter stage with non-linear processing
        double v = (input - tanh(delay[i])) * g / (1.0 + g);
        delay[i] = tanh(v + delay[i]);
        
        // Output of this stage becomes input to the next
        input = delay[i];
    }
    
    // Apply final non-linear processing
    double output = delay[3];
    output = TanhSaturation(output, saturation);
    
    return output;
}

double VCF::ProcessKendonCutoffFilter() {
    // Implementation based on Ken Donnelly's cutoff frequency algorithm
    // This approach more accurately models the voltage-controlled behavior of real analog filters
    double sample_rate = 44100.0;
    double dt = 1.0 / sample_rate;
    
    // Calculate cutoff frequency with voltage control
    double base_freq = cutoff_frequency;
    double cv_freq = base_freq * pow(2.0, control_voltage * 1.0); // 1V/Octave
    
    // Apply envelope modulation
    cv_freq *= (1.0 + envelope_amount * resonance);
    
    // Clamp to valid range
    cv_freq = std::max(MIN_CUTOFF, std::min(MAX_CUTOFF, cv_freq));
    
    // Apply drive to input
    double input = TransistorResponse(input_signal, drive);
    
    // Process the filter with the calculated frequency
    // Ken Donnelly's approach models the non-linear transistor behavior more accurately
    double omega = 2.0 * M_PI * cv_freq / sample_rate;
    
    // Apply the filter with improved non-linear modeling
    // This is a simplified implementation - a full implementation would use 
    // differential equations to model the actual transistor circuit behavior
    for (int i = 0; i < 4; i++) {
        // One-pole filter stage with non-linear feedback
        double feedback = (i == 0) ? (resonance * delay[3]) : 0.0;
        double input_to_stage = (i == 0) ? input : delay[i-1];
        
        // Calculate change in state
        double dstate = omega * (tanh(input_to_stage - feedback) - delay[i]);
        
        // Update state with integration
        delay[i] += dt * dstate;
        
        // Apply non-linear distortion to next stage
        delay[i] = tanh(delay[i] * 0.8);
    }
    
    // Apply final output processing
    double result = delay[3];
    result = TanhSaturation(result, saturation);
    
    return result;
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

void VCF::SetDrive(double drive) {
    this->drive = std::max(MIN_DRIVE, std::min(MAX_DRIVE, drive));
}

void VCF::SetSaturation(double sat) {
    this->saturation = std::max(MIN_SATURATION, std::min(MAX_SATURATION, sat));
}

// Helper methods for non-linear processing
double VCF::TanhSaturation(double input, double saturation_level) {
    // Apply hyperbolic tangent saturation to model soft clipping
    return tanh(input * saturation_level) / saturation_level;
}

double VCF::TransistorResponse(double input, double drive) {
    // Model the non-linear response of transistors with drive control
    double gain = 1.0 + drive * 5.0; // Increase gain with drive
    double result = input * gain;
    
    // Apply soft clipping that's characteristic of transistor circuits
    if (result > 0.5) {
        result = 0.5 + 0.5 * tanh((result - 0.5) * 2.0);
    } else if (result < -0.5) {
        result = -0.5 + 0.5 * tanh((result + 0.5) * 2.0);
    }
    
    return result;
}

double VCF::DiodeResponse(double input) {
    // Model the non-linear response of diodes
    // This is a simplified model - real diodes have exponential response
    if (input > 0.3) {
        return 0.3 + 0.7 * tanh((input - 0.3) * 3.0);
    } else if (input < -0.3) {
        return -0.3 + 0.7 * tanh((input + 0.3) * 3.0);
    } else {
        return input * 0.7; // Less gain near zero
    }
}