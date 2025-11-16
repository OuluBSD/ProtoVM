#include "AudioProcessingModes.h"
#include <cmath>

// AudioProcessor implementation
AudioProcessor::AudioProcessor(const std::string& name) 
    : config(AudioMode::STEREO, 2, 44100.0), processor_name(name), bypassed(false) {
    // Initialize with default configuration
    input_channels.resize(2, 0.0);
    output_channels.resize(2, 0.0);
    analog_values.resize(4, 0.0);  // 2 inputs + 2 outputs
}

AudioProcessor::~AudioProcessor() {
}

bool AudioProcessor::Tick() {
    // Update inputs from analog_values (first 2 values are inputs)
    for (int i = 0; i < std::min(static_cast<int>(input_channels.size()), 2); i++) {
        input_channels[i] = analog_values[i];
    }
    
    // Process the audio based on the mode
    if (!bypassed) {
        InternalProcess();
    } else {
        // Bypass mode - pass inputs directly to outputs
        output_channels = input_channels;
    }
    
    // Update outputs in analog_values (last 2 values are outputs)
    for (int i = 0; i < std::min(static_cast<int>(output_channels.size()), 2); i++) {
        analog_values[2 + i] = output_channels[i];
        UpdateAnalogValue(2 + i, output_channels[i]);
    }
    
    return true;
}

void AudioProcessor::SetMode(AudioMode mode) {
    config.mode = mode;
    // Adjust channel count based on mode
    switch (mode) {
        case AudioMode::MONO:
            SetChannelCount(1);
            break;
        case AudioMode::STEREO:
        case AudioMode::M_S_ENCODE:
        case AudioMode::M_S_DECODE:
        case AudioMode::DUAL_MONO:
            SetChannelCount(2);
            break;
        case AudioMode::LEFT_ONLY:
        case AudioMode::RIGHT_ONLY:
            SetChannelCount(1);  // Still uses 2 input channels but outputs 1
            break;
    }
}

void AudioProcessor::SetChannelCount(int count) {
    config.channel_count = count;
    input_channels.resize(count, 0.0);
    output_channels.resize(count, 0.0);
    analog_values.resize(count * 2, 0.0);  // Inputs + outputs
}

void AudioProcessor::SetSampleRate(double rate) {
    config.sample_rate = rate;
}

void AudioProcessor::SetConfig(const AudioChannelConfig& new_config) {
    config = new_config;
    SetChannelCount(config.channel_count);
}

bool AudioProcessor::ProcessBuffer(const std::vector<std::vector<double>>& input, 
                                  std::vector<std::vector<double>>& output) {
    // Process an entire buffer of audio samples
    size_t sample_count = input.empty() ? 0 : input[0].size();
    int channel_count = static_cast<int>(input.size());
    
    // Resize output to match input
    output.resize(channel_count);
    for (auto& channel : output) {
        channel.resize(sample_count, 0.0);
    }
    
    // Process each sample
    for (size_t i = 0; i < sample_count; i++) {
        // Set input channels to current sample values
        for (int ch = 0; ch < channel_count && ch < static_cast<int>(input_channels.size()); ch++) {
            input_channels[ch] = input[ch][i];
        }
        
        // Process this sample
        if (!bypassed) {
            InternalProcess();
        } else {
            output_channels = input_channels;
        }
        
        // Store output values
        for (int ch = 0; ch < channel_count && ch < static_cast<int>(output_channels.size()); ch++) {
            output[ch][i] = output_channels[ch];
        }
    }
    
    return true;
}

void AudioProcessor::SetInputChannels(const std::vector<double>& inputs) {
    input_channels = inputs;
    // Update analog_values as well
    for (size_t i = 0; i < std::min(inputs.size(), analog_values.size()/2); i++) {
        analog_values[i] = inputs[i];
    }
}

// MonoProcessor implementation
MonoProcessor::MonoProcessor(const std::string& name) 
    : AudioProcessor(name) {
    SetMode(AudioMode::MONO);
    mono_output = 0.0;
}

bool MonoProcessor::InternalProcess() {
    // For mono processing, we average all input channels or use just the first
    double sum = 0.0;
    for (double input : GetInputChannels()) {
        sum += input;
    }
    
    if (!GetInputChannels().empty()) {
        mono_output = sum / GetInputChannels().size();
    } else {
        mono_output = 0.0;
    }
    
    // Apply any mono processing here if needed
    // For now, just pass through
    std::vector<double> output = {mono_output};
    const_cast<std::vector<double>&>(GetOutputChannels()) = output;
    
    return true;
}

// StereoProcessor implementation
StereoProcessor::StereoProcessor(const std::string& name) 
    : AudioProcessor(name), stereo_width(1.0), channel_balance(0.0) {
    SetMode(AudioMode::STEREO);
    left_channel.resize(1, 0.0);
    right_channel.resize(1, 0.0);
}

bool StereoProcessor::InternalProcess() {
    auto inputs = GetInputChannels();
    
    // Ensure we have at least 2 channels
    if (inputs.size() < 2) {
        inputs.resize(2, 0.0);
    }
    
    double left = inputs[0];
    double right = inputs[1];
    
    // Apply stereo width
    AudioModeUtils::ApplyStereoWidth(left, right, stereo_width);
    
    // Apply balance
    AudioModeUtils::ApplyBalance(left, right, channel_balance);
    
    // Store results
    std::vector<double> output = {left, right};
    const_cast<std::vector<double>&>(GetOutputChannels()) = output;
    
    return true;
}

// MidSideProcessor implementation
MidSideProcessor::MidSideProcessor(const std::string& name) 
    : AudioProcessor(name), is_encoder(true), mid_signal(0.0), side_signal(0.0) {
    SetMode(AudioMode::M_S_ENCODE);
}

bool MidSideProcessor::InternalProcess() {
    auto inputs = GetInputChannels();
    
    // Ensure we have at least 2 channels
    if (inputs.size() < 2) {
        inputs.resize(2, 0.0);
    }
    
    if (is_encoder) {
        // Convert L,R to M,S
        AudioModeUtils::StereoToMidSide(inputs[0], inputs[1], mid_signal, side_signal);
    } else {
        // Convert M,S to L,R
        double left, right;
        AudioModeUtils::MidSideToStereo(inputs[0], inputs[1], left, right);
        mid_signal = left;
        side_signal = right;
    }
    
    // Store results
    std::vector<double> output = {mid_signal, side_signal};
    const_cast<std::vector<double>&>(GetOutputChannels()) = output;
    
    return true;
}

// DualMonoProcessor implementation
DualMonoProcessor::DualMonoProcessor(const std::string& name) 
    : AudioProcessor(name), ch1_param1(1.0), ch1_param2(0.0), 
      ch2_param1(1.0), ch2_param2(0.0) {
    SetMode(AudioMode::DUAL_MONO);
}

bool DualMonoProcessor::InternalProcess() {
    auto inputs = GetInputChannels();
    
    // Ensure we have at least 2 channels
    if (inputs.size() < 2) {
        inputs.resize(2, 0.0);
    }
    
    // Process each channel independently
    double ch1_out = inputs[0] * ch1_param1 + ch1_param2;
    double ch2_out = inputs[1] * ch2_param1 + ch2_param2;
    
    // Store results
    std::vector<double> output = {ch1_out, ch2_out};
    const_cast<std::vector<double>&>(GetOutputChannels()) = output;
    
    return true;
}

// AudioModeUtils implementation
namespace AudioModeUtils {
    double StereoToMono(const std::vector<double>& stereo_input) {
        if (stereo_input.size() < 2) {
            return stereo_input.empty() ? 0.0 : stereo_input[0];
        }
        // Average left and right channels
        return (stereo_input[0] + stereo_input[1]) * 0.5;
    }
    
    std::vector<double> MonoToStereo(double mono_input) {
        // Duplicate mono to both stereo channels
        return {mono_input, mono_input};
    }
    
    void StereoToMidSide(double left, double right, double& mid, double& side) {
        mid = (left + right) * 0.5;   // Mid = average of L and R
        side = (left - right) * 0.5;  // Side = half the difference
    }
    
    void MidSideToStereo(double mid, double side, double& left, double& right) {
        left = mid + side;   // L = mid + side
        right = mid - side;  // R = mid - side
    }
    
    void ApplyBalance(double& left, double& right, double balance) {
        // Balance: -1.0 = full left, 0.0 = center, 1.0 = full right
        if (balance < 0) {
            // Attenuate right channel
            right *= (1.0 + balance);
        } else {
            // Attenuate left channel
            left *= (1.0 - balance);
        }
    }
    
    void ApplyStereoWidth(double& left, double& right, double width) {
        // Width: 0.0 = mono (L=R), 1.0 = full stereo
        if (width < 1.0) {
            double mid = (left + right) * 0.5;
            left = mid + (left - mid) * width;
            right = mid + (right - mid) * width;
        }
    }
}
