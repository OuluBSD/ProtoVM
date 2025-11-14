#include "TubeFilter.h"
#include <cmath>
#include <algorithm>

TubeFilter::TubeFilter(TubeFilterType type, double cutoff_freq)
    : filter_type(type)
    , cutoff_frequency(cutoff_freq)
    , resonance(0.5)
    , input_signal(0.0)
    , output(0.0)
    , triode_model(100.0, 1.0, 0.0)  // Default triode parameters
    , anode_voltage(150.0)  // Default anode voltage
    , grid_voltage(GRID_VOLTAGE_OFFSET)  // Default grid bias
{
    // Initialize filter state
    for (int i = 0; i < 4; i++) {
        filter_state[i] = 0.0;
    }
}

bool TubeFilter::Tick() {
    // Apply tube-based filtering with non-linear characteristics
    // The tube affects the filter response, providing warmth and saturation
    
    // Calculate frequency-dependent behavior
    double sample_rate = 44100.0;
    double omega = 2.0 * M_PI * cutoff_frequency / sample_rate;
    
    // Adjust tube parameters based on input signal
    // This creates the non-linear response typical of tube circuits
    double effective_grid_voltage = grid_voltage + input_signal * 0.1; // Small input influence on grid
    
    // Calculate tube amplification factor based on current conditions
    double tube_gain = triode_model.GetAmplificationFactor();
    tube_gain *= (1.0 - std::abs(effective_grid_voltage) / 20.0); // Gain decreases with higher grid voltage
    
    // Apply filter based on type
    switch (filter_type) {
        case TubeFilterType::LOWPASS:
            {
                // RC low-pass with tube characteristics
                double alpha = omega / (omega + 1.0);
                // Apply tube effect to the filtering coefficient
                alpha *= (0.5 + 0.5 * tube_gain / triode_model.GetAmplificationFactor());
                
                // Update filter state
                filter_state[0] = alpha * input_signal + (1.0 - alpha) * filter_state[0];
                
                // Apply resonance (feedback) with tube saturation
                filter_state[0] += resonance * filter_state[3] * 0.5;
                
                // Apply tube saturation to current stage
                filter_state[0] = tanh(filter_state[0] * tube_gain * 0.1) / tube_gain * 10.0;
                
                output = filter_state[0];
            }
            break;
            
        case TubeFilterType::HIGHPASS:
            {
                // RC high-pass with tube characteristics
                double alpha = 1.0 / (omega + 1.0);
                // Apply tube effect to the filtering coefficient
                alpha *= (0.5 + 0.5 * tube_gain / triode_model.GetAmplificationFactor());
                
                // Update filter state
                filter_state[0] = alpha * (input_signal - filter_state[1]) + (1.0 - alpha) * filter_state[0];
                filter_state[1] = filter_state[0];
                
                // Apply tube saturation
                filter_state[0] = tanh(filter_state[0] * tube_gain * 0.1) / tube_gain * 10.0;
                
                output = filter_state[0];
            }
            break;
            
        case TubeFilterType::BANDPASS:
            {
                // Simplified bandpass using combination of lowpass and highpass
                double alpha_lp = omega / (omega + 1.0);
                double alpha_hp = 1.0 / (omega + 1.0);
                
                // Apply tube effect
                alpha_lp *= (0.5 + 0.5 * tube_gain / triode_model.GetAmplificationFactor());
                alpha_hp *= (0.5 + 0.5 * tube_gain / triode_model.GetAmplificationFactor());
                
                // Lowpass stage
                filter_state[0] = alpha_lp * input_signal + (1.0 - alpha_lp) * filter_state[0];
                
                // Highpass stage
                filter_state[1] = alpha_hp * (filter_state[0] - filter_state[2]) + (1.0 - alpha_hp) * filter_state[1];
                filter_state[2] = filter_state[1];
                
                // Apply tube saturation
                filter_state[1] = tanh(filter_state[1] * tube_gain * 0.1) / tube_gain * 10.0;
                
                output = filter_state[1];
            }
            break;
    }
    
    // Update the last stage for feedback purposes
    filter_state[3] = output;
    
    return true;
}

void TubeFilter::SetType(TubeFilterType type) {
    this->filter_type = type;
}

void TubeFilter::SetCutoffFrequency(double freq) {
    this->cutoff_frequency = std::max(MIN_CUTOFF, std::min(MAX_CUTOFF, freq));
}

void TubeFilter::SetInput(double input) {
    this->input_signal = input;
}

void TubeFilter::SetResonance(double resonance) {
    this->resonance = std::max(MIN_RESONANCE, std::min(MAX_RESONANCE, resonance));
}