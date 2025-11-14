#include "VCO.h"
#include <cmath>
#include <algorithm>

VCO::VCO(VCOType type, double base_frequency)
    : type(type)
    , base_frequency(base_frequency)
    , control_voltage(0.0)
    , amplitude(1.0)
    , fm_modulation(0.0)
    , pwm_duty_cycle(0.5)
    , phase(0.0)
    , output(0.0)
{
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
    
    // Calculate phase increment based on frequency and sample rate (assuming 44.1kHz)
    // This is a simplified model - in a real simulation we'd use the actual simulation tick rate
    double sample_rate = 44100.0;  // Should be configurable in a real simulation
    double phase_increment = (TWO_PI * frequency) / sample_rate;
    
    // Update phase
    phase += phase_increment;
    if (phase > TWO_PI) {
        phase -= TWO_PI;
    }
    
    // Generate waveform based on type
    switch (type) {
        case VCOType::SINE:
            output = amplitude * sin(phase);
            break;
            
        case VCOType::SAWTOOTH:
            // Sawtooth from -1 to 1
            output = amplitude * ((2.0 * phase) / TWO_PI - 1.0);
            break;
            
        case VCOType::TRIANGLE:
            // Triangle wave
            if (phase < M_PI) {
                output = amplitude * (2.0 * phase / M_PI - 1.0);
            } else {
                output = amplitude * (1.0 - 2.0 * (phase - M_PI) / M_PI);
            }
            break;
            
        case VCOType::SQUARE:
            // Square wave with PWM
            output = (phase < TWO_PI * pwm_duty_cycle) ? amplitude : -amplitude;
            break;
            
        case VCOType::NOISE:
            // Simple pseudo-random noise - in real implementation would be different
            // Using a simple pseudo-random generator for demo purposes
            {
                static unsigned int seed = 0;  // Would need proper seeding in real implementation
                seed = seed * 1103515245 + 12345;
                output = amplitude * ((double)(seed % 32768) / 16384.0 - 1.0);
            }
            break;
    }
    
    return true;
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