#include "ExamplePatches.h"

ExamplePatches::ExamplePatches() {
    // Initialize the list of available patches
    available_patches.push_back(PatchInfo("Warm Pad", "Rich, evolving pad sound with slow attack and release", 
                                         PatchCategory::PAD, SynthArchitecture::SUBTRACTIVE));
    available_patches.push_back(PatchInfo("Sharp Lead", "Bright, cutting lead sound with resonance", 
                                         PatchCategory::LEAD, SynthArchitecture::SUBTRACTIVE));
    available_patches.push_back(PatchInfo("Analog Bass", "Warm, round bass sound with filter envelope", 
                                         PatchCategory::BASS, SynthArchitecture::SUBTRACTIVE));
    available_patches.push_back(PatchInfo("Bell Sound", "Bright, metallic bell-like tone", 
                                         PatchCategory::FX, SynthArchitecture::FM));
    available_patches.push_back(PatchInfo("Brass Section", "Rich, powerful brass ensemble sound", 
                                         PatchCategory::BRASS, SynthArchitecture::SUBTRACTIVE));
    available_patches.push_back(PatchInfo("Chiptune Sound", "Retro 8-bit style square wave sound", 
                                         PatchCategory::FX, SynthArchitecture::SUBTRACTIVE));
    available_patches.push_back(PatchInfo("FM Electric Piano", "Classic electric piano sound using FM synthesis", 
                                         PatchCategory::PLUCKED, SynthArchitecture::FM));
    available_patches.push_back(PatchInfo("WaveTable Saw", "Rich sawtooth waveform with morphing", 
                                         PatchCategory::LEAD, SynthArchitecture::WAVE_TABLE));
    available_patches.push_back(PatchInfo("Additive Harmonics", "Pure tone built from harmonic series", 
                                         PatchCategory::PAD, SynthArchitecture::ADDITIVE));
}

ExamplePatches::~ExamplePatches() {
    // Cleanup if needed
}

void ExamplePatches::CreateAllExamplePatches(PresetManager& preset_manager) {
    CreateSubtractivePatches(preset_manager);
    CreateFMPatches(preset_manager);
    CreateWaveTablePatches(preset_manager);
    CreateAdditivePatches(preset_manager);
}

void ExamplePatches::CreateSubtractivePatches(PresetManager& preset_manager) {
    CreateWarmPad(preset_manager);
    CreateSharpLead(preset_manager);
    CreateBassPatch(preset_manager);
    CreateBellSound(preset_manager);  // Though this could also be FM
    CreateBrassSection(preset_manager);
    CreateAnalogBass(preset_manager);
    CreateChiptuneSound(preset_manager);
}

void ExamplePatches::CreateFMPatches(PresetManager& preset_manager) {
    CreateFMBell(preset_manager);
    CreateFMBrass(preset_manager);
    CreateFMElectricPiano(preset_manager);
}

void ExamplePatches::CreateWaveTablePatches(PresetManager& preset_manager) {
    CreateWaveTableSaw(preset_manager);
    CreateWaveTableSquare(preset_manager);
    // CreateWaveTableSync(preset_manager);  // Advanced feature
}

void ExamplePatches::CreateAdditivePatches(PresetManager& preset_manager) {
    CreateAdditiveHarmonic(preset_manager);
    CreateAdditiveBell(preset_manager);
    // CreateAdditiveFormant(preset_manager);  // Advanced feature
}

void ExamplePatches::CreateWarmPad(PresetManager& preset_manager) {
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // VCO settings - use sawtooth waveform for rich harmonics
    if (!params.vco_params.empty()) {
        params.vco_params[0].waveform_type = static_cast<int>(VCOType::SAWTOOTH);
        params.vco_params[0].amplitude = 0.8;
    }
    
    // VCF settings - open filter with some resonance for warmth
    params.vcf_params.cutoff_freq = 1500.0;
    params.vcf_params.resonance = 0.4;
    
    // ADSR settings - slow attack and release for pad characteristic
    ApplyADSRToParams(params, 1.2, 0.3, 0.7, 1.0);
    
    // Add modulation - LFO to filter for subtle movement
    AddModulationConnection(params, ModulationSource::LFO1, 
                           ModulationDestination::VCF_CUTOFF, 0.1, "LFO Filter Mod");
    
    // Add another modulation - ADSR to VCA for amplitude envelope
    AddModulationConnection(params, ModulationSource::ADSR1, 
                           ModulationDestination::VCA_LEVEL, 1.0, "ADSR VCA Mod");
    
    params.name = "Warm Pad";
    params.description = "Rich, evolving pad sound with slow attack and release";
    params.category = static_cast<int>(PatchCategory::PAD);
    
    preset_manager.CreatePreset(params, "Warm Pad");
}

void ExamplePatches::CreateSharpLead(PresetManager& preset_manager) {
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // VCO settings - use sawtooth or square for bright harmonics
    if (!params.vco_params.empty()) {
        params.vco_params[0].waveform_type = static_cast<int>(VCOType::SAWTOOTH);
        params.vco_params[0].amplitude = 0.9;
    }
    
    // VCF settings - bright with high resonance for cutting sound
    params.vcf_params.cutoff_freq = 3000.0;
    params.vcf_params.resonance = 0.7;
    params.vcf_params.env_amount = 0.8;  // High envelope to filter
    
    // ADSR settings - fast attack, medium decay, high sustain
    ApplyADSRToParams(params, 0.02, 0.3, 0.8, 0.4);
    
    // Add modulation - pitch bend for expression
    AddModulationConnection(params, ModulationSource::WHEEL, 
                           ModulationDestination::VCO1_PITCH, 0.1, "Pitch Wheel Mod");
    
    // Add filter modulation from ADSR
    AddModulationConnection(params, ModulationSource::ADSR1, 
                           ModulationDestination::VCF_CUTOFF, 0.6, "ADSR Filter Mod");
    
    params.name = "Sharp Lead";
    params.description = "Bright, cutting lead sound with resonance";
    params.category = static_cast<int>(PatchCategory::LEAD);
    
    preset_manager.CreatePreset(params, "Sharp Lead");
}

void ExamplePatches::CreateBassPatch(PresetManager& preset_manager) {
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // VCO settings - use square wave or sawtooth for bass
    if (!params.vco_params.empty()) {
        params.vco_params[0].waveform_type = static_cast<int>(VCOType::SQUARE);
        params.vco_params[0].amplitude = 0.85;
    }
    
    // VCF settings - low cutoff to emphasize bass frequencies
    params.vcf_params.cutoff_freq = 800.0;
    params.vcf_params.resonance = 0.3;
    
    // ADSR settings - quick attack, fast decay, low sustain, quick release
    ApplyADSRToParams(params, 0.01, 0.2, 0.3, 0.2);
    
    // Add modulation - velocity to amplitude for dynamics
    AddModulationConnection(params, ModulationSource::VELOCITY, 
                           ModulationDestination::VCA_LEVEL, 0.3, "Velocity Level Mod");
    
    params.name = "Bass Patch";
    params.description = "Punchy bass sound";
    params.category = static_cast<int>(PatchCategory::BASS);
    
    preset_manager.CreatePreset(params, "Bass Patch");
}

void ExamplePatches::CreateBellSound(PresetManager& preset_manager) {
    PatchParameters params = CreateFMBaseParams();
    
    // FM Operator settings for bell-like timbre
    // Operator 0 (modulator) - higher frequency ratio, medium level
    if (!params.vco_params.empty() && params.vco_params.size() > 0) {
        params.vco_params[0].frequency = 440.0 * 2.0;  // Harmonic 2
        params.vco_params[0].amplitude = 0.7;
    }
    
    // Operator 1 (carrier) - fundamental frequency, higher level
    if (params.vco_params.size() > 1) {
        params.vco_params[1].frequency = 440.0;  // Fundamental
        params.vco_params[1].amplitude = 0.9;
    }
    
    // Add more operators for complexity
    if (params.vco_params.size() > 2) {
        params.vco_params[2].frequency = 440.0 * 3.1;  // Inharmonic partial
        params.vco_params[2].amplitude = 0.5;
    }
    
    if (params.vco_params.size() > 3) {
        params.vco_params[3].frequency = 440.0 * 4.5;  // Another inharmonic partial
        params.vco_params[3].amplitude = 0.3;
    }
    
    // ADSR settings - quick attack, long decay for bell characteristic
    ApplyADSRToParams(params, 0.01, 1.5, 0.0, 1.0);
    
    params.name = "Bell Sound";
    params.description = "Bright, metallic bell-like tone";
    params.category = static_cast<int>(PatchCategory::FX);
    
    preset_manager.CreatePreset(params, "Bell Sound");
}

void ExamplePatches::CreateStringSound(PresetManager& preset_manager) {
    // This would be similar to the warm pad but with different characteristics
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // VCO settings - use sawtooth waveform which is similar to string harmonics
    if (!params.vco_params.empty()) {
        params.vco_params[0].waveform_type = static_cast<int>(VCOType::SAWTOOTH);
        params.vco_params[0].amplitude = 0.75;
    }
    
    // VCF settings - moderate filtering
    params.vcf_params.cutoff_freq = 1800.0;
    params.vcf_params.resonance = 0.2;
    
    // ADSR settings - medium attack for string plucking characteristic
    ApplyADSRToParams(params, 0.3, 0.4, 0.8, 0.6);
    
    // Add subtle modulation
    AddModulationConnection(params, ModulationSource::LFO1, 
                           ModulationDestination::VCO1_PITCH, 0.02, "Subtle Vibrato");
    
    params.name = "String Sound";
    params.description = "Warm string ensemble sound";
    params.category = static_cast<int>(PatchCategory::STRING);
    
    preset_manager.CreatePreset(params, "String Sound");
}

void ExamplePatches::CreateBrassSection(PresetManager& preset_manager) {
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // VCO settings - use sawtooth for brass harmonics, add second VCO slightly detuned
    if (params.vco_params.size() > 1) {
        params.vco_params[0].waveform_type = static_cast<int>(VCOType::SAWTOOTH);
        params.vco_params[0].amplitude = 0.8;
        params.vco_params[1].waveform_type = static_cast<int>(VCOType::SAWTOOTH);
        params.vco_params[1].amplitude = 0.75;
        // Slightly detune the second oscillator
        params.vco_params[1].fm_amount = 0.05;  // Small frequency offset
    }
    
    // VCF settings - bright with some resonance
    params.vcf_params.cutoff_freq = 2500.0;
    params.vcf_params.resonance = 0.5;
    
    // ADSR settings - medium attack, high sustain
    ApplyADSRToParams(params, 0.1, 0.3, 0.9, 0.4);
    
    // Add modulation - breath control
    AddModulationConnection(params, ModulationSource::AFTERTOUCH, 
                           ModulationDestination::VCF_CUTOFF, 0.3, "Aftertouch Filter");
    
    params.name = "Brass Section";
    params.description = "Rich, powerful brass ensemble sound";
    params.category = static_cast<int>(PatchCategory::BRASS);
    
    preset_manager.CreatePreset(params, "Brass Section");
}

void ExamplePatches::CreateAnalogBass(PresetManager& preset_manager) {
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // VCO settings - use sawtooth with square mix for analog character
    if (!params.vco_params.empty()) {
        params.vco_params[0].waveform_type = static_cast<int>(VCOType::SAWTOOTH);
        params.vco_params[0].amplitude = 0.9;
    }
    
    // Add sub-oscillator for low end (if available)
    if (params.vco_params.size() > 1) {
        params.vco_params[1].waveform_type = static_cast<int>(VCOType::SQUARE);
        params.vco_params[1].frequency = 440.0 / 2.0;  // One octave down
        params.vco_params[1].amplitude = 0.4;  // Subtle sub-oscillator
    }
    
    // VCF settings - low cutoff, high resonance for analog bass character
    params.vcf_params.cutoff_freq = 1200.0;
    params.vcf_params.resonance = 0.8;
    params.vcf_params.env_amount = 0.7;  // Strong envelope modulation
    
    // ADSR settings - punchy
    ApplyADSRToParams(params, 0.01, 0.2, 0.5, 0.3);
    
    params.name = "Analog Bass";
    params.description = "Warm, round bass sound with filter envelope";
    params.category = static_cast<int>(PatchCategory::BASS);
    
    preset_manager.CreatePreset(params, "Analog Bass");
}

void ExamplePatches::CreateVocoderEffect(PresetManager& preset_manager) {
    // This would be a more advanced patch with multiple filters controlled by an envelope follower
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // Use multiple bandpass filters to create formant-like characteristics
    params.vcf_params.cutoff_freq = 1000.0;  // Formant 1
    params.vcf_params.resonance = 0.9;      // High Q for formant clarity
    
    // ADSR settings for speech-like envelope
    ApplyADSRToParams(params, 0.01, 0.1, 0.8, 0.2);
    
    params.name = "Vocoder Effect";
    params.description = "Speech-like formant filter effect";
    params.category = static_cast<int>(PatchCategory::FX);
    
    preset_manager.CreatePreset(params, "Vocoder Effect");
}

void ExamplePatches::CreateChiptuneSound(PresetManager& preset_manager) {
    PatchParameters params = CreateSubtractiveBaseParams();
    
    // VCO settings - use square wave for classic 8-bit sound
    if (!params.vco_params.empty()) {
        params.vco_params[0].waveform_type = static_cast<int>(VCOType::SQUARE);
        params.vco_params[0].amplitude = 0.8;
        params.vco_params[0].pwm_duty_cycle = 0.25;  // 25% duty cycle
    }
    
    // VCF settings - minimal filtering to maintain harsh square wave character
    params.vcf_params.cutoff_freq = 4000.0;
    params.vcf_params.resonance = 0.1;
    
    // ADSR settings - quick for chiptune style
    ApplyADSRToParams(params, 0.01, 0.3, 0.5, 0.4);
    
    params.name = "Chiptune Sound";
    params.description = "Retro 8-bit style square wave sound";
    params.category = static_cast<int>(PatchCategory::FX);
    
    preset_manager.CreatePreset(params, "Chiptune Sound");
}

void ExamplePatches::CreateFMBrass(PresetManager& preset_manager) {
    PatchParameters params = CreateFMBaseParams();
    
    // FM settings for brass sound
    // Usually requires a more complex algorithm with feedback
    
    // Operator 0 (modulator)
    if (params.vco_params.size() > 0) {
        params.vco_params[0].frequency = 440.0 * 1.0;  // Same as fundamental
        params.vco_params[0].amplitude = 0.8;
    }
    
    // Operator 1 (carrier)
    if (params.vco_params.size() > 1) {
        params.vco_params[1].frequency = 440.0;  // Fundamental
        params.vco_params[1].amplitude = 0.9;
    }
    
    // Operator 2 (additional harmonics)
    if (params.vco_params.size() > 2) {
        params.vco_params[2].frequency = 440.0 * 2.0;  // Second harmonic
        params.vco_params[2].amplitude = 0.6;
    }
    
    // Operator 3 (brightness)
    if (params.vco_params.size() > 3) {
        params.vco_params[3].frequency = 440.0 * 3.0;  // Third harmonic
        params.vco_params[3].amplitude = 0.4;
    }
    
    // ADSR settings for brass characteristic
    ApplyADSRToParams(params, 0.1, 0.4, 0.9, 0.3);
    
    params.name = "FM Brass";
    params.description = "Brass sound created with FM synthesis";
    params.category = static_cast<int>(PatchCategory::BRASS);
    
    preset_manager.CreatePreset(params, "FM Brass");
}

void ExamplePatches::CreateFMBell(PresetManager& preset_manager) {
    PatchParameters params = CreateFMBaseParams();
    
    // FM settings for bell sound (already implemented in CreateBellSound, 
    // but with different parameters here)
    
    // Operator 0 (fast decaying harmonic)
    if (params.vco_params.size() > 0) {
        params.vco_params[0].frequency = 440.0 * 1.414;  // Square root of 2 harmonic
        params.vco_params[0].amplitude = 0.9;
    }
    
    // Operator 1 (main carrier)
    if (params.vco_params.size() > 1) {
        params.vco_params[1].frequency = 440.0;  // Fundamental
        params.vco_params[1].amplitude = 0.8;
    }
    
    // Operator 2 (harmonic)
    if (params.vco_params.size() > 2) {
        params.vco_params[2].frequency = 440.0 * 0.667;  // Subharmonic
        params.vco_params[2].amplitude = 0.6;
    }
    
    // Operator 3 (brightness)
    if (params.vco_params.size() > 3) {
        params.vco_params[3].frequency = 440.0 * 2.828;  // Multiple of square root of 2
        params.vco_params[3].amplitude = 0.4;
    }
    
    // ADSR settings for bell characteristic (quick attack, long decay)
    ApplyADSRToParams(params, 0.01, 2.0, 0.0, 1.5);
    
    params.name = "FM Bell";
    params.description = "Bell sound created with FM synthesis";
    params.category = static_cast<int>(PatchCategory::FX);
    
    preset_manager.CreatePreset(params, "FM Bell");
}

void ExamplePatches::CreateFMElectricPiano(PresetManager& preset_manager) {
    PatchParameters params = CreateFMBaseParams();
    
    // FM settings for electric piano (like DX7 electric piano)
    
    // Operator 0 (harmonic)
    if (params.vco_params.size() > 0) {
        params.vco_params[0].frequency = 440.0 * 1.0;  // Same as fundamental
        params.vco_params[0].amplitude = 0.7;
    }
    
    // Operator 1 (main carrier)
    if (params.vco_params.size() > 1) {
        params.vco_params[1].frequency = 440.0;  // Fundamental
        params.vco_params[1].amplitude = 0.9;
    }
    
    // Operator 2 (brightness)
    if (params.vco_params.size() > 2) {
        params.vco_params[2].frequency = 440.0 * 3.0;  // Third harmonic
        params.vco_params[2].amplitude = 0.6;
    }
    
    // Operator 3 (percussion)
    if (params.vco_params.size() > 3) {
        params.vco_params[3].frequency = 440.0 * 5.0;  // Fifth harmonic
        params.vco_params[3].amplitude = 0.3;
    }
    
    // ADSR settings - quick attack with moderate decay for electric piano characteristic
    ApplyADSRToParams(params, 0.01, 0.8, 0.6, 0.6);
    
    params.name = "FM Electric Piano";
    params.description = "Classic electric piano sound using FM synthesis";
    params.category = static_cast<int>(PatchCategory::PLUCKED);
    
    preset_manager.CreatePreset(params, "FM Electric Piano");
}

void ExamplePatches::CreateWaveTableSaw(PresetManager& preset_manager) {
    PatchParameters params = CreateWaveTableBaseParams();
    
    // Wave table settings for sawtooth
    // This would typically involve having a sawtooth wave in the wavetable collection
    
    // ADSR settings
    ApplyADSRToParams(params, 0.05, 0.3, 0.8, 0.4);
    
    params.name = "WaveTable Saw";
    params.description = "Rich sawtooth waveform with morphing";
    params.category = static_cast<int>(PatchCategory::LEAD);
    
    preset_manager.CreatePreset(params, "WaveTable Saw");
}

void ExamplePatches::CreateWaveTableSquare(PresetManager& preset_manager) {
    PatchParameters params = CreateWaveTableBaseParams();
    
    // Wave table settings for square wave
    
    // ADSR settings
    ApplyADSRToParams(params, 0.01, 0.2, 0.7, 0.3);
    
    params.name = "WaveTable Square";
    params.description = "Classic square wave with harmonic richness";
    params.category = static_cast<int>(PatchCategory::FX);
    
    preset_manager.CreatePreset(params, "WaveTable Square");
}

void ExamplePatches::CreateAdditiveHarmonic(PresetManager& preset_manager) {
    PatchParameters params = CreateAdditiveBaseParams();
    
    // Set up harmonic series with decreasing amplitudes
    // This would be handled in the additive synth implementation
    
    // ADSR settings
    ApplyADSRToParams(params, 0.1, 0.3, 0.7, 0.4);
    
    params.name = "Additive Harmonics";
    params.description = "Pure tone built from harmonic series";
    params.category = static_cast<int>(PatchCategory::PAD);
    
    preset_manager.CreatePreset(params, "Additive Harmonics");
}

void ExamplePatches::CreateAdditiveBell(PresetManager& preset_manager) {
    PatchParameters params = CreateAdditiveBaseParams();
    
    // Set up inharmonic partials for bell-like sound
    // This would involve setting specific frequencies and amplitudes for bell harmonics
    
    // ADSR settings - quick attack, long decay
    ApplyADSRToParams(params, 0.01, 1.8, 0.0, 1.2);
    
    params.name = "Additive Bell";
    params.description = "Bell sound created with additive synthesis";
    params.category = static_cast<int>(PatchCategory::FX);
    
    preset_manager.CreatePreset(params, "Additive Bell");
}

PatchParameters ExamplePatches::CreateSubtractiveBaseParams() {
    PatchParameters params;
    
    // Initialize with one VCO, VCF, VCA, LFO, and ADSR
    PatchParameters::VCOParams vco_params;
    vco_params.waveform_type = static_cast<int>(VCOType::SAWTOOTH);
    vco_params.frequency = 440.0;  // A4 note
    vco_params.amplitude = 0.8;
    vco_params.fm_amount = 0.0;
    vco_params.pwm_duty_cycle = 0.5;
    vco_params.anti_aliasing = true;
    params.vco_params.push_back(vco_params);
    
    // Add a second VCO for detuning/richness if needed
    params.vco_params.push_back(vco_params);
    
    // VCF params
    params.vcf_params.filter_type = 0;  // Low-pass filter
    params.vcf_params.cutoff_freq = 2000.0;
    params.vcf_params.resonance = 0.2;
    params.vcf_params.env_amount = 0.5;
    params.vcf_params.key_track_amount = 0.5;
    
    // VCA params
    params.vca_params.level = 0.8;
    params.vca_params.linear_response = false;
    
    // LFO params
    PatchParameters::LFOParams lfo_params;
    lfo_params.waveform_type = static_cast<int>(LFOType::SINE);
    lfo_params.frequency = 5.0;  // 5Hz
    lfo_params.amplitude = 0.5;
    params.lfo_params.push_back(lfo_params);
    
    // ADSR params
    PatchParameters::ADSRParams adsr_params;
    adsr_params.attack = 0.1;
    adsr_params.decay = 0.3;
    adsr_params.sustain = 0.7;
    adsr_params.release = 0.4;
    params.adsr_params.push_back(adsr_params);
    
    return params;
}

PatchParameters ExamplePatches::CreateFMBaseParams() {
    PatchParameters params;
    
    // Initialize with 4 operators (VCOs) for FM synthesis
    for (int i = 0; i < 4; i++) {
        PatchParameters::VCOParams vco_params;
        vco_params.waveform_type = static_cast<int>(VCOType::SINE);
        vco_params.frequency = 440.0;  // Will be adjusted per operator
        vco_params.amplitude = (i == 0 || i == 2) ? 0.5 : 0.8;  // Different amplitudes for different operators
        vco_params.fm_amount = 0.0;
        vco_params.pwm_duty_cycle = 0.5;
        vco_params.anti_aliasing = true;
        params.vco_params.push_back(vco_params);
    }
    
    // VCA params
    params.vca_params.level = 0.8;
    params.vca_params.linear_response = false;
    
    // ADSR params
    PatchParameters::ADSRParams adsr_params;
    adsr_params.attack = 0.1;
    adsr_params.decay = 0.3;
    adsr_params.sustain = 0.7;
    adsr_params.release = 0.4;
    params.adsr_params.push_back(adsr_params);
    
    return params;
}

PatchParameters ExamplePatches::CreateWaveTableBaseParams() {
    PatchParameters params;
    
    // Initialize with basic parameters
    params.vca_params.level = 0.8;
    params.vca_params.linear_response = false;
    
    // ADSR params
    PatchParameters::ADSRParams adsr_params;
    adsr_params.attack = 0.1;
    adsr_params.decay = 0.3;
    adsr_params.sustain = 0.7;
    adsr_params.release = 0.4;
    params.adsr_params.push_back(adsr_params);
    
    return params;
}

PatchParameters ExamplePatches::CreateAdditiveBaseParams() {
    PatchParameters params;
    
    // Initialize with basic parameters
    params.vca_params.level = 0.8;
    params.vca_params.linear_response = false;
    
    // ADSR params
    PatchParameters::ADSRParams adsr_params;
    adsr_params.attack = 0.1;
    adsr_params.decay = 0.3;
    adsr_params.sustain = 0.7;
    adsr_params.release = 0.4;
    params.adsr_params.push_back(adsr_params);
    
    return params;
}

void ExamplePatches::ApplyADSRToParams(PatchParameters& params, double a, double d, double s, double r) {
    if (!params.adsr_params.empty()) {
        params.adsr_params[0].attack = a;
        params.adsr_params[0].decay = d;
        params.adsr_params[0].sustain = s;
        params.adsr_params[0].release = r;
    } else {
        PatchParameters::ADSRParams adsr;
        adsr.attack = a;
        adsr.decay = d;
        adsr.sustain = s;
        adsr.release = r;
        params.adsr_params.push_back(adsr);
    }
}

void ExamplePatches::AddModulationConnection(PatchParameters& params, ModulationSource source, 
                                            ModulationDestination dest, double amount, const std::string& name) {
    PatchParameters::ModulationParams::ConnectionParams conn;
    conn.source = static_cast<int>(source);
    conn.destination = static_cast<int>(dest);
    conn.amount = amount;
    conn.active = true;
    conn.name = name;
    
    params.modulation_params.connections.push_back(conn);
}