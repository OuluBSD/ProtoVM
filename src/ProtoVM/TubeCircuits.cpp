#include "TubeCircuits.h"
#include <algorithm>
#include <cmath>

// TubeFilter implementation
TubeFilter::TubeFilter(TubeFilterType type) 
    : filter_type(type)
    , input_signal(0.0)
    , output_signal(0.0)
    , cutoff_frequency(1000.0)  // 1kHz default
    , q_factor(0.707)           // Butterworth response
    , filter_gain(1.0)
    , is_enabled(true)
{
    // Initialize with a single triode for basic filtering
    filter_tubes.push_back(std::make_unique<Triode>());
}

TubeFilter::~TubeFilter() {
    // Cleanup handled by smart pointers
}

bool TubeFilter::Tick() {
    if (!is_enabled) {
        output_signal = input_signal;
        return true;
    }
    
    // Process the signal through the filter
    ProcessSignal();
    
    // Tick all tubes used in the filter
    for (auto& tube : filter_tubes) {
        tube->Tick();
    }
    
    return true;
}

double TubeFilter::GetResponseAtFrequency(double freq) const {
    // Calculate filter response at a specific frequency
    // This is a simplified model - real tube filters are more complex
    
    double normalized_freq = freq / cutoff_frequency;
    
    switch (filter_type) {
        case TubeFilterType::LOW_PASS:
            // Magnitude response of a first-order low-pass filter
            return 1.0 / sqrt(1 + (normalized_freq * normalized_freq));
            
        case TubeFilterType::HIGH_PASS:
            // Magnitude response of a first-order high-pass filter
            return normalized_freq / sqrt(1 + (normalized_freq * normalized_freq));
            
        case TubeFilterType::BAND_PASS:
            // Simplified band-pass response
            return (q_factor * normalized_freq) / 
                   sqrt(pow(1 - normalized_freq * normalized_freq, 2) + 
                        pow(q_factor * normalized_freq, 2));
                        
        case TubeFilterType::BAND_STOP:
            // Simplified band-stop response
            double denominator = pow(1 - normalized_freq * normalized_freq, 2) + 
                                pow(q_factor * normalized_freq, 2);
            return sqrt(pow(1 - normalized_freq * normalized_freq, 2)) / 
                   sqrt(denominator);
                   
        case TubeFilterType::ALL_PASS:
            // All-pass maintains magnitude but shifts phase
            return 1.0;
            
        default:
            return 1.0;
    }
}


// TubeLowPassFilter implementation
TubeLowPassFilter::TubeLowPassFilter() 
    : TubeFilter(TubeFilterType::LOW_PASS) {
    // Set up tubes for low-pass configuration
    filter_tubes.clear();
    // Using a common-cathode amplifier stage followed by RC network model
    filter_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // 12AX7
}

void TubeLowPassFilter::ProcessSignal() {
    // Implement a tube-based low-pass filter
    // This simulates a triode amplifier stage with RC load
    
    if (filter_tubes.empty()) {
        output_signal = input_signal;
        return;
    }
    
    // Apply the low-pass filter characteristic
    output_signal = CalculateLowPassResponse(input_signal, cutoff_frequency, q_factor);
    
    // Apply filter gain
    output_signal *= filter_gain;
    
    // Limit output to prevent clipping
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
}

double TubeLowPassFilter::CalculateLowPassResponse(double input, double cutoff, double q) {
    // Simplified implementation of a tube-based low-pass filter
    // In real circuits, this would involve complex interactions between
    // the tube characteristics and reactive components
    
    // For simulation, we'll use a digital approximation of an analog RC filter
    // with characteristics modified by tube circuit properties
    
    static double prev_output = 0.0;
    static bool first_run = true;
    
    if (first_run) {
        prev_output = input;
        first_run = false;
    }
    
    // Calculate the time constant based on cutoff frequency
    double dt = 1.0 / 44100.0;  // Assuming 44.1kHz sample rate
    double rc = 1.0 / (2.0 * M_PI * cutoff);
    
    // Calculate alpha for the first-order filter
    double alpha = dt / (rc + dt);
    
    // Apply the filter with tube-like characteristics
    double result = prev_output + alpha * (input - prev_output);
    
    // Apply resonance/high Q effects
    if (q > 0.707) {
        // Boost frequencies near the cutoff to simulate resonance
        double resonance_factor = 1.0 + (q - 0.707) * 0.5;
        result *= resonance_factor;
    }
    
    // Update previous output for next sample
    prev_output = result;
    
    return result;
}


// TubeHighPassFilter implementation
TubeHighPassFilter::TubeHighPassFilter() 
    : TubeFilter(TubeFilterType::HIGH_PASS) {
    // Set up tubes for high-pass configuration
    filter_tubes.clear();
    filter_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // 12AX7
}

void TubeHighPassFilter::ProcessSignal() {
    // Implement a tube-based high-pass filter
    output_signal = CalculateHighPassResponse(input_signal, cutoff_frequency, q_factor);
    
    // Apply filter gain
    output_signal *= filter_gain;
    
    // Limit output to prevent clipping
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
}

double TubeHighPassFilter::CalculateHighPassResponse(double input, double cutoff, double q) {
    // Simplified implementation of a tube-based high-pass filter
    
    static double prev_input = 0.0, prev_output = 0.0;
    static bool first_run = true;
    
    if (first_run) {
        prev_input = input;
        prev_output = 0.0;
        first_run = false;
    }
    
    // Calculate the time constant based on cutoff frequency
    double dt = 1.0 / 44100.0;  // Assuming 44.1kHz sample rate
    double rc = 1.0 / (2.0 * M_PI * cutoff);
    
    // Calculate alpha for the first-order filter
    double alpha = rc / (rc + dt);
    
    // Apply the high-pass filter with tube-like characteristics
    double result = alpha * prev_output + alpha * (input - prev_input);
    
    // Apply resonance effects
    if (q > 0.707) {
        // For high-pass, resonance affects the transition band
        result *= (1.0 + (q - 0.707) * 0.3);
    }
    
    // Update for next sample
    prev_input = input;
    prev_output = result;
    
    return result;
}


// TubeBandPassFilter implementation
TubeBandPassFilter::TubeBandPassFilter() 
    : TubeFilter(TubeFilterType::BAND_PASS) {
    // Set up tubes for band-pass configuration
    filter_tubes.clear();
    filter_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // 12AX7
    filter_tubes.push_back(std::make_unique<Triode>(100.0, 62000.0, 1.6e-3)); // Additional gain stage
}

void TubeBandPassFilter::ProcessSignal() {
    // Implement a tube-based band-pass filter
    output_signal = CalculateBandPassResponse(input_signal, cutoff_frequency, q_factor);
    
    // Apply filter gain
    output_signal *= filter_gain;
    
    // Limit output to prevent clipping
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
}

double TubeBandPassFilter::CalculateBandPassResponse(double input, double center_freq, double q) {
    // Simplified implementation of a tube-based band-pass filter
    // This simulates a tuned circuit with adjustable Q factor
    
    static double prev_input1 = 0.0, prev_input2 = 0.0;
    static double prev_output1 = 0.0, prev_output2 = 0.0;
    static bool first_run = true;
    
    if (first_run) {
        prev_input1 = input;
        prev_input2 = input;
        prev_output1 = 0.0;
        prev_output2 = 0.0;
        first_run = false;
    }
    
    // Calculate parameters based on center frequency and Q
    double dt = 1.0 / 44100.0;
    double omega = 2.0 * M_PI * center_freq;
    double damp = 1.0 / (2.0 * q);
    
    // Second-order band-pass filter implementation (biquad)
    double a0 = 1.0 + dt * damp * omega + dt * dt * omega * omega;
    double a1 = -2.0 + 2.0 * dt * dt * omega * omega;
    double a2 = 1.0 - dt * damp * omega + dt * dt * omega * omega;
    double b0 = dt * omega;
    double b2 = -dt * omega;
    
    // Apply the filter
    double result = (b0 * input + b2 * prev_input2 - a1 * prev_output1 - a2 * prev_output2) / a0;
    
    // Update for next sample
    prev_input2 = prev_input1;
    prev_input1 = input;
    prev_output2 = prev_output1;
    prev_output1 = result;
    
    return result;
}


// TubeOscillator implementation
TubeOscillator::TubeOscillator(TubeOscillatorType type) 
    : osc_type(type)
    , output_signal(0.0)
    , frequency(440.0)  // A4 note
    , amplitude(1.0)
    , waveform_type(VCOType::SINE)
    , is_enabled(true)
    , feedback(1.0)
    , phase(0.0)
{
    // Initialize with a single triode for basic oscillation
    osc_tubes.push_back(std::make_unique<Triode>());
}

TubeOscillator::~TubeOscillator() {
    // Cleanup handled by smart pointers
}

bool TubeOscillator::Tick() {
    if (!is_enabled) {
        output_signal = 0.0;
        return true;
    }
    
    // Process the signal to generate oscillation
    ProcessSignal();
    
    // Tick all tubes used in the oscillator
    for (auto& tube : osc_tubes) {
        tube->Tick();
    }
    
    return true;
}


// TubeHartleyOscillator implementation
TubeHartleyOscillator::TubeHartleyOscillator() 
    : TubeOscillator(TubeOscillatorType::HARTLEY) {
    // Hartley oscillator uses a triode with a tapped inductor for feedback
    osc_tubes.clear();
    osc_tubes.push_back(std::make_unique<Triode>(100.0, 6200.0, 1.6e-3)); // 12AX7 for oscillation
}

void TubeHartleyOscillator::ProcessSignal() {
    // Hartley oscillator implementation using a triode and LC circuit simulation
    
    // Calculate phase increment based on frequency
    double dt = 1.0 / 44100.0;  // Assuming 44.1kHz sample rate
    double phase_increment = (2.0 * M_PI * frequency) * dt;
    
    // Update phase
    phase += phase_increment;
    if (phase > 2 * M_PI) {
        phase -= 2 * M_PI;
    }
    
    // Generate sine wave
    output_signal = amplitude * sin(phase);
    
    // In a real Hartley oscillator, the feedback would be determined by the
    // inductive voltage divider (the tapped inductor), but in our simulation
    // we'll maintain oscillation by ensuring proper feedback
    
    // Apply amplitude stabilization to prevent the oscillation from growing or decaying
    // This simulates the tube's nonlinear characteristics that naturally stabilize oscillation
    static double avg_amplitude = amplitude;
    double current_amplitude = abs(output_signal);
    
    // Exponential averaging to track the amplitude
    avg_amplitude = 0.999 * avg_amplitude + 0.001 * current_amplitude;
    
    // Adjust the gain to maintain stable oscillation
    if (avg_amplitude > 0) {
        output_signal *= (amplitude / avg_amplitude);
    }
    
    // Ensure the oscillation continues by applying positive feedback
    // In our simulation, this is handled by the amplitude stabilization
}


// TubeColpittsOscillator implementation
TubeColpittsOscillator::TubeColpittsOscillator() 
    : TubeOscillator(TubeOscillatorType::COLPITTS) {
    // Colpitts oscillator uses a triode with capacitive voltage divider for feedback
    osc_tubes.clear();
    osc_tubes.push_back(std::make_unique<Triode>(100.0, 6200.0, 1.6e-3)); // 12AX7 for oscillation
}

void TubeColpittsOscillator::ProcessSignal() {
    // Colpitts oscillator implementation using simulation of LC tank circuit
    
    // Calculate phase increment based on frequency
    double dt = 1.0 / 44100.0;  // Assuming 44.1kHz sample rate
    double phase_increment = (2.0 * M_PI * frequency) * dt;
    
    // Update phase
    phase += phase_increment;
    if (phase > 2 * M_PI) {
        phase -= 2 * M_PI;
    }
    
    // Generate sine wave with Colpitts-specific characteristics
    // In our simulation, we'll generate a sine wave with slight harmonic content
    // that mimics the non-linear characteristics of a real Colpitts oscillator
    output_signal = amplitude * sin(phase);
    
    // Add a small amount of second harmonic to simulate tube non-linearity
    output_signal += 0.05 * amplitude * sin(2 * phase);
    
    // Apply amplitude stabilization
    static double avg_amplitude = amplitude;
    double current_amplitude = abs(output_signal);
    
    // Exponential averaging to track the amplitude
    avg_amplitude = 0.999 * avg_amplitude + 0.001 * current_amplitude;
    
    if (avg_amplitude > 0) {
        output_signal *= (amplitude / avg_amplitude);
    }
}


// TubeWienBridgeOscillator implementation
TubeWienBridgeOscillator::TubeWienBridgeOscillator() 
    : TubeOscillator(TubeOscillatorType::WIEN_BRIDGE) {
    // Wien bridge oscillator uses two triodes - one for oscillation, one for automatic gain control
    osc_tubes.clear();
    osc_tubes.push_back(std::make_unique<Triode>(100.0, 6200.0, 1.6e-3)); // Oscillation stage
    osc_tubes.push_back(std::make_unique<Triode>(100.0, 6200.0, 1.6e-3)); // AGC stage
}

void TubeWienBridgeOscillator::ProcessSignal() {
    // Wien bridge oscillator implementation
    // This oscillator is known for its low distortion and good frequency stability
    
    // Calculate phase increment based on frequency
    double dt = 1.0 / 44100.0;  // Assuming 44.1kHz sample rate
    double phase_increment = (2.0 * M_PI * frequency) * dt;
    
    // Update phase
    phase += phase_increment;
    if (phase > 2 * M_PI) {
        phase -= 2 * M_PI;
    }
    
    // Generate appropriate waveform based on type
    switch (waveform_type) {
        case VCOType::SINE:
            output_signal = amplitude * sin(phase);
            break;
        case VCOType::TRIANGLE:
            // Convert phase to triangle wave
            if (phase < M_PI) {
                output_signal = amplitude * (2.0 * phase / M_PI - 1.0);
            } else {
                output_signal = amplitude * (1.0 - 2.0 * (phase - M_PI) / M_PI);
            }
            break;
        case VCOType::SQUARE:
            // Convert phase to square wave
            output_signal = (phase < M_PI) ? amplitude : -amplitude;
            break;
        default:
            output_signal = amplitude * sin(phase);
            break;
    }
    
    // Wien bridge has inherent amplitude stabilization, but we'll add some
    // simulation of the AGC circuit that would be present in a tube version
    static double avg_amplitude = amplitude;
    double current_amplitude = abs(output_signal);
    
    // Exponential averaging to track the amplitude
    avg_amplitude = 0.995 * avg_amplitude + 0.005 * current_amplitude;
    
    if (avg_amplitude > 0) {
        output_signal *= (amplitude / avg_amplitude);
    }
    
    // In a real Wien bridge, the amplitude is stabilized by a lamp or diode circuit
    // In our tube version, we might use a second triode as a voltage-controlled resistor
}


// TubePhaseShiftOscillator implementation
TubePhaseShiftOscillator::TubePhaseShiftOscillator() 
    : TubeOscillator(TubeOscillatorType::PHASE_SHIFT) {
    // Phase shift oscillator uses multiple triodes (usually 3) with RC networks for feedback
    osc_tubes.clear();
    for (int i = 0; i < 3; i++) {
        osc_tubes.push_back(std::make_unique<Triode>(100.0, 6200.0, 1.6e-3)); // 12AX7
    }
}

void TubePhaseShiftOscillator::ProcessSignal() {
    // Phase shift oscillator implementation
    // Uses RC networks to create 180° phase shift, with the inverting tube adding another 180° = 360° total
    
    // Calculate the frequency based on RC values
    // For a 3-stage phase shift network: f = 1/(2*PI*R*C*sqrt(6))
    // But we'll use the set frequency as the primary control
    double dt = 1.0 / 44100.0;  // Assuming 44.1kHz sample rate
    double phase_increment = (2.0 * M_PI * frequency) * dt;
    
    // Update phase
    phase += phase_increment;
    if (phase > 2 * M_PI) {
        phase -= 2 * M_PI;
    }
    
    // Generate sine wave
    output_signal = amplitude * sin(phase);
    
    // Apply simulation of phase-shift network effects
    // The phase shift oscillator has a tendency to distort, especially when overdriven
    double distortion_factor = 0.05; // Low distortion for this oscillator type
    
    output_signal += distortion_factor * amplitude * sin(3 * phase); // Add some 3rd harmonic
    
    // Apply amplitude stabilization
    static double avg_amplitude = amplitude;
    double current_amplitude = abs(output_signal);
    
    // Exponential averaging to track the amplitude
    avg_amplitude = 0.998 * avg_amplitude + 0.002 * current_amplitude;
    
    if (avg_amplitude > 0) {
        output_signal *= (amplitude / avg_amplitude);
    }
}