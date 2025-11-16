#include "AdditionalSources.h"
#include <algorithm>
#include <fstream>

// Antenna implementation
Antenna::Antenna(double sensitivity, double frequency) 
    : sensitivity(sensitivity), frequency(frequency), gen(rd()), noise_dist(0.0, 0.01) {
    analog_values.resize(1, 0.0);  // Single terminal relative to system ground
}

bool Antenna::Tick() {
    // Simulate receiving electromagnetic signals
    double time = simulation_time;
    
    // Generate a realistic signal that might come from an antenna
    // This includes multiple possible frequencies and some noise
    double signal = 0.0;
    
    // Primary signal at set frequency
    signal += sensitivity * 0.5 * sin(2.0 * PI * frequency * time);
    
    // Add some nearby frequencies to simulate realistic reception
    signal += sensitivity * 0.3 * sin(2.0 * PI * (frequency * 1.01) * time);
    signal += sensitivity * 0.2 * sin(2.0 * PI * (frequency * 0.99) * time);
    
    // Add realistic noise
    signal += noise_dist(gen);
    
    analog_values[0] = signal;
    UpdateAnalogValue(0, signal);
    
    return true;
}

void Antenna::SetSensitivity(double sens) {
    sensitivity = sens;
}

void Antenna::SetFrequency(double freq) {
    frequency = freq;
}

// AM Source implementation
AmSource::AmSource(double carrier_freq, double modulation_freq, 
                   double modulation_index, double amplitude) 
    : carrier_freq(carrier_freq), modulation_freq(modulation_freq), 
      modulation_index(modulation_index), amplitude(amplitude) {
    analog_values.resize(1, 0.0);
}

bool AmSource::Tick() {
    double time = simulation_time;
    
    // Generate AM signal: (1 + m*cos(wm*t)) * A*cos(wc*t)
    // where m = modulation index, wm = modulation freq, wc = carrier freq, A = amplitude
    double carrier = sin(2.0 * PI * carrier_freq * time);
    double modulating = cos(2.0 * PI * modulation_freq * time);
    double am_signal = amplitude * (1.0 + modulation_index * modulating) * carrier;
    
    analog_values[0] = am_signal;
    UpdateAnalogValue(0, am_signal);
    
    return true;
}

void AmSource::SetCarrierFrequency(double freq) {
    carrier_freq = freq;
}

void AmSource::SetModulationFrequency(double freq) {
    modulation_freq = freq;
}

void AmSource::SetModulationIndex(double index) {
    modulation_index = index;
    if (modulation_index < 0.0) modulation_index = 0.0;
    if (modulation_index > 1.0) modulation_index = 1.0;  // Prevent overmodulation
}

void AmSource::SetAmplitude(double amp) {
    amplitude = amp;
}

// FM Source implementation
FmSource::FmSource(double carrier_freq, double modulation_freq, 
                   double modulation_index, double amplitude) 
    : carrier_freq(carrier_freq), modulation_freq(modulation_freq), 
      modulation_index(modulation_index), amplitude(amplitude) {
    analog_values.resize(1, 0.0);
}

bool FmSource::Tick() {
    double time = simulation_time;
    
    // Generate FM signal: A * cos(wc*t + m*sin(wm*t))
    // where m = modulation index, wm = modulation freq, wc = carrier freq, A = amplitude
    double carrier_phase = 2.0 * PI * carrier_freq * time;
    double modulating = sin(2.0 * PI * modulation_freq * time);
    double fm_signal = amplitude * cos(carrier_phase + modulation_index * modulating);
    
    analog_values[0] = fm_signal;
    UpdateAnalogValue(0, fm_signal);
    
    return true;
}

void FmSource::SetCarrierFrequency(double freq) {
    carrier_freq = freq;
}

void FmSource::SetModulationFrequency(double freq) {
    modulation_freq = freq;
}

void FmSource::SetModulationIndex(double index) {
    modulation_index = index;
}

void FmSource::SetAmplitude(double amp) {
    amplitude = amp;
}

// Current Source implementation
CurrentSource::CurrentSource(double current) : current_val(current) {
    analog_values.resize(2, 0.0);  // Two terminals for current source
}

bool CurrentSource::Tick() {
    // In an ideal current source, the current is constant regardless of voltage
    // For simulation purposes, we'll output a voltage that would create the desired current
    // assuming some typical load conditions
    analog_values[0] = current_val * 1000.0;  // Assuming 1k load for voltage calculation
    analog_values[1] = 0.0;  // Reference terminal
    
    UpdateAnalogValue(0, analog_values[0]);
    UpdateAnalogValue(1, analog_values[1]);
    
    return true;
}

void CurrentSource::SetCurrent(double current) {
    current_val = current;
}

// Noise Generator implementation
NoiseGenerator::NoiseGenerator(NoiseType type, double amplitude) 
    : noise_type(type), amplitude(amplitude), gen(rd()), white_noise_dist(0.0, 1.0) {
    analog_values.resize(1, 0.0);
    
    // Initialize pink noise buffer
    for (int i = 0; i < 3; i++) {
        pink_buffer[i] = 0.0;
    }
    
    // Initialize brown noise value
    brown_value = 0.0;
}

bool NoiseGenerator::Tick() {
    double noise = 0.0;
    
    switch (noise_type) {
        case NoiseType::WHITE:
            noise = white_noise_dist(gen) * amplitude;
            break;
            
        case NoiseType::PINK: {
            // Simple approximation of pink noise using a weighted sum of random numbers
            double white = white_noise_dist(gen);
            pink_buffer[0] = 0.99765 * pink_buffer[0] + white * 0.0990460;
            pink_buffer[1] = 0.96300 * pink_buffer[1] + white * 0.2965164;
            pink_buffer[2] = 0.57000 * pink_buffer[2] + white * 1.0526913;
            noise = (pink_buffer[0] + pink_buffer[1] + pink_buffer[2] + white * 0.1848) * amplitude;
            break;
        }
            
        case NoiseType::BROWN: {
            // Brown noise (random walk)
            double increment = white_noise_dist(gen) * amplitude * 0.1;
            brown_value += increment;
            // Add some decay to keep values bounded
            brown_value *= 0.99;
            noise = brown_value;
            break;
        }
    }
    
    analog_values[0] = noise;
    UpdateAnalogValue(0, noise);
    
    return true;
}

void NoiseGenerator::SetNoiseType(NoiseType type) {
    noise_type = type;
}

void NoiseGenerator::SetAmplitude(double amp) {
    amplitude = amp;
}

// Audio Input implementation
AudioInput::AudioInput(double amplitude, double frequency) 
    : amplitude(amplitude), frequency(frequency), current_sample_idx(0), has_audio_data(false) {
    analog_values.resize(1, 0.0);
}

bool AudioInput::Tick() {
    double signal = 0.0;
    
    if (has_audio_data && !audio_data.empty()) {
        // Use pre-loaded audio data
        if (current_sample_idx < audio_data.size()) {
            signal = audio_data[current_sample_idx];
            current_sample_idx = (current_sample_idx + 1) % audio_data.size();
        }
    } else {
        // Generate a test tone
        double time = simulation_time;
        signal = amplitude * sin(2.0 * PI * frequency * time);
        
        // Add some harmonic content to make it more realistic
        signal += (amplitude * 0.3) * sin(2.0 * PI * (frequency * 2) * time);  // Second harmonic
        signal += (amplitude * 0.1) * sin(2.0 * PI * (frequency * 3) * time);  // Third harmonic
    }
    
    analog_values[0] = signal;
    UpdateAnalogValue(0, signal);
    
    return true;
}

void AudioInput::SetAmplitude(double amp) {
    amplitude = amp;
}

void AudioInput::SetFrequency(double freq) {
    frequency = freq;
}

void AudioInput::SetAudioData(const std::vector<double>& data) {
    audio_data = data;
    has_audio_data = !data.empty();
    current_sample_idx = 0;
}

// Data Input implementation
DataInput::DataInput(InputType type, int bits) 
    : input_type(type), bit_count(bits), data_value(0), serial_bit_idx(0), 
      clock_frequency(1.0), clock_phase(false), time_per_bit(1.0) {
    analog_values.resize(std::max(bits, 1), 0.0);  // At least one connector
    if (input_type == InputType::PARALLEL) {
        analog_values.resize(bit_count, 0.0);
    } else {
        analog_values.resize(1, 0.0);  // Just the data line for serial
    }
    time_per_bit = 1.0 / clock_frequency;
}

bool DataInput::Tick() {
    if (input_type == InputType::PARALLEL) {
        // Output parallel data bits
        for (int i = 0; i < bit_count && i < static_cast<int>(analog_values.size()); i++) {
            double voltage = ((data_value >> i) & 1) ? 5.0 : 0.0;  // 5V for high, 0V for low
            analog_values[i] = voltage;
            UpdateAnalogValue(i, voltage);
        }
    } else {
        // Output serial data bits based on clock
        double time = simulation_time;
        double time_in_period = fmod(time, time_per_bit);
        
        // Toggle clock phase at the halfway point of each bit period
        bool new_clock_phase = (time_in_period < time_per_bit / 2.0) ? false : true;
        
        if (new_clock_phase != clock_phase) {
            clock_phase = new_clock_phase;
            
            // On clock rising edge, advance to next bit
            if (clock_phase && !serial_data.empty()) {
                bool bit_value = serial_data[serial_bit_idx];
                double voltage = bit_value ? 5.0 : 0.0;
                analog_values[0] = voltage;
                UpdateAnalogValue(0, voltage);
                
                serial_bit_idx = (serial_bit_idx + 1) % serial_data.size();
            }
        } else if (!serial_data.empty()) {
            // Maintain current bit value
            bool bit_value = serial_data[serial_bit_idx];
            double voltage = bit_value ? 5.0 : 0.0;
            analog_values[0] = voltage;
            UpdateAnalogValue(0, voltage);
        }
    }
    
    return true;
}

void DataInput::SetInputType(InputType type) {
    input_type = type;
}

void DataInput::SetBitCount(int bits) {
    bit_count = bits;
    if (input_type == InputType::PARALLEL) {
        analog_values.resize(bit_count, 0.0);
    }
}

void DataInput::SetDataValue(uint32_t value) {
    data_value = value;
    // Clamp to the number of available bits
    uint32_t max_value = (1 << bit_count) - 1;
    if (data_value > max_value) data_value = max_value;
}

void DataInput::SetSerialData(const std::vector<bool>& data) {
    serial_data = data;
    serial_bit_idx = 0;
}

void DataInput::SetClockFrequency(double freq) {
    clock_frequency = freq;
    if (clock_frequency > 0.0) {
        time_per_bit = 1.0 / clock_frequency;
    }
}

// External Voltage implementation
ExternalVoltage::ExternalVoltage(double initial_voltage) 
    : voltage(initial_voltage), current_sample_idx(0), use_wave_data(false), sample_rate(44100.0) {
    analog_values.resize(1, voltage);
}

bool ExternalVoltage::Tick() {
    double output_voltage = voltage;
    
    if (use_wave_data && !wave_data.empty()) {
        // Play back waveform data
        if (current_sample_idx < wave_data.size()) {
            output_voltage = wave_data[current_sample_idx];
            current_sample_idx++;
            
            // Loop back to beginning if we reach the end
            if (current_sample_idx >= wave_data.size()) {
                current_sample_idx = 0;
            }
        }
    }
    
    analog_values[0] = output_voltage;
    UpdateAnalogValue(0, output_voltage);
    
    return true;
}

void ExternalVoltage::SetVoltage(double v) {
    voltage = v;
}

void ExternalVoltage::SetExternalVoltage(double v) {
    voltage = v;
}

void ExternalVoltage::SetWaveformData(const std::vector<double>& data) {
    wave_data = data;
    current_sample_idx = 0;
    use_wave_data = !data.empty();
}

void ExternalVoltage::LoadWaveFile(const String& filename) {
    // This is a stub implementation - in a real implementation, 
    // this would load an actual wave file
    // For now, just generate a simple test signal
    
    wave_data.clear();
    // Generate 1 second of a simple test signal
    int samples = static_cast<int>(sample_rate);
    for (int i = 0; i < samples; i++) {
        double time = static_cast<double>(i) / sample_rate;
        // Simple test signal: a combination of different frequencies
        wave_data.push_back(0.5 * sin(2.0 * PI * 220.0 * time) +  // A note
                           0.3 * sin(2.0 * PI * 440.0 * time) +   // A octave
                           0.2 * sin(2.0 * PI * 110.0 * time));   // A below
    }
    
    current_sample_idx = 0;
    use_wave_data = !wave_data.empty();
}