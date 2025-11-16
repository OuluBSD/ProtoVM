#include "TubeTapeEcho.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeTapeEcho implementation
TubeTapeEcho::TubeTapeEcho(TapeEchoType type)
    : echo_type(type)
    , delay_time(0.3)           // 300ms default delay
    , feedback(0.4)             // 40% feedback
    , tape_saturation(0.5)      // Medium saturation
    , wow_flutter(0.02)         // 2% wow/flutter
    , head_distance(2.0)        // 2cm head distance
    , tape_speed(7.5)           // 7.5 ips (common speed)
    , low_pass_freq(4000.0)     // 4kHz low-pass
    , high_pass_freq(100.0)     // 100Hz high-pass
    , stereo_width(1.0)         // Normal stereo width
    , write_pos(0)
    , read_pos_left(0)
    , read_pos_right(0)
    , delay_line_size(0)
    , input_signal_left(0.0)
    , input_signal_right(0.0)
    , output_signal_left(0.0)
    , output_signal_right(0.0)
    , modulation_phase(0.0)
    , tube_characteristics(0.4)
{
    InitializeEcho(type);
    
    // Initialize with tubes for tape saturation simulation
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }
    
    // Calculate delay buffer size based on max delay time
    delay_line_size = MAX_DELAY_SIZE;
    delay_line_left.resize(delay_line_size, 0.0);
    delay_line_right.resize(delay_line_size, 0.0);
    
    // Initialize positions
    size_t initial_delay = static_cast<size_t>(delay_time * 44100);
    read_pos_left = (write_pos + delay_line_size - initial_delay) % delay_line_size;
    read_pos_right = (write_pos + delay_line_size - initial_delay) % delay_line_size;
}

TubeTapeEcho::~TubeTapeEcho() {
    // Destructor handled by member destructors
}

void TubeTapeEcho::InitializeEcho(TapeEchoType type) {
    switch (type) {
        case ECHORECORDER_TAPE_ECHO:
            delay_time = 0.3;
            feedback = 0.4;
            tape_saturation = 0.6;
            wow_flutter = 0.03;
            head_distance = 1.5;
            tape_speed = 7.5;
            low_pass_freq = 3500.0;
            high_pass_freq = 120.0;
            stereo_width = 0.8;
            break;
            
        case SPACE_ECHO:
            delay_time = 0.15;
            feedback = 0.3;
            tape_saturation = 0.4;
            wow_flutter = 0.01;
            head_distance = 0.8;
            tape_speed = 3.75;
            low_pass_freq = 5000.0;
            high_pass_freq = 80.0;
            stereo_width = 1.2;
            break;
            
        case MAGNETIC_TAPE_DELAY:
            delay_time = 0.6;
            feedback = 0.5;
            tape_saturation = 0.7;
            wow_flutter = 0.04;
            head_distance = 2.5;
            tape_speed = 15.0;
            low_pass_freq = 3000.0;
            high_pass_freq = 150.0;
            stereo_width = 1.0;
            break;
            
        case VINTAGE_ANALOG_DELAY:
            delay_time = 0.25;
            feedback = 0.35;
            tape_saturation = 0.5;
            wow_flutter = 0.02;
            head_distance = 1.8;
            tape_speed = 7.5;
            low_pass_freq = 4000.0;
            high_pass_freq = 100.0;
            stereo_width = 0.9;
            break;
    }
}

bool TubeTapeEcho::Tick() {
    // This would be called for each audio sample
    ProcessSignal();
    return true;
}

void TubeTapeEcho::ProcessSignal() {
    // Apply wow and flutter modulation to delay time
    ApplyWowAndFlutter();
    
    // Calculate variable delay positions
    size_t delay_samples = static_cast<size_t>(delay_time * 44100);
    read_pos_left = (write_pos + delay_line_size - delay_samples) % delay_line_size;
    read_pos_right = (read_pos_left + static_cast<size_t>((delay_time * 0.1 * stereo_width * 44100) / 2)) % delay_line_size;
    
    // Get delayed samples
    double delayed_left = delay_line_left[read_pos_left];
    double delayed_right = delay_line_right[read_pos_right];
    
    // Apply tape characteristics to delayed signal
    double processed_left = delayed_left;
    double processed_right = delayed_right;
    
    ApplyTapeCharacteristics();
    ApplyFilters();
    
    // Mix input with feedback
    double mixed_left = input_signal_left + processed_left * feedback;
    double mixed_right = input_signal_right + processed_right * feedback;
    
    // Store mixed signals in delay line
    delay_line_left[write_pos] = mixed_left;
    delay_line_right[write_pos] = mixed_right;
    
    // Apply final tape processing to output
    output_signal_left = processed_left * (1.0 - feedback) + input_signal_left * feedback * 0.5;
    output_signal_right = processed_right * (1.0 - feedback) + input_signal_right * feedback * 0.5;
    
    // Advance write position
    write_pos = (write_pos + 1) % delay_line_size;
    
    // Update modulation phase for wow/flutter
    modulation_phase += 0.001; // Very slow modulation for wow effect
}

void TubeTapeEcho::ApplyWowAndFlutter() {
    // Apply low-frequency modulation to simulate wow (slow variation)
    // and flutter (fast variation) in tape speed
    double wow = 0.5 * sin(modulation_phase * 0.1) * wow_flutter;
    double flutter = 0.3 * sin(modulation_phase * 50.0) * wow_flutter;
    double total_modulation = wow + flutter;
    
    // Apply modulation to delay time
    delay_time *= (1.0 + total_modulation);
    
    // Keep delay_time within bounds
    delay_time = std::max(MIN_DELAY_TIME, std::min(MAX_DELAY_TIME, delay_time));
}

void TubeTapeEcho::ApplyTapeCharacteristics() {
    // Apply tape saturation using tanh function to softly clip
    if (tape_saturation > 0.0) {
        double saturation_factor = 2.0 + tape_saturation * 3.0; // 2.0 to 5.0 range
        output_signal_left = tanh(output_signal_left * saturation_factor) / saturation_factor;
        output_signal_right = tanh(output_signal_right * saturation_factor) / saturation_factor;
    }
    
    // Apply tube characteristics to add harmonic content
    if (!tubes.empty()) {
        // Use the first tube to add some harmonic richness
        auto& tube = tubes[0];
        tube->SetGridVoltage(-1.0 + (output_signal_left + output_signal_right) * 0.05);  // Apply signal to grid
        tube->SetPlateVoltage(250.0);  // Set plate voltage
        tube->SetCathodeVoltage(0.0);
        tube->CalculateTubeBehavior();

        // Use the tube's plate current to modify the signal
        double plate_current = tube->GetPlateCurrent();
        double tube_effect = plate_current * 0.001;  // Scale appropriately

        // Mix in tube characteristics
        output_signal_left = output_signal_left * (1.0 - tube_characteristics * 0.5) + 
                            tube_effect * tube_characteristics * 0.5;
        output_signal_right = output_signal_right * (1.0 - tube_characteristics * 0.5) + 
                             tube_effect * tube_characteristics * 0.5;
    }
}

void TubeTapeEcho::ApplyFilters() {
    // Apply low-pass filter to simulate tape's frequency response
    // Using simple first-order IIR filter
    static double lp_state_left = 0.0;
    static double lp_state_right = 0.0;
    
    // Calculate coefficients based on frequency
    double dt = 1.0 / 44100.0;
    double rc = 1.0 / (2.0 * M_PI * low_pass_freq);
    double lp_coeff = dt / (rc + dt);
    
    output_signal_left = lp_state_left + lp_coeff * (output_signal_left - lp_state_left);
    output_signal_right = lp_state_right + lp_coeff * (output_signal_right - lp_state_right);
    
    lp_state_left = output_signal_left;
    lp_state_right = output_signal_right;
    
    // Apply high-pass filter to remove DC and low frequencies
    static double hp_state_in_left = 0.0;
    static double hp_state_out_left = 0.0;
    static double hp_state_in_right = 0.0;
    static double hp_state_out_right = 0.0;
    
    rc = 1.0 / (2.0 * M_PI * high_pass_freq);
    double hp_coeff = rc / (rc + dt);
    
    double hp_input_left = output_signal_left;
    output_signal_left = hp_coeff * (output_signal_left + hp_state_out_left - hp_state_in_left);
    hp_state_in_left = hp_input_left;
    hp_state_out_left = output_signal_left;
    
    double hp_input_right = output_signal_right;
    output_signal_right = hp_coeff * (output_signal_right + hp_state_out_right - hp_state_in_right);
    hp_state_in_right = hp_input_right;
    hp_state_out_right = output_signal_right;
}

void TubeTapeEcho::SetDelayTime(double time) {
    delay_time = std::max(MIN_DELAY_TIME, std::min(MAX_DELAY_TIME, time));
}

void TubeTapeEcho::SetFeedback(double feedback) {
    this->feedback = std::max(MIN_FEEDBACK, std::min(MAX_FEEDBACK, feedback));
}

void TubeTapeEcho::SetTapeSaturation(double saturation) {
    tape_saturation = std::max(MIN_TAPE_SATURATION, std::min(MAX_TAPE_SATURATION, saturation));
}

void TubeTapeEcho::SetWowAndFlutter(double wow_flutter) {
    this->wow_flutter = std::max(MIN_WOW_FLUTTER, std::min(MAX_WOW_FLUTTER, wow_flutter));
}

void TubeTapeEcho::SetHeadDistance(double distance) {
    head_distance = std::max(MIN_HEAD_DISTANCE, std::min(MAX_HEAD_DISTANCE, distance));
}

void TubeTapeEcho::SetTapeSpeed(double speed) {
    tape_speed = std::max(MIN_TAPE_SPEED, std::min(MAX_TAPE_SPEED, speed));
    
    // Update delay time based on head distance and tape speed
    // delay_time = head_distance / (tape_speed * 2.54) // Convert ips to cm/s
    // But we'll keep delay_time as the primary control and use these parameters for other effects
}

void TubeTapeEcho::SetLowPassFilterFreq(double freq) {
    low_pass_freq = std::max(MIN_LOWPASS_FREQ, std::min(MAX_LOWPASS_FREQ, freq));
}

void TubeTapeEcho::SetHighPassFilterFreq(double freq) {
    high_pass_freq = std::max(MIN_HIGHPASS_FREQ, std::min(MAX_HIGHPASS_FREQ, freq));
}

void TubeTapeEcho::SetStereoWidth(double width) {
    stereo_width = std::max(MIN_STEREO_WIDTH, std::min(MAX_STEREO_WIDTH, width));
}