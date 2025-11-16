#include "TubeAudioIO.h"
#include <algorithm>
#include <cmath>

// TubeAudioInput implementation
TubeAudioInput::TubeAudioInput(InputType source_type, double input_impedance, double max_input_level)
    : input_type(source_type), input_impedance(input_impedance), max_input_level(max_input_level),
      current_sample_idx(0), sample_rate(44100), output_signal(0.0), last_processed_signal(0.0), has_audio_data(false) {
    analog_values.resize(1, 0.0);  // Single output connector
}

TubeAudioInput::~TubeAudioInput() {
}

bool TubeAudioInput::Tick() {
    double input_signal = 0.0;

    if (has_audio_data && !audio_data.empty() && !audio_data[0].empty()) {
        // Use pre-loaded audio data
        if (current_sample_idx < audio_data[0].size()) {
            input_signal = audio_data[0][current_sample_idx];
            current_sample_idx++;
            
            // Loop back to beginning if we reach the end
            if (current_sample_idx >= audio_data[0].size()) {
                current_sample_idx = 0;
            }
        }
    } else {
        // For real-time input simulation, we could poll an audio device
        // For now, we'll use the base AnalogNodeBase voltage as input
        // This would be set externally by a real audio input in a real implementation
        input_signal = analog_values[0];
    }

    // Process the input signal based on input type
    output_signal = ProcessInputSignal(input_signal);
    
    // Apply input impedance loading effect
    // In a real tube circuit, this would affect the previous stage
    // For simulation, we'll just note this is happening
    
    analog_values[0] = output_signal;
    UpdateAnalogValue(0, output_signal);

    return true;
}

void TubeAudioInput::SetAudioData(const std::vector<std::vector<double>>& data) {
    audio_data = data;
    has_audio_data = !data.empty() && !data[0].empty();
    current_sample_idx = 0;
}

double TubeAudioInput::ProcessInputSignal(double input_signal) {
    // Apply input-specific processing based on type
    switch (input_type) {
        case InputType::MICROPHONE:
            // Microphone inputs typically have lower levels and may need pre-amplification
            // Simulate microphone preamp gain (typically 20-60dB)
            input_signal *= 30.0;  // 30x gain typical for microphone
            break;
            
        case InputType::INSTRUMENT:
            // Instrument inputs may have specific impedance matching
            // and frequency response characteristics
            // Apply a simple low-pass filter to simulate cable capacitance
            input_signal = input_signal * 0.95 + last_processed_signal * 0.05;
            break;
            
        case InputType::LINE_LEVEL:
            // Line level inputs are already at appropriate levels
            // Apply simple clipping if needed
            if (input_signal > max_input_level) input_signal = max_input_level;
            if (input_signal < -max_input_level) input_signal = -max_input_level;
            break;
            
        default:
            // FILE_INPUT and REALTIME_INPUT
            if (input_signal > max_input_level) input_signal = max_input_level;
            if (input_signal < -max_input_level) input_signal = -max_input_level;
            break;
    }

    // Store for next sample (for instrument input filtering)
    last_processed_signal = input_signal;
    
    return input_signal;
}

// TubeAudioOutput implementation
TubeAudioOutput::TubeAudioOutput(OutputType dest_type, double output_impedance, double load_impedance)
    : output_type(dest_type), output_impedance(output_impedance), load_impedance(load_impedance),
      input_signal(0.0), output_signal(0.0), last_processed_signal(0.0), sample_rate(44100), max_buffer_size(88200),  // 2 seconds at 44.1kHz
      file_output_active(false), buffer_write_pos(0) {
    analog_values.resize(1, 0.0);  // Single input connector
    
    output_buffer.resize(2);  // Stereo by default
    for (auto& channel : output_buffer) {
        channel.resize(max_buffer_size, 0.0);
    }
}

TubeAudioOutput::~TubeAudioOutput() {
    if (file_output_active) {
        StopFileOutput();
    }
}

bool TubeAudioOutput::Tick() {
    // Get input from the tube circuit
    if (analog_values.size() > 0) {
        input_signal = analog_values[0];
    }
    
    // Process the output signal based on output type
    output_signal = ProcessOutputSignal(input_signal);
    
    // Add to output buffer for file writing
    if (file_output_active) {
        output_buffer[0][buffer_write_pos] = output_signal;  // Left channel
        output_buffer[1][buffer_write_pos] = output_signal;  // Right channel (for mono output)
        
        buffer_write_pos = (buffer_write_pos + 1) % max_buffer_size;
        
        // Write buffer to file periodically
        if (buffer_write_pos == 0) {
            WriteBufferToFile();
        }
    }

    return true;
}

double TubeAudioOutput::ProcessOutputSignal(double input_signal) {
    // Apply output-specific processing based on type
    switch (output_type) {
        case OutputType::HEADPHONES:
            // Apply headphone-specific impedance and amplification
            // Headphone amplifiers typically have low output impedance
            input_signal *= 2.0;  // Amplification for headphones
            break;
            
        case OutputType::SPEAKER:
            // Apply speaker-specific characteristics
            // Speakers may have frequency response irregularities
            input_signal *= 0.8;  // Slight attenuation for speakers
            // Apply simple frequency shaping to simulate speaker response
            input_signal = input_signal * 0.95 + last_processed_signal * 0.05;
            break;
            
        case OutputType::LINE_OUT:
            // Apply line out characteristics
            // Ensure signal is within appropriate range
            if (input_signal > 2.0) input_signal = 2.0;  // Line level max
            if (input_signal < -2.0) input_signal = -2.0;
            break;
            
        default:
            // FILE_OUTPUT and REALTIME_OUTPUT
            break;
    }
    
    // Store for next sample (for speaker output filtering)
    last_processed_signal = input_signal;
    
    return input_signal;
}

bool TubeAudioOutput::StartFileOutput(const std::string& filepath, int channels) {
    if (file_output_active) {
        StopFileOutput();
    }

    // Configure output format
    SF_INFO sf_info;
    sf_info.samplerate = sample_rate;
    sf_info.channels = channels;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    // Open file for writing
    output_file_handle.reset(sf_open(filepath.c_str(), SFM_WRITE, &sf_info));
    if (!output_file_handle) {
        LOG("Failed to open audio file for writing: " << filepath << " - " << sf_strerror(nullptr));
        return false;
    }

    current_file_path = filepath;
    file_output_active = true;
    
    // Initialize buffer for the number of channels
    output_buffer.resize(channels);
    for (auto& channel : output_buffer) {
        channel.resize(max_buffer_size, 0.0);
    }

    return true;
}

bool TubeAudioOutput::StopFileOutput() {
    if (!file_output_active) {
        return true;
    }

    // Write any remaining buffered data
    WriteBufferToFile();

    // Close the file
    output_file_handle.reset();
    file_output_active = false;
    
    return true;
}

bool TubeAudioOutput::WriteBufferToFile() {
    if (!file_output_active || !output_file_handle) {
        return false;
    }

    // Convert internal buffer to appropriate format and write to file
    // For simplicity, we'll write in chunks
    std::vector<double> temp_buffer(max_buffer_size * output_buffer.size());
    
    // Interleave samples for multi-channel output
    for (size_t i = 0; i < max_buffer_size; i++) {
        for (size_t ch = 0; ch < output_buffer.size(); ch++) {
            temp_buffer[i * output_buffer.size() + ch] = output_buffer[ch][i];
        }
    }
    
    // Write to file
    sf_count_t frames_written = sf_writef_double(output_file_handle.get(), 
                                                 temp_buffer.data(), 
                                                 max_buffer_size);
    
    return frames_written == max_buffer_size;
}

bool TubeAudioOutput::ClearBuffer() {
    for (auto& channel : output_buffer) {
        std::fill(channel.begin(), channel.end(), 0.0);
    }
    buffer_write_pos = 0;
    return true;
}

// TubeAudioStereoInterface implementation
TubeAudioStereoInterface::TubeAudioStereoInterface()
    : left_input(TubeAudioInput::InputType::LINE_LEVEL),
      right_input(TubeAudioInput::InputType::LINE_LEVEL),
      left_output(TubeAudioOutput::OutputType::LINE_OUT),
      right_output(TubeAudioOutput::OutputType::LINE_OUT),
      sample_rate(44100) {
}

TubeAudioStereoInterface::~TubeAudioStereoInterface() {
}

void TubeAudioStereoInterface::SetSampleRate(int rate) {
    sample_rate = rate;
    left_input.SetSampleRate(rate);
    right_input.SetSampleRate(rate);
    left_output.SetSampleRate(rate);
    right_output.SetSampleRate(rate);
}

bool TubeAudioStereoInterface::StartFileOutput(const std::string& filepath) {
    return left_output.StartFileOutput(filepath, 2);  // 2 channels for stereo
}

bool TubeAudioStereoInterface::StopFileOutput() {
    bool left_result = left_output.StopFileOutput();
    bool right_result = right_output.StopFileOutput();
    return left_result && right_result;
}

bool TubeAudioStereoInterface::Process() {
    // Process both input and output for left and right channels
    bool left_in_ok = left_input.Tick();
    bool right_in_ok = right_input.Tick();
    bool left_out_ok = left_output.Tick();
    bool right_out_ok = right_output.Tick();
    
    return left_in_ok && right_in_ok && left_out_ok && right_out_ok;
}

// ParameterAutomation implementation
ParameterAutomation::ParameterAutomation() {
}

ParameterAutomation::~ParameterAutomation() {
}

void ParameterAutomation::AddAutomationPoint(int param_id, const AutomationPoint& point) {
    auto& param_data = param_map[param_id];
    
    // Insert the point in time-sorted order
    auto& points = param_data.points;
    auto pos = std::lower_bound(points.begin(), points.end(), point, 
                               [](const AutomationPoint& a, const AutomationPoint& b) {
                                   return a.time < b.time;
                               });
    points.insert(pos, point);
}

double ParameterAutomation::GetParameterValue(int param_id, double current_time) {
    if (param_map.find(param_id) == param_map.end()) {
        // Parameter doesn't exist, return 0.0
        return 0.0;
    }
    
    const auto& param_data = param_map[param_id];
    const auto& points = param_data.points;
    
    if (points.empty()) {
        return param_data.current_value;
    }
    
    // Find the relevant automation points
    auto current_point = points.end();
    for (auto it = points.rbegin(); it != points.rend(); ++it) {
        if (it->time <= current_time && it->active) {
            current_point = it.base() - 1;
            break;
        }
    }
    
    if (current_point == points.end()) {
        // No active point before current time, return first point's value or 0
        for (const auto& pt : points) {
            if (pt.active) {
                return pt.value;
            }
        }
        return param_data.current_value;
    }
    
    // If we're exactly at a point, return that value
    if (current_point->time == current_time) {
        return current_point->value;
    }
    
    // Find the next active point
    auto next_point = points.end();
    for (auto it = current_point + 1; it != points.end(); ++it) {
        if (it->active) {
            next_point = it;
            break;
        }
    }
    
    // If no next point, return current point's value
    if (next_point == points.end()) {
        return current_point->value;
    }
    
    // Interpolate between the two points based on interpolation mode
    double t = (current_time - current_point->time) / (next_point->time - current_point->time);
    
    switch (param_data.interp_mode) {
        case InterpolationMode::LINEAR:
            return current_point->value + t * (next_point->value - current_point->value);
            
        case InterpolationMode::SMOOTH:
            // Smooth interpolation using a smoothstep function
            t = t * t * (3.0 - 2.0 * t);
            return current_point->value + t * (next_point->value - current_point->value);
            
        case InterpolationMode::STEP:
        default:
            return current_point->value;
    }
}

void ParameterAutomation::SetParameterAtTime(int param_id, double value) {
    // Add an automation point at the current simulation time
    double current_sim_time = simulation_time;  // Assuming this is available globally
    AddAutomationPoint(param_id, AutomationPoint(current_sim_time, value));
    
    // Update the current value
    param_map[param_id].current_value = value;
}

void ParameterAutomation::SetInterpolationMode(int param_id, InterpolationMode mode) {
    param_map[param_id].interp_mode = mode;
}

ParameterAutomation::InterpolationMode ParameterAutomation::GetInterpolationMode(int param_id) const {
    if (param_map.find(param_id) != param_map.end()) {
        return param_map.at(param_id).interp_mode;
    }
    return InterpolationMode::LINEAR;  // Default
}

void ParameterAutomation::ClearAutomation(int param_id) {
    if (param_map.find(param_id) != param_map.end()) {
        param_map[param_id].points.clear();
    }
}

const std::vector<ParameterAutomation::AutomationPoint>& 
ParameterAutomation::GetAutomationPoints(int param_id) const {
    static std::vector<AutomationPoint> empty_points;
    
    if (param_map.find(param_id) != param_map.end()) {
        return param_map.at(param_id).points;
    }
    
    return empty_points;
}