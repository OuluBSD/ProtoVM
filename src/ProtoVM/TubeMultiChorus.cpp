#include "TubeMultiChorus.h"
#include "Common.h"
#include <algorithm>
#include <cmath>

// TubeMultiChorus implementation
TubeMultiChorus::TubeMultiChorus(MultiChorusType type, int voice_count)
    : chorus_type(type)
    , lfo_frequency(0.5)
    , lfo_amount(0.7)
    , delay_depth(0.002)    // 2ms depth
    , voice_count(std::min(voice_count, MAX_VOICES))
    , detune_depth(0.1)     // 10% detune between voices
    , feedback(0.3)
    , spread(0.8)
    , constellation_size(8)
    , left_output(0.0)
    , right_output(0.0)
    , input_signal(0.0)
    , current_time(0.0)
    , tube_characteristics(0.3)
{
    InitializeChorus(type);
    
    // Initialize with tubes for harmonics
    for (int i = 0; i < 2; i++) {
        tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Tube simulation
    }
    
    // Create LFOs for each voice with different characteristics
    for (int i = 0; i < voice_count; i++) {
        // Use different LFO frequencies for each voice to create the phase-constellation effect
        double freq = lfo_frequency * (1.0 + 0.1 * sin(i * 2.0 * M_PI / constellation_size));
        modulation_lfos.push_back(std::make_unique<LFO>(LFOType::SINE, freq));
    }
    
    // Initialize delay lines for each voice
    delay_lines.resize(voice_count);
    write_indices.resize(voice_count, 0);
    allpass_delays.resize(voice_count);
    allpass_outputs.resize(voice_count);
    
    for (int i = 0; i < voice_count; i++) {
        delay_lines[i].resize(MAX_DELAY_SIZE, 0.0);
        allpass_delays[i].resize(4, 0.0);  // 4-stage allpass filter
        allpass_outputs[i].resize(4, 0.0);
    }
}

TubeMultiChorus::~TubeMultiChorus() {
    // Destructor handled by member destructors
}

void TubeMultiChorus::InitializeChorus(MultiChorusType type) {
    switch (type) {
        case PHASE_CONSTELLATION_CHORUS:
            lfo_frequency = 0.6;
            lfo_amount = 0.8;
            delay_depth = 0.003;
            detune_depth = 0.15;
            voice_count = std::min(voice_count, 8);
            constellation_size = 8;
            feedback = 0.1;
            break;
            
        case VINTAGE_JET_STREAM_CHORUS:
            lfo_frequency = 0.8;
            lfo_amount = 0.6;
            delay_depth = 0.0025;
            detune_depth = 0.12;
            voice_count = std::min(voice_count, 6);
            constellation_size = 6;
            feedback = 0.2;
            break;
            
        case MODERN_GLASS_CHORUS:
            lfo_frequency = 0.4;
            lfo_amount = 0.9;
            delay_depth = 0.004;
            detune_depth = 0.08;
            voice_count = std::min(voice_count, 12);
            constellation_size = 12;
            feedback = 0.05;
            break;
            
        case STEREO_FIELD_CHORUS:
            lfo_frequency = 0.7;
            lfo_amount = 0.75;
            delay_depth = 0.003;
            detune_depth = 0.1;
            voice_count = std::min(voice_count, 8);
            constellation_size = 8;
            feedback = 0.15;
            spread = 1.0;
            break;
    }
}

bool TubeMultiChorus::Tick() {
    // This would be called for each audio sample
    ProcessSignal();
    return true;
}

void TubeMultiChorus::ProcessSignal() {
    // Update all LFOs
    UpdateLFOs();

    // Apply input signal
    double signal = input_signal;

    // Process each voice through delay line and allpass filters
    double mixed_signal_left = 0.0;
    double mixed_signal_right = 0.0;

    for (int voice = 0; voice < voice_count; voice++) {
        // Get LFO modulation for this voice
        double modulation = modulation_lfos[voice]->GetOutput() * lfo_amount;

        // Calculate variable delay based on LFO and voice position in constellation
        double phase_offset = (voice * 2.0 * M_PI) / constellation_size;
        double variable_delay = delay_depth * (0.5 + 0.5 * modulation * cos(phase_offset));
        variable_delay = std::max(MIN_DELAY_DEPTH, std::min(MAX_DELAY_DEPTH, variable_delay));  // Limit delay

        size_t delay_samples = static_cast<size_t>(variable_delay * 44100);
        delay_samples = std::min(delay_samples, static_cast<size_t>(MAX_DELAY_SIZE - 1));

        // Get delayed sample for this voice through allpass filtering for phase constellation
        double delayed_signal = GetDelayedSample(voice, delay_samples);

        // Apply allpass filtering to create phase constellation effect
        double allpass_coeff = 0.6 * (1.0 + 0.2 * cos(phase_offset + current_time));
        for (int stage = 0; stage < 4; stage++) {
            double input = delayed_signal;
            double output = -allpass_coeff * input + allpass_delays[voice][stage] + allpass_coeff * allpass_outputs[voice][stage];
            allpass_delays[voice][stage] = input;
            allpass_outputs[voice][stage] = output;
            delayed_signal = output;
        }

        // Calculate stereo positioning based on voice position in constellation
        double angle = (voice * 2.0 * M_PI) / voice_count;
        double left_gain = cos(angle) * 0.5 + 0.5;
        double right_gain = sin(angle) * 0.5 + 0.5;
        
        // Apply stereo spread control
        left_gain = left_gain * spread + (1.0 - spread) * 0.5;
        right_gain = right_gain * spread + (1.0 - spread) * 0.5;

        // Mix with original signal
        mixed_signal_left += delayed_signal * left_gain;
        mixed_signal_right += delayed_signal * right_gain;
    }

    // Apply feedback if enabled
    if (feedback != 0.0) {
        double feedback_signal = (mixed_signal_left + mixed_signal_right) * 0.5 * feedback;
        input_signal += feedback_signal;
    }

    // Apply tube characteristics to add harmonic content
    ApplyTubeCharacteristics();

    // Average the voices and apply to outputs
    left_output = mixed_signal_left / voice_count;
    right_output = mixed_signal_right / voice_count;
    
    // Update current time for phase calculations
    current_time += 1.0/44100.0;
}

double TubeMultiChorus::GetDelayedSample(int voice, size_t delay_samples) {
    if (delay_samples >= MAX_DELAY_SIZE) {
        delay_samples = MAX_DELAY_SIZE - 1;
    }

    size_t read_pos = (write_indices[voice] + MAX_DELAY_SIZE - delay_samples) % MAX_DELAY_SIZE;
    return delay_lines[voice][read_pos];
}

void TubeMultiChorus::UpdateLFOs() {
    // Update all LFOs for chorus voices with detuning
    for (int i = 0; i < voice_count; i++) {
        // Slightly detune each LFO based on constellation position
        double detune_factor = 1.0 + detune_depth * sin(i * 2.0 * M_PI / constellation_size);
        modulation_lfos[i]->SetFrequency(lfo_frequency * detune_factor);
        modulation_lfos[i]->SetAmplitude(lfo_amount);
        modulation_lfos[i]->Tick();
    }
}

void TubeMultiChorus::ApplyTubeCharacteristics() {
    // Apply subtle tube characteristics to the mixed signal
    if (tubes.empty()) {
        return;
    }

    // Use the first tube to add some harmonic richness
    auto& tube = tubes[0];
    tube->SetGridVoltage(-1.0 + (left_output + right_output) * 0.05);  // Apply signal to grid
    tube->SetPlateVoltage(250.0);  // Set plate voltage
    tube->SetCathodeVoltage(0.0);
    tube->CalculateTubeBehavior();

    // Use the tube's plate current to modify the signal
    double plate_current = tube->GetPlateCurrent();
    double tube_effect = plate_current * 0.001;  // Scale appropriately

    // Add subtle even-order harmonics characteristic of tubes
    double harmonic_content = 0.02 * (left_output + right_output) * 
                             (left_output + right_output) * 
                             ((left_output + right_output) > 0 ? 1 : -1);
    
    left_output = left_output * (1.0 - tube_characteristics * 0.5) + 
                  tube_effect * tube_characteristics * 0.5 + 
                  harmonic_content * tube_characteristics * 0.5;
                  
    right_output = right_output * (1.0 - tube_characteristics * 0.5) + 
                   tube_effect * tube_characteristics * 0.5 + 
                   harmonic_content * tube_characteristics * 0.5;
}

void TubeMultiChorus::SetLFOFrequency(double freq) {
    lfo_frequency = std::max(0.1, std::min(10.0, freq));
}

void TubeMultiChorus::SetLFOAmount(double amount) {
    lfo_amount = std::max(0.0, std::min(1.0, amount));
}

void TubeMultiChorus::SetDelayDepth(double depth) {
    delay_depth = std::max(MIN_DELAY_DEPTH, std::min(MAX_DELAY_DEPTH, depth));
}

void TubeMultiChorus::SetVoiceCount(int count) {
    voice_count = std::max(2, std::min(MAX_VOICES, count));
    // Need to resize arrays accordingly, but for simplicity in this implementation
    // we'll just limit the processing to the new count
}

void TubeMultiChorus::SetDetuneDepth(double detune) {
    detune_depth = std::max(MIN_DETUNE_DEPTH, std::min(MAX_DETUNE_DEPTH, detune));
}

void TubeMultiChorus::SetFeedback(double feedback) {
    this->feedback = std::max(MIN_FEEDBACK, std::min(MAX_FEEDBACK, feedback));
}

void TubeMultiChorus::SetSpread(double spread) {
    this->spread = std::max(MIN_SPREAD, std::min(MAX_SPREAD, spread));
}

void TubeMultiChorus::SetConstellationSize(int size) {
    constellation_size = std::max(4, std::min(16, size));
}