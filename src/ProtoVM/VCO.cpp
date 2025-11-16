#include "VCO.h"
#include <cmath>
#include <algorithm>
#include <random>

VCO::VCO(VCOType type, double base_frequency)
    : type(type)
    , base_frequency(base_frequency)
    , control_voltage(0.0)
    , amplitude(1.0)
    , fm_modulation(0.0)
    , pwm_duty_cycle(0.5)
    , phase(0.0)
    , output(0.0)
    , last_input_sample(0.0)
    , sample_rate(44100.0)
    , anti_aliasing_enabled(true)
{
    // Initialize the random number generator for noise
    random_gen.seed(12345); // Fixed seed for predictable results during simulation
    
    // Initialize band-limited lookup tables if needed
    InitializeWaveformTables();
}

bool VCO::Tick() {
    // Calculate the actual frequency based on control voltage and FM modulation
    double frequency = base_frequency;

    // Add control voltage effect (typically 1V/octave or ~69.3 Hz per volt for 1V/oct)
    frequency *= pow(2.0, control_voltage * CV_SENSITIVITY);

    // Add FM modulation
    frequency += fm_modulation * base_frequency;  // FM as a percentage of base frequency

    // Clamp frequency to valid range
    frequency = std::max(MIN_FREQ, std::min(MAX_FREQ, frequency));

    // Calculate phase increment based on frequency and sample rate
    double phase_increment = (TWO_PI * frequency) / sample_rate;

    // Update phase
    phase += phase_increment;
    if (phase > TWO_PI) {
        phase -= TWO_PI;
    }

    // Generate waveform based on type
    switch (type) {
        case VCOType::SINE:
            output = GenerateSineWave();
            break;

        case VCOType::SAWTOOTH:
            output = GenerateSawtoothWave();
            break;

        case VCOType::TRIANGLE:
            output = GenerateTriangleWave();
            break;

        case VCOType::SQUARE:
            output = GenerateSquareWave();
            break;

        case VCOType::PULSE:
            output = GeneratePulseWave();
            break;

        case VCOType::NOISE:
            output = GenerateNoise();
            break;

        case VCOType::S_H:  // Sample and Hold
            output = GenerateSampleAndHold();
            break;

        case VCOType::MORSE_CODE:
            output = GenerateMorseCode();
            break;

        case VCOType::CUSTOM:
            output = GenerateCustomWave();
            break;

        default:
            output = amplitude * sin(phase);
            break;
    }

    // Apply amplitude
    output *= amplitude;

    return true;
}

double VCO::GenerateSineWave() {
    // Standard sine wave with anti-aliasing consideration
    return sin(phase);
}

double VCO::GenerateSawtoothWave() {
    if (anti_aliasing_enabled) {
        // Band-limited sawtooth using Fourier series
        return GenerateBandLimitedSawtooth();
    } else {
        // Simple sawtooth from -1 to 1
        return ((2.0 * phase) / TWO_PI - 1.0);
    }
}

double VCO::GenerateTriangleWave() {
    if (anti_aliasing_enabled) {
        // Band-limited triangle using Fourier series
        return GenerateBandLimitedTriangle();
    } else {
        // Simple triangle wave
        if (phase < M_PI) {
            return (2.0 * phase / M_PI - 1.0);
        } else {
            return (1.0 - 2.0 * (phase - M_PI) / M_PI);
        }
    }
}

double VCO::GenerateSquareWave() {
    if (anti_aliasing_enabled) {
        // Band-limited square using Fourier series
        return GenerateBandLimitedSquare();
    } else {
        // Standard square wave with PWM
        return (phase < TWO_PI * pwm_duty_cycle) ? 1.0 : -1.0;
    }
}

double VCO::GeneratePulseWave() {
    if (anti_aliasing_enabled) {
        // Band-limited pulse using Fourier series
        return GenerateBandLimitedPulse();
    } else {
        // Standard pulse wave (narrow pulse)
        double pulse_width = std::max(0.001, pwm_duty_cycle * 0.1); // 10% width max
        return (phase < TWO_PI * pulse_width) ? 1.0 : -1.0;
    }
}

double VCO::GenerateNoise() {
    // Pseudo-random noise - different from simple generator
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    return dist(random_gen);
}

double VCO::GenerateSampleAndHold() {
    // Sample and hold based on trigger frequency
    static double held_value = 0.0;
    static bool need_new_value = true;
    
    // Change value at trigger frequency (e.g., 10Hz)
    static double trigger_freq = 10.0; // Could be control input
    static double trigger_phase = 0.0;
    static double trigger_phase_inc = (TWO_PI * trigger_freq) / sample_rate;
    
    trigger_phase += trigger_phase_inc;
    if (trigger_phase > TWO_PI && need_new_value) {
        std::uniform_real_distribution<double> dist(-1.0, 1.0);
        held_value = dist(random_gen);
        need_new_value = false;
    } else if (trigger_phase > M_PI && !need_new_value) {
        need_new_value = true; // Ready for next sample
    }
    
    if (trigger_phase > TWO_PI) {
        trigger_phase -= TWO_PI;
    }
    
    return held_value;
}

double VCO::GenerateMorseCode() {
    // Generate a simple morse code pattern for "SOS" (dot-dot-dot dash-dash-dash dot-dot-dot)
    static const char morse_pattern[] = "...---...";  // SOS in morse
    static int pattern_index = 0;
    static double pattern_phase = 0.0;
    static double pattern_phase_inc = (2.0 * M_PI) / (sample_rate * 0.1); // 10Hz base rate
    
    pattern_phase += pattern_phase_inc;
    if (pattern_phase > TWO_PI) {
        pattern_phase = 0.0;
        pattern_index = (pattern_index + 1) % (sizeof(morse_pattern) - 1);
    }
    
    // Determine if we're in a dot, dash, or space
    char current_symbol = morse_pattern[pattern_index];
    if (current_symbol == '.') {  // Dot
        return sin(phase) * 0.8;
    } else if (current_symbol == '-') {  // Dash
        return sin(phase) * 0.8;
    } else {  // Space
        return 0.0;
    }
}

double VCO::GenerateCustomWave() {
    // Custom waveform could be defined by user
    // For now, returning a simple waveform
    return sin(phase) * 0.5 + 0.5 * sin(2 * phase) * 0.3; // Harmonic addition
}

double VCO::GenerateBandLimitedSawtooth() {
    // Use a Fourier series approximation to create a band-limited sawtooth
    double result = 0.0;
    double fundamental_freq = base_frequency * pow(2.0, control_voltage * CV_SENSITIVITY);
    fundamental_freq += fm_modulation * base_frequency;
    fundamental_freq = std::max(MIN_FREQ, std::min(MAX_FREQ, fundamental_freq));
    
    int max_harmonics = static_cast<int>(sample_rate / (2 * fundamental_freq)); // Nyquist limit
    max_harmonics = std::min(max_harmonics, 20); // Limit for performance
    
    for (int n = 1; n <= max_harmonics; n++) {
        double harmonic_phase = n * phase;
        result += (2.0 * pow(-1.0, n + 1) / (n * M_PI)) * sin(harmonic_phase);
    }
    
    return result;
}

double VCO::GenerateBandLimitedTriangle() {
    // Use a Fourier series approximation to create a band-limited triangle
    double result = 0.0;
    double fundamental_freq = base_frequency * pow(2.0, control_voltage * CV_SENSITIVITY);
    fundamental_freq += fm_modulation * base_frequency;
    fundamental_freq = std::max(MIN_FREQ, std::min(MAX_FREQ, fundamental_freq));
    
    int max_harmonics = static_cast<int>(sample_rate / (2 * fundamental_freq));
    max_harmonics = std::min(max_harmonics, 20); // Limit for performance
    
    for (int n = 1; n <= max_harmonics; n++) {
        if ((n - 1) % 2 == 0) { // Only odd harmonics
            int harmonic_number = 2 * n - 1;
            double harmonic_phase = harmonic_number * phase;
            result += (8.0 * pow(-1.0, (n-1)/2) / (M_PI * M_PI * harmonic_number * harmonic_number)) * 
                      sin(harmonic_phase);
        }
    }
    
    return result;
}

double VCO::GenerateBandLimitedSquare() {
    // Use a Fourier series approximation to create a band-limited square
    double result = 0.0;
    double fundamental_freq = base_frequency * pow(2.0, control_voltage * CV_SENSITIVITY);
    fundamental_freq += fm_modulation * base_frequency;
    fundamental_freq = std::max(MIN_FREQ, std::min(MAX_FREQ, fundamental_freq));
    
    int max_harmonics = static_cast<int>(sample_rate / (2 * fundamental_freq));
    max_harmonics = std::min(max_harmonics, 20); // Limit for performance
    
    for (int n = 1; n <= max_harmonics; n++) {
        if ((n - 1) % 2 == 0) { // Only odd harmonics
            int harmonic_number = 2 * n - 1;
            double harmonic_phase = harmonic_number * phase;
            result += (4.0 / (M_PI * harmonic_number)) * sin(harmonic_phase);
        }
    }
    
    return result;
}

double VCO::GenerateBandLimitedPulse() {
    // Use a Fourier series approximation to create a band-limited pulse
    double result = 0.0;
    double fundamental_freq = base_frequency * pow(2.0, control_voltage * CV_SENSITIVITY);
    fundamental_freq += fm_modulation * base_frequency;
    fundamental_freq = std::max(MIN_FREQ, std::min(MAX_FREQ, fundamental_freq));
    
    int max_harmonics = static_cast<int>(sample_rate / (2 * fundamental_freq));
    max_harmonics = std::min(max_harmonics, 20); // Limit for performance
    
    for (int n = 1; n <= max_harmonics; n++) {
        double harmonic_phase = n * phase;
        // For a pulse wave with duty cycle, we include all harmonics
        result += (2.0 / (n * M_PI)) * 
                  sin(M_PI * n * pwm_duty_cycle) * 
                  cos(harmonic_phase - M_PI/2); // Phase shift to center on 0
    }
    
    return result;
}

void VCO::InitializeWaveformTables() {
    // Initialize any lookup tables or precomputed values if needed
    // This is a placeholder for more complex implementations
}

void VCO::SetType(VCOType type) {
    this->type = type;
}

void VCO::SetBaseFrequency(double freq) {
    this->base_frequency = std::max(MIN_FREQ, std::min(MAX_FREQ, freq));
}

void VCO::SetControlVoltage(double cv) {
    this->control_voltage = cv;
}

void VCO::SetAmplitude(double amp) {
    this->amplitude = std::max(0.0, std::min(10.0, amp)); // Limit amplitude to reasonable range
}

void VCO::SetFMModulation(double fm) {
    this->fm_modulation = std::max(-1.0, std::min(1.0, fm)); // Limit FM to ±100%
}

void VCO::SetPWM(double duty_cycle) {
    this->pwm_duty_cycle = std::max(0.01, std::min(0.99, duty_cycle)); // Limit to 1-99%
}

void VCO::SetSampleRate(double rate) {
    this->sample_rate = rate;
}

void VCO::EnableAntiAliasing(bool enable) {
    this->anti_aliasing_enabled = enable;
}