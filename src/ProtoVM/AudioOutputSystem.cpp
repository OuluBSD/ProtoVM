#include "AudioOutputSystem.h"
#include <algorithm>
#include <cmath>
#include <cstring>

AudioOutputSystem::AudioOutputSystem(const AudioFormat& format) 
    : audio_format(format)
    , max_buffer_size(44100 * 10)  // Default to 10 seconds at 44.1kHz
    , output_file_handle(nullptr, &sf_close)
    , real_time_active(false)
    , file_output_active(false)
{
    buffer.reserve(max_buffer_size * audio_format.channels);
    Initialize();
}

AudioOutputSystem::~AudioOutputSystem() {
    if (file_output_active) {
        sf_close(output_file_handle.get());
    }
}

bool AudioOutputSystem::Tick() {
    // The audio output system processes the audio buffer and potentially outputs samples
    // In a real-time context, this would send samples to the audio device
    ProcessOutput();
    return true;
}

void AudioOutputSystem::SetFormat(const AudioFormat& format) {
    audio_format = format;
    
    // Calculate max amplitude based on bit depth
    switch (audio_format.bit_depth) {
        case 8:
            audio_format.max_amplitude = 127.0;
            break;
        case 16:
            audio_format.max_amplitude = 32767.0;
            break;
        case 24:
            audio_format.max_amplitude = 8388607.0;
            break;
        case 32:
            audio_format.max_amplitude = 2147483647.0;
            break;
        default:
            audio_format.max_amplitude = 32767.0; // Default to 16-bit
            break;
    }
}

bool AudioOutputSystem::AddSample(const std::vector<double>& sample) {
    if (sample.size() != audio_format.channels) {
        return false; // Sample size doesn't match channel count
    }
    
    // Check if buffer would exceed maximum size
    if (buffer.size() + audio_format.channels > max_buffer_size * audio_format.channels) {
        // Process the output before adding more samples
        ProcessOutput();
    }
    
    // Add all channel samples to the buffer
    for (double s : sample) {
        buffer.push_back(ConvertSample(s));
    }
    
    return true;
}

bool AudioOutputSystem::AddSample(double mono_sample) {
    if (audio_format.channels != 1) {
        return false; // This method is only for mono output
    }
    
    // Check if buffer would exceed maximum size
    if (buffer.size() + 1 > max_buffer_size) {
        ProcessOutput();
    }
    
    buffer.push_back(ConvertSample(mono_sample));
    
    return true;
}

bool AudioOutputSystem::WriteToFile(const std::string& filepath, int sf_format) {
    SF_INFO sf_info;
    sf_info.samplerate = audio_format.sample_rate;
    sf_info.channels = audio_format.channels;
    sf_info.format = sf_format;
    
    SNDFILE* file = sf_open(filepath.c_str(), SFM_WRITE, &sf_info);
    if (!file) {
        return false;
    }
    
    // Convert our buffer to the appropriate format
    std::vector<short> short_buffer = ConvertBufferToShort();
    
    // Write the buffer to file
    sf_count_t samples_written = sf_write_short(file, short_buffer.data(), short_buffer.size());
    
    sf_close(file);
    
    // Clear the buffer after writing
    buffer.clear();
    
    return samples_written == short_buffer.size();
}

bool AudioOutputSystem::StartRealTimeOutput() {
    // Placeholder for real-time audio output
    // This would require platform-specific audio API (ALSA for Linux, WASAPI for Windows, etc.)
    // For simulation purposes, we'll just set the flag
    real_time_active = true;
    return true;
}

bool AudioOutputSystem::StopRealTimeOutput() {
    real_time_active = false;
    return true;
}

void AudioOutputSystem::ClearBuffer() {
    buffer.clear();
}

void AudioOutputSystem::SetMaxBufferSize(int max_samples) {
    max_buffer_size = max_samples;
    buffer.reserve(max_samples * audio_format.channels);
}

double AudioOutputSystem::ConvertSample(double input_sample) const {
    // Clamp the input sample to [-1, 1] range
    input_sample = std::max(-1.0, std::min(1.0, input_sample));
    
    // Scale to the appropriate range based on bit depth
    double scaled = input_sample * audio_format.max_amplitude;
    
    // Round to nearest integer value and convert back to [-1, 1] range
    double result = std::round(scaled) / audio_format.max_amplitude;
    
    return result;
}

bool AudioOutputSystem::Initialize() {
    // Initialize any required resources
    return true;
}

void AudioOutputSystem::ProcessOutput() {
    // In a real implementation, this would process the audio buffer
    // and potentially send samples to an audio output device
    // For now, we'll just keep this as a placeholder
}

std::vector<short> AudioOutputSystem::ConvertBufferToShort() const {
    std::vector<short> short_buffer;
    short_buffer.reserve(buffer.size());
    
    for (double sample : buffer) {
        // Convert from [-1, 1] range to appropriate integer range
        double scaled = sample * 32767.0;  // Scale to 16-bit range
        short converted = static_cast<short>(std::max(-32768.0, std::min(32767.0, scaled)));
        short_buffer.push_back(converted);
    }
    
    return short_buffer;
}