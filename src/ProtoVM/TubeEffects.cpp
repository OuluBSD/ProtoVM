#include "TubeEffects.h"
#include <algorithm>
#include <cmath>

// TubeEffect base class implementation
TubeEffect::TubeEffect(EffectType type) 
    : effect_type(type)
    , input_signal(0.0)
    , output_signal(0.0)
    , bypass_effect(false)
    , wet_dry_mix(0.5)
    , effect_gain(1.0)
    , output_level(1.0)
    , is_enabled(true)
{
    // Initialize with basic tube for the effect
    effect_tubes.push_back(std::make_unique<Triode>());
}

TubeEffect::~TubeEffect() {
    // Cleanup handled by destructors
}

bool TubeEffect::Tick() {
    if (!is_enabled || bypass_effect) {
        output_signal = input_signal;
        return true;
    }
    
    // Process the signal through the effect
    ProcessSignal();
    
    // Apply tube characteristics
    ApplyTubeCharacteristics(output_signal);
    
    // Apply output level
    output_signal *= output_level;
    
    // Limit output to prevent clipping
    output_signal = std::max(-5.0, std::min(5.0, output_signal));
    
    // Tick all tubes used in the effect
    for (auto& tube : effect_tubes) {
        tube->Tick();
    }
    
    return true;
}

void TubeEffect::ApplyTubeCharacteristics(double& signal) {
    // Apply subtle tube characteristics to the signal
    // This adds harmonic content and slight compression
    
    if (effect_tubes.empty()) {
        return;
    }
    
    // Use the first tube to add some harmonic richness
    auto& tube = effect_tubes[0];
    tube->SetGridVoltage(-1.0 + signal * 0.1);  // Apply signal to grid
    tube->SetPlateVoltage(250.0);  // Set plate voltage
    tube->SetCathodeVoltage(0.0);
    tube->CalculateTubeBehavior();
    
    // Use the tube's plate current to modify the signal
    double plate_current = tube->GetPlateCurrent();
    double tube_effect = plate_current * 0.001;  // Scale appropriately
    
    // Add subtle even-order harmonics characteristic of tubes
    double harmonic_content = 0.02 * signal * signal * (signal > 0 ? 1 : -1) * (1.0 - wet_dry_mix);
    signal = signal * (1.0 - 0.02) + tube_effect * 0.02 + harmonic_content;
}

double TubeEffect::ApplyWetDryMix(double dry_signal, double wet_signal) {
    // Apply wet/dry mix to combine original and processed signals
    return dry_signal * (1.0 - wet_dry_mix) + wet_signal * wet_dry_mix;
}


// TubeCompressor implementation
TubeCompressor::TubeCompressor(CompressionType type) 
    : TubeEffect(EffectType::COMPRESSOR)
    , compression_type(type)
    , compression_threshold(0.5)
    , compression_ratio(4.0)
    , attack_time(0.01)  // 10ms
    , release_time(0.1)  // 100ms
    , compression_knee(0.5)  // Medium soft knee
    , makeup_gain(2.0)
    , auto_makeup(false)
    , current_gain_reduction(0.0)
    , current_level(0.0)
    , envelope_detector(0.0)
    , sidechain_filter(0.0)
{
    // Initialize with tubes configured for compression
    effect_tubes.clear();
    effect_tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // Detection stage
    effect_tubes.push_back(std::make_unique<Triode>(50.0, 4700.0, 6.0e-3));     // VCA stage
    
    // Set appropriate parameters for compression type
    switch (compression_type) {
        case CompressionType::VARI_MU:
            compression_ratio = 3.0;
            attack_time = 0.005;   // Faster attack
            release_time = 0.15;   // Medium release
            break;
        case CompressionType::LIMITING:
            compression_ratio = 20.0;  // Hard limiting
            compression_threshold = 0.3;
            attack_time = 0.001;   // Very fast attack
            release_time = 0.2;    // Slower release
            break;
        case CompressionType::PEAK_LIMITING:
            compression_ratio = 100.0; // Very hard limiting
            compression_threshold = 0.2;
            attack_time = 0.0005;  // Extremely fast attack
            release_time = 0.3;    // Slow release
            break;
        default:
            break;
    }
}

void TubeCompressor::ProcessSignal() {
    // Apply input gain
    double signal = input_signal * effect_gain;
    
    // Update compressor state based on input level
    UpdateCompressorState();
    
    // Apply compression based on calculated gain reduction
    double compressed_signal = signal;
    
    if (current_level > compression_threshold) {
        // Calculate compression ratio based on signal level and threshold
        double excess_level = current_level - compression_threshold;
        double compressed_excess = excess_level / compression_ratio;
        double gain_reduction = excess_level - compressed_excess;
        
        // Apply smooth gain reduction
        static double applied_reduction = 0.0;
        double attack_coeff = 1.0 - exp(-1.0 / (44100.0 * attack_time));
        double release_coeff = 1.0 - exp(-1.0 / (44100.0 * release_time));
        
        if (gain_reduction > applied_reduction) {
            // Attacking (signal level increased)
            applied_reduction = applied_reduction + attack_coeff * (gain_reduction - applied_reduction);
        } else {
            // Releasing (signal level decreased)
            applied_reduction = applied_reduction + release_coeff * (gain_reduction - applied_reduction);
        }
        
        // Apply the gain reduction to the signal
        compressed_signal *= exp(-applied_reduction);
        current_gain_reduction = applied_reduction;
    }
    
    // Apply makeup gain if enabled
    if (auto_makeup) {
        compressed_signal *= makeup_gain;
    } else {
        compressed_signal *= makeup_gain;  // Apply fixed makeup gain
    }
    
    // Set output signal
    output_signal = ApplyWetDryMix(input_signal, compressed_signal);
}

void TubeCompressor::UpdateCompressorState() {
    // Update the compressor's internal state based on the input signal
    current_level = abs(input_signal);
    
    // Update the envelope detector (simplified)
    static double prev_env = 0.0;
    double attack_coeff = 1.0 - exp(-1.0 / (44100.0 * attack_time));
    double release_coeff = 1.0 - exp(-1.0 / (44100.0 * release_time));
    
    if (current_level > prev_env) {
        // Attack phase
        prev_env = prev_env + attack_coeff * (current_level - prev_env);
    } else {
        // Release phase
        prev_env = prev_env + release_coeff * (current_level - prev_env);
    }
    
    envelope_detector = prev_env;
    
    // Update sidechain filter state (simplified)
    sidechain_filter = current_level;
}


// TubePhaser implementation
TubePhaser::TubePhaser(int stages) 
    : TubeEffect(EffectType::PHASER)
    , lfo_frequency(0.5)
    , lfo_amount(0.7)
    , phaser_feedback(0.0)
    , notch_count(6)
    , center_frequency(1000.0)
    , phase_depth(0.8)
    , stage_count(stages)
{
    // Initialize with tubes for VCA simulation
    effect_tubes.clear();
    for (int i = 0; i < 2; i++) {
        effect_tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // VCA simulation
    }
    
    modulation_lfo = std::make_unique<LFO>(LFOType::TRIANGLE, lfo_frequency);
    
    // Initialize allpass stages
    allpass_stages.resize(stage_count, 0.0);
    allpass_outputs.resize(stage_count, 0.0);
}

void TubePhaser::ProcessSignal() {
    // Get LFO modulation
    UpdateLFO();
    double modulation = modulation_lfo->GetOutput() * lfo_amount * phase_depth;
    
    // Apply input gain
    double signal = input_signal * effect_gain;
    
    // Process through allpass stages
    double processed_signal = signal;
    
    for (int stage = 0; stage < stage_count; stage++) {
        // Calculate modulation for this stage (stages have different center frequencies)
        double stage_modulation = modulation * (stage + 1) / stage_count;
        ProcessAllpassStage(stage, processed_signal, stage_modulation);
        
        // Apply feedback if not the last stage
        if (stage < stage_count - 1) {
            processed_signal = processed_signal * (1.0 - abs(phaser_feedback)) + 
                              allpass_outputs[stage] * phaser_feedback;
        }
    }
    
    // Mix with original signal
    output_signal = ApplyWetDryMix(signal, processed_signal);
    
    // Apply output level
    output_signal *= output_level;
}

void TubePhaser::ProcessAllpassStage(int stage, double& signal, double modulation) {
    // Simple allpass filter simulation
    // The modulation changes the delay time of the allpass filter
    
    // Calculate delay based on center frequency and modulation
    double base_delay = 1.0 / (center_frequency * (stage + 1));
    double modulated_delay = base_delay * (1.0 + modulation);
    
    // Convert to samples (simplified)
    int delay_samples = static_cast<int>(modulated_delay * 44100);
    delay_samples = std::max(1, std::min(100, delay_samples));  // Limit delay
    
    // Apply allpass filter: y[n] = x[n] - a*x[n-d] + y[n-d]
    static std::vector<double> delay_buffer(100, 0.0);
    static int buffer_pos = 0;
    
    // Calculate feedback coefficient based on delay
    double a = 0.6;  // Fixed coefficient for simplicity
    
    // Get delayed input and output
    int delay_pos = (buffer_pos - delay_samples + 100) % 100;
    
    double delayed_input = delay_buffer[delay_pos];
    double delayed_output = allpass_outputs[stage];  // Use previous output as feedback
    
    // Calculate new output
    double new_output = signal - a * delayed_input + a * delayed_output;
    
    // Store in delay buffer
    delay_buffer[buffer_pos] = signal;
    allpass_outputs[stage] = new_output;
    
    // Update signal for next stage
    signal = new_output;
    
    // Advance buffer position
    buffer_pos = (buffer_pos + 1) % 100;
}

void TubePhaser::UpdateLFO() {
    // Update the LFO for modulation
    modulation_lfo->SetFrequency(lfo_frequency);
    modulation_lfo->SetAmplitude(lfo_amount);
    modulation_lfo->Tick();
}


// TubeFlanger implementation
TubeFlanger::TubeFlanger() 
    : TubeEffect(EffectType::FLANGER)
    , lfo_frequency(0.25)
    , lfo_amount(0.7)
    , flanger_feedback(0.3)
    , delay_depth(0.002)    // 2ms depth
    , center_delay(0.001)   // 1ms center delay
{
    // Initialize with tubes for VCA simulation
    effect_tubes.clear();
    for (int i = 0; i < 2; i++) {
        effect_tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // VCA simulation
    }
    
    modulation_lfo = std::make_unique<LFO>(LFOType::TRIANGLE, lfo_frequency);
    
    // Initialize delay line
    delay_line.resize(MAX_DELAY_SIZE, 0.0);
    write_index = 0;
    read_index = 0;
    max_delay_samples = static_cast<size_t>(center_delay * 44100 + delay_depth * 44100);
}

void TubeFlanger::ProcessSignal() {
    // Get LFO modulation
    UpdateLFO();
    double modulation = modulation_lfo->GetOutput() * lfo_amount;
    
    // Apply input gain
    double signal = input_signal * effect_gain;
    
    // Calculate variable delay based on LFO
    double variable_delay = center_delay + delay_depth * modulation;
    variable_delay = std::max(0.0001, std::min(0.005, variable_delay));  // Limit delay
    
    size_t delay_samples = static_cast<size_t>(variable_delay * 44100);
    delay_samples = std::min(delay_samples, max_delay_samples);
    
    // Get delayed sample
    double delayed_signal = GetDelayedSample(delay_samples);
    
    // Apply feedback
    double feedback_signal = delayed_signal * flanger_feedback;
    
    // Mix original and delayed signals
    double processed_signal = signal + feedback_signal;
    
    // Add some harmonic content for tube character
    processed_signal += 0.05 * signal * delayed_signal;  // Subtle intermodulation
    
    // Update delay line
    delay_line[write_index] = signal + feedback_signal * 0.5;  // Include some feedback in delay line
    
    // Advance write index
    write_index = (write_index + 1) % MAX_DELAY_SIZE;
    
    // Mix with original signal
    output_signal = ApplyWetDryMix(signal, processed_signal);
}

void TubeFlanger::UpdateLFO() {
    // Update the LFO for modulation
    modulation_lfo->SetFrequency(lfo_frequency);
    modulation_lfo->SetAmplitude(lfo_amount);
    modulation_lfo->Tick();
}

double TubeFlanger::GetDelayedSample(size_t delay_samples) {
    if (delay_samples >= MAX_DELAY_SIZE) {
        delay_samples = MAX_DELAY_SIZE - 1;
    }
    
    size_t read_pos = (write_index + MAX_DELAY_SIZE - delay_samples) % MAX_DELAY_SIZE;
    return delay_line[read_pos];
}


// TubeChorus implementation
TubeChorus::TubeChorus(int voices) 
    : TubeEffect(EffectType::CHORUS)
    , lfo_frequency(1.0)
    , lfo_amount(0.3)
    , delay_depth(0.002)    // 2ms depth
    , voice_count(voices)
    , detune_depth(0.1)     // 10% detune between voices
{
    // Initialize with tubes for VCA simulation
    effect_tubes.clear();
    for (int i = 0; i < 2; i++) {
        effect_tubes.push_back(std::make_unique<Triode>(100.0, 100000.0, 1.6e-3));  // VCA simulation
    }
    
    // Create LFOs for each voice
    for (int i = 0; i < voice_count; i++) {
        // Slightly different frequencies for each voice
        double freq = lfo_frequency * (1.0 + i * detune_depth / voice_count);
        modulation_lfos.push_back(std::make_unique<LFO>(LFOType::SINE, freq));
    }
    
    // Initialize delay lines for each voice
    delay_lines.resize(voice_count);
    write_indices.resize(voice_count, 0);
    read_indices.resize(voice_count, 0);
    
    for (int i = 0; i < voice_count; i++) {
        delay_lines[i].resize(MAX_DELAY_SIZE, 0.0);
    }
    
    max_delay_samples = static_cast<size_t>(delay_depth * 44100);
}

void TubeChorus::ProcessSignal() {
    // Update all LFOs
    UpdateLFOs();
    
    // Apply input gain
    double signal = input_signal * effect_gain;
    
    // Process each voice
    double mixed_signal = 0.0;
    
    for (int voice = 0; voice < voice_count; voice++) {
        // Get LFO modulation for this voice
        double modulation = modulation_lfos[voice]->GetOutput() * lfo_amount;
        
        // Calculate variable delay based on LFO
        double variable_delay = delay_depth * (0.5 + 0.5 * modulation);
        variable_delay = std::max(0.0001, std::min(0.005, variable_delay));  // Limit delay
        
        size_t delay_samples = static_cast<size_t>(variable_delay * 44100);
        delay_samples = std::min(delay_samples, max_delay_samples);
        
        // Get delayed sample for this voice
        double delayed_signal = GetDelayedSample(voice, delay_samples);
        
        // Add to mixed signal
        mixed_signal += delayed_signal;
    }
    
    // Average the voices
    mixed_signal /= voice_count;
    
    // Mix with original signal
    output_signal = ApplyWetDryMix(signal, mixed_signal);
}

void TubeChorus::UpdateLFOs() {
    // Update all LFOs for chorus voices
    for (int i = 0; i < voice_count; i++) {
        // Slightly detune each LFO
        double detune_factor = 1.0 + i * detune_depth / voice_count;
        modulation_lfos[i]->SetFrequency(lfo_frequency * detune_factor);
        modulation_lfos[i]->SetAmplitude(lfo_amount);
        modulation_lfos[i]->Tick();
    }
}

double TubeChorus::GetDelayedSample(int voice, size_t delay_samples) {
    if (delay_samples >= MAX_DELAY_SIZE) {
        delay_samples = MAX_DELAY_SIZE - 1;
    }
    
    size_t read_pos = (write_indices[voice] + MAX_DELAY_SIZE - delay_samples) % MAX_DELAY_SIZE;
    return delay_lines[voice][read_pos];
}