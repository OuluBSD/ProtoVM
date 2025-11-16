#include "EffectsProcessor.h"
#include <cmath>
#include <algorithm>

EffectsProcessor::EffectsProcessor(EffectType type)
    : effect_type(type)
    , input_signal(0.0)
    , output(0.0)
    , is_enabled(true)
    , wet_dry_mix(0.5)
    , parameter_count(0)
    , delay_buffer_size(44100)  // 1 second at 44.1kHz
    , read_index(0)
    , write_index(0)
    , lfo_phase(0.0)
    , lfo_rate(1.0)
    , feedback(0.3)
    , compressor_threshold(0.5)
    , compressor_ratio(3.0)
    , compressor_attack(0.01)
    , compressor_release(0.1)
    , last_output(0.0)
{
    // Initialize delay buffer
    delay_buffer.resize(delay_buffer_size, 0.0);
    
    // Initialize filter state
    for (int i = 0; i < 4; i++) {
        filter_state[i] = 0.0;
    }
    
    // Initialize effect-specific parameters
    InitializeParameters();
}

EffectsProcessor::~EffectsProcessor() {
    // Cleanup handled by stack variables
}

bool EffectsProcessor::Tick() {
    if (!is_enabled) {
        output = input_signal;
        return true;
    }
    
    switch (effect_type) {
        case EffectType::REVERB:
            ProcessReverb();
            break;
            
        case EffectType::DELAY:
            ProcessDelay();
            break;
            
        case EffectType::CHORUS:
            ProcessChorus();
            break;
            
        case EffectType::PHASER:
            ProcessPhaser();
            break;
            
        case EffectType::FLANGER:
            ProcessFlanger();
            break;
            
        case EffectType::COMPRESSOR:
            ProcessCompressor();
            break;
            
        case EffectType::DISTORTION:
            ProcessDistortion();
            break;
            
        case EffectType::TREMOLO:
            ProcessTremolo();
            break;
            
        case EffectType::WAH_WAH:
            ProcessWahWah();
            break;
            
        case EffectType::PARAMETRIC_EQ:
            ProcessParametricEQ();
            break;
            
        default:
            output = input_signal;
            break;
    }
    
    return true;
}

void EffectsProcessor::SetType(EffectType type) {
    this->effect_type = type;
    InitializeParameters();
}

void EffectsProcessor::SetInput(double input) {
    this->input_signal = input;
}

int EffectsProcessor::AddParameter(const EffectParameter& param) {
    parameters.push_back(param);
    return parameter_count++;
}

EffectParameter* EffectsProcessor::GetParameter(int id) {
    if (id >= 0 && id < static_cast<int>(parameters.size())) {
        return &parameters[id];
    }
    return nullptr;
}

bool EffectsProcessor::SetParameterValue(int id, double value) {
    if (id >= 0 && id < static_cast<int>(parameters.size())) {
        // Clamp value to valid range
        value = std::max(parameters[id].min_value, std::min(parameters[id].max_value, value));
        parameters[id].current_value = value;
        return true;
    }
    return false;
}

bool EffectsProcessor::SetParameterByName(const std::string& name, double value) {
    for (size_t i = 0; i < parameters.size(); i++) {
        if (parameters[i].name == name) {
            return SetParameterValue(static_cast<int>(i), value);
        }
    }
    return false;
}

double EffectsProcessor::GetParameterValue(int id) const {
    if (id >= 0 && id < static_cast<int>(parameters.size())) {
        return parameters[id].current_value;
    }
    return 0.0;
}

double EffectsProcessor::GetParameterByName(const std::string& name) const {
    for (size_t i = 0; i < parameters.size(); i++) {
        if (parameters[i].name == name) {
            return parameters[i].current_value;
        }
    }
    return 0.0;
}

void EffectsProcessor::ProcessReverb() {
    // Simplified reverb using multiple delay lines with different feedback
    double dry = input_signal;
    double wet = 0.0;
    
    // Get reverb-specific parameters
    double room_size = GetParameterByName("Room Size");
    double damping = GetParameterByName("Damping");
    double width = GetParameterByName("Width");
    
    // Simplified reverb algorithm using feedback delay networks
    // This is a very simplified version - real reverb is much more complex
    
    // Update delay time based on room size
    int delay_samples = static_cast<int>(delay_buffer_size * 0.1 * room_size);
    delay_samples = std::max(1, std::min(delay_samples, delay_buffer_size - 1));
    
    // Calculate read position
    int read_pos = (write_index - delay_samples + delay_buffer_size) % delay_buffer_size;
    
    // Apply feedback
    double delayed = delay_buffer[read_pos];
    double input_with_feedback = input_signal + feedback * delayed * (1.0 - damping);
    
    // Store in delay buffer
    delay_buffer[write_index] = input_with_feedback;
    write_index = (write_index + 1) % delay_buffer_size;
    
    // Output is combination of input and delayed signal
    wet = 0.7 * input_signal + 0.3 * delayed;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessDelay() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get delay-specific parameters
    double delay_time = GetParameterByName("Delay Time");  // In seconds
    feedback = GetParameterByName("Feedback");
    double tone = GetParameterByName("Tone");
    
    // Calculate delay in samples
    int delay_samples = static_cast<int>(delay_time * 44100.0);  // Assuming 44.1kHz
    delay_samples = std::max(1, std::min(delay_samples, delay_buffer_size - 1));
    
    // Calculate read position
    int read_pos = (write_index - delay_samples + delay_buffer_size) % delay_buffer_size;
    
    // Get delayed signal
    double delayed = delay_buffer[read_pos];
    
    // Apply feedback and tone control
    double input_with_feedback = input_signal + feedback * delayed * (1.0 - tone * 0.5);
    
    // Store in delay buffer
    delay_buffer[write_index] = input_with_feedback;
    write_index = (write_index + 1) % delay_buffer_size;
    
    wet = delayed;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessChorus() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get chorus-specific parameters
    lfo_rate = GetParameterByName("LFO Rate");  // In Hz
    double depth = GetParameterByName("Depth");
    double feedback = GetParameterByName("Feedback");
    
    // Update LFO phase
    double lfo_increment = (2.0 * M_PI * lfo_rate) / 44100.0;
    lfo_phase += lfo_increment;
    if (lfo_phase > 2.0 * M_PI) {
        lfo_phase -= 2.0 * M_PI;
    }
    
    // Calculate modulated delay (0.5ms to 5ms variation)
    double modulation = 0.5 + 0.5 * sin(lfo_phase);  // 0 to 1
    int delay_samples = static_cast<int>(2.0 + depth * 8.0 * modulation); // 2-10 samples
    
    delay_samples = std::max(1, std::min(delay_samples, 100)); // Limit to reasonable range
    
    // Calculate read position with fractional delay for smooth modulation
    int read_pos = (write_index - delay_samples + delay_buffer_size) % delay_buffer_size;
    
    // Get delayed signal
    double delayed = delay_buffer[read_pos];
    
    // Apply feedback and store
    double input_with_feedback = input_signal + feedback * delayed;
    delay_buffer[write_index] = input_with_feedback;
    write_index = (write_index + 1) % delay_buffer_size;
    
    wet = 0.6 * input_signal + 0.4 * delayed;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessPhaser() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get phaser-specific parameters
    lfo_rate = GetParameterByName("LFO Rate");
    double depth = GetParameterByName("Depth");
    int stages = static_cast<int>(GetParameterByName("Stages"));
    
    // Update LFO phase
    double lfo_increment = (2.0 * M_PI * lfo_rate) / 44100.0;
    lfo_phase += lfo_increment;
    if (lfo_phase > 2.0 * M_PI) {
        lfo_phase -= 2.0 * M_PI;
    }
    
    // Calculate sweep frequency based on LFO
    double sweep_freq = 100.0 + 1900.0 * (0.5 + 0.5 * sin(lfo_phase) * depth);
    
    // Apply all-pass filter stages to create phaser effect
    double signal = input_signal;
    double dt = 1.0 / 44100.0;
    
    // Simplified phaser using first-order all-pass filters
    for (int i = 0; i < stages && i < 4; i++) {
        // Calculate coefficient for all-pass filter
        double rc = 1.0 / (2.0 * M_PI * sweep_freq);
        double alpha = dt / (rc + dt);
        
        // Apply first-order all-pass filter
        double output = alpha * signal + filter_state[i] - alpha * filter_state[i];
        filter_state[i] = output;
        signal = output;
    }
    
    wet = signal;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessFlanger() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get flanger-specific parameters
    lfo_rate = GetParameterByName("LFO Rate");
    double depth = GetParameterByName("Depth");
    feedback = GetParameterByName("Feedback");
    
    // Update LFO phase
    double lfo_increment = (2.0 * M_PI * lfo_rate) / 44100.0;
    lfo_phase += lfo_increment;
    if (lfo_phase > 2.0 * M_PI) {
        lfo_phase -= 2.0 * M_PI;
    }
    
    // Calculate modulated delay (0-10ms variation)
    double modulation = 0.5 + 0.5 * sin(lfo_phase);  // 0 to 1
    int delay_samples = static_cast<int>(depth * 10.0 * modulation); // 0-10ms * sample rate
    
    delay_samples = std::max(1, std::min(delay_samples, 500)); // Limit to reasonable range
    
    // Calculate read position
    int read_pos = (write_index - delay_samples + delay_buffer_size) % delay_buffer_size;
    
    // Get delayed signal
    double delayed = delay_buffer[read_pos];
    
    // Apply feedback
    double input_with_feedback = input_signal + feedback * delayed;
    delay_buffer[write_index] = input_with_feedback;
    write_index = (write_index + 1) % delay_buffer_size;
    
    wet = 0.7 * input_signal + 0.3 * delayed;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessCompressor() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get compressor-specific parameters
    compressor_threshold = GetParameterByName("Threshold");
    compressor_ratio = GetParameterByName("Ratio");
    compressor_attack = GetParameterByName("Attack");
    compressor_release = GetParameterByName("Release");
    double makeup_gain = GetParameterByName("Makeup Gain");
    
    // Calculate compression factor based on input level
    double input_level = fabs(input_signal);
    double gain = 1.0;  // Default gain
    
    if (input_level > compressor_threshold) {
        // Apply compression formula: output = threshold + (input - threshold) / ratio
        gain = compressor_threshold + (input_level - compressor_threshold) / compressor_ratio;
        gain = gain / input_level;  // Convert to gain factor
    }
    
    // Apply soft knee compression
    if (input_level > compressor_threshold * 0.9) {
        double knee_width = 0.1 * compressor_threshold;
        if (input_level <= compressor_threshold * 1.1) {
            // Apply smooth transition in knee region
            double knee = input_level - compressor_threshold * 0.9;
            double linear_gain = 1.0;
            double compressed_gain = compressor_threshold + 
                (input_level - compressor_threshold) / compressor_ratio;
            compressed_gain = compressed_gain / input_level;
            
            double mix = knee / knee_width;  // 0 to 1 across knee
            gain = linear_gain + mix * (compressed_gain - linear_gain);
        }
    }
    
    wet = input_signal * gain * makeup_gain;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessDistortion() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get distortion-specific parameters
    double drive = GetParameterByName("Drive");
    double tone = GetParameterByName("Tone");
    double mix = GetParameterByName("Mix");
    
    // Apply gain boost before distortion
    double pre_gain = input_signal * (1.0 + drive * 5.0);
    
    // Apply soft clipping (tanh) for smooth distortion
    double distorted = tanh(pre_gain);
    
    // Apply tone control with simple filter
    if (tone < 0.5) {
        // Low-pass filtering for warmer tone
        double alpha = 0.1 + 0.4 * tone;  // 0.1 to 0.3
        filter_state[0] = alpha * distorted + (1.0 - alpha) * filter_state[0];
        distorted = filter_state[0];
    } else {
        // High-pass filtering for brighter tone
        double alpha = 0.1 + 0.4 * (1.0 - tone);  // 0.1 to 0.3
        filter_state[0] = alpha * distorted + (1.0 - alpha) * (filter_state[0] - input_signal) + input_signal;
        distorted = filter_state[0];
    }
    
    wet = distorted;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessTremolo() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get tremolo-specific parameters
    lfo_rate = GetParameterByName("Rate");
    double depth = GetParameterByName("Depth");
    
    // Update LFO phase
    double lfo_increment = (2.0 * M_PI * lfo_rate) / 44100.0;
    lfo_phase += lfo_increment;
    if (lfo_phase > 2.0 * M_PI) {
        lfo_phase -= 2.0 * M_PI;
    }
    
    // Calculate amplitude modulation
    double amplitude_mod = (1.0 - depth) + depth * (0.5 + 0.5 * sin(lfo_phase));
    
    wet = input_signal * amplitude_mod;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessWahWah() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get wah-wah-specific parameters
    lfo_rate = GetParameterByName("Pedal Position");  // In this case, it's controlled by pedal position
    double resonance = GetParameterByName("Resonance");
    double sweep_range = GetParameterByName("Sweep Range");
    
    // Calculate center frequency based on pedal position (0-1 range)
    double center_freq = 200.0 + (2500.0 - 200.0) * lfo_rate * sweep_range;
    
    // Apply resonant bandpass filter
    double sample_rate = 44100.0;
    double omega = 2.0 * M_PI * center_freq / sample_rate;
    double sn = sin(omega);
    double cs = cos(omega);
    double alpha = sn / (2.0 * resonance);  // BW = resonance
    
    // Direct form 2 transposed biquad bandpass filter
    double a0 = 1.0 + alpha;
    double b0 = alpha;
    double b1 = 0.0;
    double b2 = -alpha;
    double a1 = -2.0 * cs / a0;
    double a2 = (1.0 - alpha) / a0;
    
    // Apply the filter
    double input = input_signal;
    double output_filtered = b0/a0 * input + b1/a0 * filter_state[0] + b2/a0 * filter_state[1] 
                            - a1 * filter_state[2] - a2 * filter_state[3];
    
    // Update delay elements
    filter_state[1] = filter_state[0];
    filter_state[0] = input;
    filter_state[3] = filter_state[2];
    filter_state[2] = output_filtered;
    
    wet = output_filtered;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::ProcessParametricEQ() {
    double dry = input_signal;
    double wet = 0.0;
    
    // Get EQ-specific parameters
    double low_freq = GetParameterByName("Low Freq");
    double low_gain = GetParameterByName("Low Gain");
    double mid_freq = GetParameterByName("Mid Freq");
    double mid_gain = GetParameterByName("Mid Gain");
    double high_freq = GetParameterByName("High Freq");
    double high_gain = GetParameterByName("High Gain");
    double quality = GetParameterByName("Quality");
    
    // Apply three bands using biquad filters
    double sample_rate = 44100.0;
    double output_signal = input_signal;
    
    // Low shelving filter
    if (low_gain != 0.0) {
        double A = pow(10.0, low_gain / 40.0);  // Amplitude factor
        double omega = 2.0 * M_PI * low_freq / sample_rate;
        double sn = sin(omega);
        double cs = cos(omega);
        double alpha = sn * sinh(log(2.0)/2.0 * quality * omega/sn);
        
        double a0 = (A + 1) + (A - 1) * cs + 2 * sqrt(A) * alpha;
        double a1 = -2 * ((A - 1) + (A + 1) * cs);
        double a2 = (A + 1) + (A - 1) * cs - 2 * sqrt(A) * alpha;
        double b0 = A * ((A + 1) - (A - 1) * cs + 2 * sqrt(A) * alpha);
        double b1 = 2 * A * ((A - 1) - (A + 1) * cs);
        double b2 = A * ((A + 1) - (A - 1) * cs - 2 * sqrt(A) * alpha);
        
        // Apply the filter
        double input = output_signal;
        output_signal = b0/a0 * input + b1/a0 * filter_state[0] + b2/a0 * filter_state[1] 
                         - a1 * filter_state[2] - a2 * filter_state[3];
        
        // Update delay elements
        filter_state[1] = filter_state[0];
        filter_state[0] = input;
        filter_state[3] = filter_state[2];
        filter_state[2] = output_signal;
    }
    
    // Peaking filter for mid
    if (mid_gain != 0.0) {
        double A = pow(10.0, mid_gain / 40.0);
        double omega = 2.0 * M_PI * mid_freq / sample_rate;
        double sn = sin(omega);
        double cs = cos(omega);
        double alpha = sn * sinh(log(2.0)/2.0 * quality * omega/sn);
        
        double b0 = 1 + alpha * A;
        double b1 = -2 * cs;
        double b2 = 1 - alpha * A;
        double a0 = 1 + alpha / A;
        double a1 = -2 * cs;
        double a2 = 1 - alpha / A;
        
        // Normalize
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        a0 = 1;
        
        // Apply the filter
        double input = output_signal;
        output_signal = b0 * input + b1 * filter_state[0] + b2 * filter_state[1] 
                         - a1 * filter_state[2] - a2 * filter_state[3];
        
        // Update delay elements
        filter_state[1] = filter_state[0];
        filter_state[0] = input;
        filter_state[3] = filter_state[2];
        filter_state[2] = output_signal;
    }
    
    // High shelving filter
    if (high_gain != 0.0) {
        double A = pow(10.0, high_gain / 40.0);
        double omega = 2.0 * M_PI * high_freq / sample_rate;
        double sn = sin(omega);
        double cs = cos(omega);
        double alpha = sn * sinh(log(2.0)/2.0 * quality * omega/sn);
        
        double a0 = (A + 1) - (A - 1) * cs + 2 * sqrt(A) * alpha;
        double a1 = 2 * ((A - 1) - (A + 1) * cs);
        double a2 = (A + 1) - (A - 1) * cs - 2 * sqrt(A) * alpha;
        double b0 = A * ((A + 1) + (A - 1) * cs + 2 * sqrt(A) * alpha);
        double b1 = -2 * A * ((A - 1) + (A + 1) * cs);
        double b2 = A * ((A + 1) + (A - 1) * cs - 2 * sqrt(A) * alpha);
        
        // Apply the filter
        double input = output_signal;
        output_signal = b0/a0 * input + b1/a0 * filter_state[0] + b2/a0 * filter_state[1] 
                         - a1 * filter_state[2] - a2 * filter_state[3];
        
        // Update delay elements
        filter_state[1] = filter_state[0];
        filter_state[0] = input;
        filter_state[3] = filter_state[2];
        filter_state[2] = output_signal;
    }
    
    wet = output_signal;
    
    output = ApplyWetDryMix(dry, wet);
}

void EffectsProcessor::SetWetDryMix(double mix) {
    this->wet_dry_mix = std::max(0.0, std::min(1.0, mix));
}

double EffectsProcessor::ApplyWetDryMix(double dry, double wet) const {
    return dry * (1.0 - wet_dry_mix) + wet * wet_dry_mix;
}

void EffectsProcessor::InitializeParameters() {
    parameters.clear();
    parameter_count = 0;
    
    switch (effect_type) {
        case EffectType::REVERB:
            AddParameter(EffectParameter("Room Size", 0.1, 1.0, 0.7, ""));
            AddParameter(EffectParameter("Damping", 0.0, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Width", 0.0, 1.0, 1.0, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 0.5, ""));
            break;
            
        case EffectType::DELAY:
            AddParameter(EffectParameter("Delay Time", 0.01, 2.0, 0.5, "s"));
            AddParameter(EffectParameter("Feedback", 0.0, 0.9, 0.3, ""));
            AddParameter(EffectParameter("Tone", 0.0, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 0.4, ""));
            break;
            
        case EffectType::CHORUS:
            AddParameter(EffectParameter("LFO Rate", 0.1, 10.0, 1.0, "Hz"));
            AddParameter(EffectParameter("Depth", 0.0, 1.0, 0.3, ""));
            AddParameter(EffectParameter("Feedback", 0.0, 0.9, 0.2, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 0.3, ""));
            break;
            
        case EffectType::PHASER:
            AddParameter(EffectParameter("LFO Rate", 0.1, 20.0, 0.5, "Hz"));
            AddParameter(EffectParameter("Depth", 0.0, 1.0, 0.8, ""));
            AddParameter(EffectParameter("Stages", 2.0, 12.0, 6.0, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 0.5, ""));
            break;
            
        case EffectType::FLANGER:
            AddParameter(EffectParameter("LFO Rate", 0.1, 10.0, 0.25, "Hz"));
            AddParameter(EffectParameter("Depth", 0.0, 1.0, 0.6, ""));
            AddParameter(EffectParameter("Feedback", 0.0, 0.9, 0.5, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 0.4, ""));
            break;
            
        case EffectType::COMPRESSOR:
            AddParameter(EffectParameter("Threshold", 0.1, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Ratio", 1.0, 20.0, 3.0, ":1"));
            AddParameter(EffectParameter("Attack", 0.001, 0.5, 0.01, "s"));
            AddParameter(EffectParameter("Release", 0.01, 1.0, 0.1, "s"));
            AddParameter(EffectParameter("Makeup Gain", 1.0, 4.0, 1.0, ""));
            break;
            
        case EffectType::DISTORTION:
            AddParameter(EffectParameter("Drive", 0.0, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Tone", 0.0, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Mix", 0.0, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 0.3, ""));
            break;
            
        case EffectType::TREMOLO:
            AddParameter(EffectParameter("Rate", 0.1, 10.0, 4.0, "Hz"));
            AddParameter(EffectParameter("Depth", 0.0, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 1.0, ""));
            break;
            
        case EffectType::WAH_WAH:
            AddParameter(EffectParameter("Pedal Position", 0.0, 1.0, 0.5, ""));
            AddParameter(EffectParameter("Resonance", 0.5, 10.0, 2.0, ""));
            AddParameter(EffectParameter("Sweep Range", 0.1, 1.0, 1.0, ""));
            AddParameter(EffectParameter("Dry/Wet", 0.0, 1.0, 0.7, ""));
            break;
            
        case EffectType::PARAMETRIC_EQ:
            AddParameter(EffectParameter("Low Freq", 20.0, 500.0, 100.0, "Hz"));
            AddParameter(EffectParameter("Low Gain", -12.0, 12.0, 0.0, "dB"));
            AddParameter(EffectParameter("Mid Freq", 200.0, 2000.0, 1000.0, "Hz"));
            AddParameter(EffectParameter("Mid Gain", -12.0, 12.0, 0.0, "dB"));
            AddParameter(EffectParameter("High Freq", 1000.0, 20000.0, 10000.0, "Hz"));
            AddParameter(EffectParameter("High Gain", -12.0, 12.0, 0.0, "dB"));
            AddParameter(EffectParameter("Quality", 0.1, 10.0, 1.0, ""));
            break;
    }
}