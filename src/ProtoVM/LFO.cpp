#include "LFO.h"
#include <cmath>
#include <algorithm>

LFO::LFO(LFOType type, double frequency)
    : type(type)
    , frequency(frequency)
    , amplitude(1.0)
    , phase(0.0)
    , output(0.0)
{
}

bool LFO::Tick() {
    // Clamp frequency to valid range
    double freq = std::max(MIN_FREQ, std::min(MAX_FREQ, frequency));
    
    // Calculate phase increment based on frequency and sample rate
    double sample_rate = 44100.0;  // Should be configurable in a real simulation
    double phase_increment = (TWO_PI * freq) / sample_rate;
    
    // Update phase
    phase += phase_increment;
    if (phase > TWO_PI) {
        phase -= TWO_PI;
    }
    
    // Generate waveform based on type
    switch (type) {
        case LFOType::SINE:
            output = amplitude * sin(phase);
            break;
            
        case LFOType::SAWTOOTH:
            // Sawtooth from -1 to 1
            output = amplitude * ((2.0 * phase) / TWO_PI - 1.0);
            break;
            
        case LFOType::TRIANGLE:
            // Triangle wave
            if (phase < M_PI) {
                output = amplitude * (2.0 * phase / M_PI - 1.0);
            } else {
                output = amplitude * (1.0 - 2.0 * (phase - M_PI) / M_PI);
            }
            break;
            
        case LFOType::SQUARE:
            // Square wave
            output = (phase < M_PI) ? amplitude : -amplitude;
            break;
            
        case LFOType::SAMPLE_HOLD:
            // Sample and hold - random value at period intervals
            {
                static bool new_value_needed = true;
                static double hold_value = 0.0;
                
                // Generate a new random value at the start of each period
                if (phase < phase_increment && new_value_needed) {
                    // Simple pseudo-random generator for demo purposes
                    static unsigned int seed = 0;
                    seed = seed * 1103515245 + 12345;
                    hold_value = amplitude * ((double)(seed % 32768) / 16384.0 - 1.0);
                    new_value_needed = false;
                } else if (phase >= M_PI && !new_value_needed) {
                    new_value_needed = true; // Reset for next cycle
                }
                
                output = hold_value;
            }
            break;
    }
    
    return true;
}

void LFO::SetType(LFOType type) {
    this->type = type;
}

void LFO::SetFrequency(double freq) {
    this->frequency = std::max(MIN_FREQ, std::min(MAX_FREQ, freq));
}

void LFO::SetAmplitude(double amp) {
    this->amplitude = std::max(MIN_AMP, std::min(MAX_AMP, amp));
}

void LFO::SetPhase(double phase) {
    this->phase = fmod(phase, TWO_PI);  // Normalize to 0-2π
}