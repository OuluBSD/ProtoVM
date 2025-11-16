#include "SynthesizerArchitectures.h"
#include <cmath>
#include <algorithm>

// Base class implementation
SynthArchitectureBase::SynthArchitectureBase(SynthArchitecture type) 
    : architecture_type(type)
{
    // Initialize the base components
    modulation_matrix = std::make_unique<ModulationMatrix>(32);  // Default to 32 modulation connections
    audio_output = std::make_unique<AudioOutputSystem>();
    preset_manager = std::make_unique<PresetManager>();
}

bool SynthArchitectureBase::Tick() {
    // Process all the components in the architecture
    // This will be called in each simulation tick
    
    // Tick all VCOs
    for (auto& vco : vcos) {
        vco->Tick();
    }
    
    // Tick all VCFs
    for (auto& vcf : vcfs) {
        vcf->Tick();
    }
    
    // Tick all VCAs
    for (auto& vca : vcas) {
        vca->Tick();
    }
    
    // Tick all LFOs
    for (auto& lfo : lfos) {
        lfo->Tick();
    }
    
    // Tick all ADSRs
    for (auto& adsr : adsrs) {
        adsr->Tick();
    }
    
    // Tick the modulation matrix
    if (modulation_matrix) {
        modulation_matrix->Tick();
    }
    
    // Tick the audio output system
    if (audio_output) {
        audio_output->Tick();
    }
    
    return true;
}

double SynthArchitectureBase::NoteToFrequency(int note) const {
    // Convert MIDI note number to frequency in Hz
    // Formula: f = 440 * 2^((n-69)/12)
    return 440.0 * pow(2.0, (note - 69) / 12.0);
}

int SynthArchitectureBase::FrequencyToNote(double freq) const {
    // Convert frequency in Hz to MIDI note number
    // Formula: n = 12 * log2(f/440) + 69
    return static_cast<int>(12 * log2(freq / 440.0) + 69.5);  // +0.5 for rounding
}


// Subtractive synthesis implementation
SubtractiveSynth::SubtractiveSynth() 
    : SynthArchitectureBase(SynthArchitecture::SUBTRACTIVE)
{
    // Initialize with default components: 1 VCO, 1 VCF, 1 VCA, 1 LFO, 1 ADSR
    vcos.push_back(std::make_unique<VCO>(VCOType::SAWTOOTH, 440.0));
    vcfs.push_back(std::make_unique<VCF>());
    vcas.push_back(std::make_unique<VCA>());
    lfos.push_back(std::make_unique<LFO>(LFOType::SINE, 5.0));  // 5Hz default
    adsrs.push_back(std::make_unique<ADSR>());
    
    // Set up default modulation connections
    // LFO modulates filter cutoff, ADSR modulates VCA
    if (modulation_matrix) {
        modulation_matrix->AddConnection(ModulationConnection(
            ModulationSource::LFO1, 
            ModulationDestination::VCF_CUTOFF, 
            0.3, 
            true, 
            "LFO Filter Mod"));
            
        modulation_matrix->AddConnection(ModulationConnection(
            ModulationSource::ADSR1, 
            ModulationDestination::VCA_LEVEL, 
            1.0, 
            true, 
            "ADSR VCA Mod"));
    }
}

bool SubtractiveSynth::NoteOn(int note, int velocity, int channel) {
    double freq = NoteToFrequency(note);
    
    // Set the frequency for all VCOs
    for (auto& vco : vcos) {
        vco->SetBaseFrequency(freq);
    }
    
    // Trigger the ADSR envelope
    if (!adsrs.empty()) {
        adsrs[0]->NoteOn();
    }
    
    // Add the note frequency to the active list
    current_note_frequencies.push_back(freq);
    
    return true;
}

bool SubtractiveSynth::NoteOff(int note, int channel) {
    double freq = NoteToFrequency(note);
    
    // Find and remove the frequency from active list
    auto it = std::find(current_note_frequencies.begin(), current_note_frequencies.end(), freq);
    if (it != current_note_frequencies.end()) {
        current_note_frequencies.erase(it);
        
        // If no more notes are active, trigger release
        if (current_note_frequencies.empty() && !adsrs.empty()) {
            adsrs[0]->NoteOff();
        }
    }
    
    return true;
}

bool SubtractiveSynth::AllNotesOff() {
    current_note_frequencies.clear();
    
    // Release all ADSRs
    for (auto& adsr : adsrs) {
        adsr->NoteOff();
    }
    
    return true;
}

bool SubtractiveSynth::SetParameter(const std::string& name, double value) {
    // Parse parameter names and route to appropriate components
    if (name.substr(0, 4) == "vco_") {
        // Parameters like vco_0_frequency, vco_1_pulse_width, etc.
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string vco_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int vco_id = 0;
            try {
                vco_id = std::stoi(vco_id_str);
                if (vco_id >= 0 && vco_id < vcos.size()) {
                    if (param_name == "frequency") {
                        vcos[vco_id]->SetBaseFrequency(value);
                        return true;
                    } else if (param_name == "pulse_width") {
                        vcos[vco_id]->SetPWM(value);
                        return true;
                    } else if (param_name == "amplitude") {
                        vcos[vco_id]->SetAmplitude(value);
                        return true;
                    } else if (param_name == "fm_amount") {
                        vcos[vco_id]->SetFMModulation(value);
                        return true;
                    }
                }
            } catch (...) {
                // Invalid vco_id, continue below
            }
        }
    } else if (name.substr(0, 4) == "vcf_") {
        // Parameters like vcf_0_cutoff, vcf_0_resonance, etc.
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string vcf_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int vcf_id = 0;
            try {
                vcf_id = std::stoi(vcf_id_str);
                if (vcf_id >= 0 && vcf_id < vcfs.size()) {
                    if (param_name == "cutoff") {
                        vcfs[vcf_id]->SetCutoffFreq(value);
                        return true;
                    } else if (param_name == "resonance") {
                        vcfs[vcf_id]->SetResonance(value);
                        return true;
                    }
                }
            } catch (...) {
                // Invalid vcf_id, continue below
            }
        }
    } else if (name.substr(0, 4) == "vca_") {
        // Parameters like vca_0_level, etc.
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string vca_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int vca_id = 0;
            try {
                vca_id = std::stoi(vca_id_str);
                if (vca_id >= 0 && vca_id < vcas.size()) {
                    if (param_name == "level") {
                        vcas[vca_id]->SetLevel(value);
                        return true;
                    }
                }
            } catch (...) {
                // Invalid vca_id, continue below
            }
        }
    } else if (name.substr(0, 4) == "adsr_") {
        // Parameters like adsr_0_attack, adsr_0_decay, etc.
        size_t underscore_pos = name.find('_', 5);
        if (underscore_pos != std::string::npos) {
            std::string adsr_id_str = name.substr(5, underscore_pos - 5);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int adsr_id = 0;
            try {
                adsr_id = std::stoi(adsr_id_str);
                if (adsr_id >= 0 && adsr_id < adsrs.size()) {
                    if (param_name == "attack") {
                        adsrs[adsr_id]->SetAttack(value);
                        return true;
                    } else if (param_name == "decay") {
                        adsrs[adsr_id]->SetDecay(value);
                        return true;
                    } else if (param_name == "sustain") {
                        adsrs[adsr_id]->SetSustain(value);
                        return true;
                    } else if (param_name == "release") {
                        adsrs[adsr_id]->SetRelease(value);
                        return true;
                    }
                }
            } catch (...) {
                // Invalid adsr_id, continue below
            }
        }
    } else if (name.substr(0, 4) == "lfo_") {
        // Parameters like lfo_0_frequency, etc.
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string lfo_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int lfo_id = 0;
            try {
                lfo_id = std::stoi(lfo_id_str);
                if (lfo_id >= 0 && lfo_id < lfos.size()) {
                    if (param_name == "frequency") {
                        lfos[lfo_id]->SetFrequency(value);
                        return true;
                    } else if (param_name == "amplitude") {
                        lfos[lfo_id]->SetAmplitude(value);
                        return true;
                    }
                }
            } catch (...) {
                // Invalid lfo_id, continue below
            }
        }
    }
    
    return false;
}

double SubtractiveSynth::GetParameter(const std::string& name) const {
    // Similar to SetParameter but returns values
    if (name.substr(0, 4) == "vco_") {
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string vco_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int vco_id = 0;
            try {
                vco_id = std::stoi(vco_id_str);
                if (vco_id >= 0 && vco_id < vcos.size()) {
                    if (param_name == "frequency") {
                        return vcos[vco_id]->GetBaseFrequency();
                    } else if (param_name == "pulse_width") {
                        return vcos[vco_id]->GetPWM();
                    } else if (param_name == "amplitude") {
                        return vcos[vco_id]->GetAmplitude();
                    } else if (param_name == "fm_amount") {
                        return vcos[vco_id]->GetFMModulation();
                    }
                }
            } catch (...) {
                // Invalid vco_id
            }
        }
    } else if (name.substr(0, 4) == "vcf_") {
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string vcf_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int vcf_id = 0;
            try {
                vcf_id = std::stoi(vcf_id_str);
                if (vcf_id >= 0 && vcf_id < vcfs.size()) {
                    if (param_name == "cutoff") {
                        return vcfs[vcf_id]->GetCutoffFreq();
                    } else if (param_name == "resonance") {
                        return vcfs[vcf_id]->GetResonance();
                    }
                }
            } catch (...) {
                // Invalid vcf_id
            }
        }
    } else if (name.substr(0, 4) == "vca_") {
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string vca_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int vca_id = 0;
            try {
                vca_id = std::stoi(vca_id_str);
                if (vca_id >= 0 && vca_id < vcas.size()) {
                    if (param_name == "level") {
                        return vcas[vca_id]->GetLevel();
                    }
                }
            } catch (...) {
                // Invalid vca_id
            }
        }
    } else if (name.substr(0, 5) == "adsr_") {
        size_t underscore_pos = name.find('_', 5);
        if (underscore_pos != std::string::npos) {
            std::string adsr_id_str = name.substr(5, underscore_pos - 5);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int adsr_id = 0;
            try {
                adsr_id = std::stoi(adsr_id_str);
                if (adsr_id >= 0 && adsr_id < adsrs.size()) {
                    if (param_name == "attack") {
                        return adsrs[adsr_id]->GetAttack();
                    } else if (param_name == "decay") {
                        return adsrs[adsr_id]->GetDecay();
                    } else if (param_name == "sustain") {
                        return adsrs[adsr_id]->GetSustain();
                    } else if (param_name == "release") {
                        return adsrs[adsr_id]->GetRelease();
                    }
                }
            } catch (...) {
                // Invalid adsr_id
            }
        }
    } else if (name.substr(0, 4) == "lfo_") {
        size_t underscore_pos = name.find('_', 4);
        if (underscore_pos != std::string::npos) {
            std::string lfo_id_str = name.substr(4, underscore_pos - 4);
            std::string param_name = name.substr(underscore_pos + 1);
            
            int lfo_id = 0;
            try {
                lfo_id = std::stoi(lfo_id_str);
                if (lfo_id >= 0 && lfo_id < lfos.size()) {
                    if (param_name == "frequency") {
                        return lfos[lfo_id]->GetFrequency();
                    } else if (param_name == "amplitude") {
                        return lfos[lfo_id]->GetAmplitude();
                    }
                }
            } catch (...) {
                // Invalid lfo_id
            }
        }
    }
    
    return 0.0;  // Default return value
}

std::vector<double> SubtractiveSynth::GetAudioOutput() {
    // Process the subtractive synthesis: VCO -> VCF -> VCA
    std::vector<double> output;
    
    if (vcos.empty() || vcfs.empty() || vcas.empty()) {
        output.resize(audio_output->GetChannelCount(), 0.0);
        return output;
    }
    
    // Get the output from the first VCO (in a more complex implementation, 
    // multiple VCOs would be mixed together)
    double vco_output = vcos[0]->GetOutput();
    
    // Apply filter modulation from the modulation matrix
    if (modulation_matrix) {
        double cutoff_mod = modulation_matrix->ProcessModulation(ModulationDestination::VCF_CUTOFF, 
                                                                 vcfs[0]->GetCutoffFreq());
        vcfs[0]->SetCutoffFreq(cutoff_mod);
    }
    
    // Process through the filter
    double filtered_output = vcfs[0]->Process(vco_output);
    
    // Apply amplitude modulation from the modulation matrix
    if (modulation_matrix) {
        double vca_mod = modulation_matrix->ProcessModulation(ModulationDestination::VCA_LEVEL,
                                                              vcas[0]->GetLevel());
        vcas[0]->SetLevel(vca_mod);
    }
    
    // Process through the VCA
    double final_output = vcas[0]->Process(filtered_output);
    
    // Set the LFO values in the modulation matrix for next tick
    if (modulation_matrix && !lfos.empty()) {
        modulation_matrix->SetLFOValue(1, lfos[0]->GetOutput());
    }
    
    // Set the ADSR values in the modulation matrix for next tick
    if (modulation_matrix && !adsrs.empty()) {
        modulation_matrix->SetADSRValue(1, adsrs[0]->GetOutput());
    }
    
    // Prepare output for stereo (or other channel count)
    output.resize(audio_output->GetChannelCount(), final_output);
    
    return output;
}

void SubtractiveSynth::SetVCOCount(int count) {
    vcos.clear();
    for (int i = 0; i < count; i++) {
        vcos.push_back(std::make_unique<VCO>(VCOType::SAWTOOTH, 440.0));
    }
    
    // Update modulation matrix with routing for new VCOs
    if (modulation_matrix) {
        for (int i = 0; i < count; i++) {
            // Add modulation connections for each VCO
            modulation_matrix->AddConnection(ModulationConnection(
                static_cast<ModulationSource>(static_cast<int>(ModulationSource::LFO1) + i % 3), 
                static_cast<ModulationDestination>(static_cast<int>(ModulationDestination::VCO1_PITCH) + i), 
                0.1, 
                true, 
                "LFO" + std::to_string((i % 3) + 1) + " VCO" + std::to_string(i + 1) + " Pitch Mod"));
        }
    }
}

void SubtractiveSynth::SetVCFCount(int count) {
    vcfs.clear();
    for (int i = 0; i < count; i++) {
        vcfs.push_back(std::make_unique<VCF>());
    }
}

void SubtractiveSynth::SetLFOCount(int count) {
    lfos.clear();
    for (int i = 0; i < count; i++) {
        lfos.push_back(std::make_unique<LFO>(LFOType::SINE, 5.0 + i));  // Slightly different frequencies
    }
}

void SubtractiveSynth::SetADSRCount(int count) {
    adsrs.clear();
    for (int i = 0; i < count; i++) {
        adsrs.push_back(std::make_unique<ADSR>());
    }
}

void SubtractiveSynth::SetFilterRouting(int vco_id, int vcf_id) {
    if (vco_id >= 0 && vco_id < vcos.size() && vcf_id >= 0 && vcf_id < vcfs.size()) {
        // Update the routing - in more complex implementation this would define
        // how multiple VCOs connect to multiple VCFs
        if (filter_routing.size() <= vco_id) {
            filter_routing.resize(vco_id + 1, 0);  // Default to VCF 0
        }
        filter_routing[vco_id] = vcf_id;
    }
}


// FM Synthesis implementation
FMSynth::FMSynth(int operators) 
    : SynthArchitectureBase(SynthArchitecture::FM), algorithm(0)
{
    // Initialize the operators
    operators.resize(operators);
    for (int i = 0; i < operators; i++) {
        operators[i].oscillator = std::make_unique<VCO>(VCOType::SINE, 440.0);
        operators[i].level = 1.0;  // Default full level
        operators[i].frequency_ratio = 1.0;  // Default to fundamental frequency
    }
    
    // Set up a simple algorithm where operator 0 modulates operator 1, 
    // which modulates operator 2, etc., with the last operator being the output
    for (int i = 0; i < operators.size() - 1; i++) {
        operators[i+1].modulations.push_back(std::make_pair(i, 1.0));  // Operator i modulates operator i+1
    }
    
    // Initialize other components
    vcos.push_back(std::make_unique<VCO>(VCOType::SINE, 440.0));  // For LFO if needed
    adsrs.push_back(std::make_unique<ADSR>());
    lfos.push_back(std::make_unique<LFO>(LFOType::SINE, 5.0));
    
    // Initialize output buffers
    operator_outputs.resize(1);  // For one active note initially
    operator_outputs[0].resize(operators.size(), 0.0);
}

bool FMSynth::NoteOn(int note, int velocity, int channel) {
    double freq = NoteToFrequency(note);
    current_note_frequencies.push_back(freq);
    
    // Set the base frequency for all operators based on their ratios
    for (size_t i = 0; i < operators.size(); i++) {
        operators[i].oscillator->SetBaseFrequency(freq * operators[i].frequency_ratio);
    }
    
    // Trigger the ADSR envelope
    if (!adsrs.empty()) {
        adsrs[0]->NoteOn();
    }
    
    // Resize operator outputs for the new note
    operator_outputs.resize(current_note_frequencies.size());
    for (auto& output_vec : operator_outputs) {
        if (output_vec.size() != operators.size()) {
            output_vec.resize(operators.size(), 0.0);
        }
    }
    
    return true;
}

bool FMSynth::NoteOff(int note, int channel) {
    double freq = NoteToFrequency(note);
    
    // Find and remove the frequency from active list
    auto it = std::find(current_note_frequencies.begin(), current_note_frequencies.end(), freq);
    if (it != current_note_frequencies.end()) {
        int idx = it - current_note_frequencies.begin();
        current_note_frequencies.erase(it);
        
        // Remove the corresponding operator outputs
        if (idx < operator_outputs.size()) {
            operator_outputs.erase(operator_outputs.begin() + idx);
        }
        
        // If no more notes are active, trigger release
        if (current_note_frequencies.empty() && !adsrs.empty()) {
            adsrs[0]->NoteOff();
        }
    }
    
    return true;
}

bool FMSynth::AllNotesOff() {
    current_note_frequencies.clear();
    operator_outputs.clear();
    
    // Release all ADSRs
    for (auto& adsr : adsrs) {
        adsr->NoteOff();
    }
    
    return true;
}

bool FMSynth::SetParameter(const std::string& name, double value) {
    // Handle FM-specific parameters like operator levels, ratios, etc.
    if (name.substr(0, 9) == "operator_") {
        size_t first_underscore = name.find('_', 9);
        if (first_underscore != std::string::npos) {
            std::string op_id_str = name.substr(9, first_underscore - 9);
            std::string param_name = name.substr(first_underscore + 1);
            
            int op_id = 0;
            try {
                op_id = std::stoi(op_id_str);
                if (op_id >= 0 && op_id < operators.size()) {
                    if (param_name == "level") {
                        operators[op_id].level = std::max(0.0, std::min(1.0, value));
                        return true;
                    } else if (param_name == "frequency_ratio") {
                        operators[op_id].frequency_ratio = std::max(0.01, value);  // Minimum 0.01 to avoid zero
                        return true;
                    } else if (param_name == "amplitude") {
                        operators[op_id].oscillator->SetAmplitude(value);
                        return true;
                    }
                }
            } catch (...) {
                // Invalid op_id
            }
        }
    } else if (name == "algorithm") {
        SetAlgorithm(static_cast<int>(value));
        return true;
    }
    
    // Fall back to base class implementation for other parameters
    return SynthArchitectureBase::SetParameter(name, value);
}

double FMSynth::GetParameter(const std::string& name) const {
    if (name.substr(0, 9) == "operator_") {
        size_t first_underscore = name.find('_', 9);
        if (first_underscore != std::string::npos) {
            std::string op_id_str = name.substr(9, first_underscore - 9);
            std::string param_name = name.substr(first_underscore + 1);
            
            int op_id = 0;
            try {
                op_id = std::stoi(op_id_str);
                if (op_id >= 0 && op_id < operators.size()) {
                    if (param_name == "level") {
                        return operators[op_id].level;
                    } else if (param_name == "frequency_ratio") {
                        return operators[op_id].frequency_ratio;
                    } else if (param_name == "amplitude") {
                        return operators[op_id].oscillator->GetAmplitude();
                    }
                }
            } catch (...) {
                // Invalid op_id
            }
        }
    } else if (name == "algorithm") {
        return static_cast<double>(algorithm);
    }
    
    // Fall back to base class implementation for other parameters
    return SynthArchitectureBase::GetParameter(name);
}

std::vector<double> FMSynth::GetAudioOutput() {
    std::vector<double> output;
    output.resize(audio_output->GetChannelCount(), 0.0);
    
    if (operators.empty()) {
        return output;
    }
    
    // Process each active note
    for (size_t note_idx = 0; note_idx < current_note_frequencies.size(); note_idx++) {
        // For each operator, calculate its output considering modulations
        for (size_t op_idx = 0; op_idx < operators.size(); op_idx++) {
            // Apply frequency modulation from other operators
            double base_freq = current_note_frequencies[note_idx] * operators[op_idx].frequency_ratio;
            
            // Apply modulations from other operators
            for (const auto& mod : operators[op_idx].modulations) {
                int modulator_id = mod.first;
                double modulation_index = mod.second;
                
                if (modulator_id < operator_outputs[note_idx].size()) {
                    base_freq += operator_outputs[note_idx][modulator_id] * modulation_index * base_freq;
                }
            }
            
            operators[op_idx].oscillator->SetBaseFrequency(base_freq);
            
            // Process the oscillator
            operators[op_idx].oscillator->Tick();
            operator_outputs[note_idx][op_idx] = operators[op_idx].oscillator->GetOutput() * operators[op_idx].level;
        }
        
        // The final output is typically from the last operator in the algorithm
        double final_output = operator_outputs[note_idx].back();
        
        // Apply amplitude envelope from ADSR
        if (!adsrs.empty()) {
            final_output *= adsrs[0]->GetOutput();
        }
        
        // Add to the overall output
        for (int ch = 0; ch < audio_output->GetChannelCount(); ch++) {
            output[ch] += final_output;
        }
    }
    
    return output;
}

void FMSynth::SetAlgorithm(int alg) {
    algorithm = alg;
    // In a real implementation, this would set up different operator routing
    // For now, we'll keep the simple linear algorithm
}

void FMSynth::SetOperatorFrequencyRatio(int op_id, double ratio) {
    if (op_id >= 0 && op_id < operators.size()) {
        operators[op_id].frequency_ratio = std::max(0.01, ratio);  // Minimum 0.01 to avoid zero
    }
}

void FMSynth::SetOperatorLevel(int op_id, double level) {
    if (op_id >= 0 && op_id < operators.size()) {
        operators[op_id].level = std::max(0.0, std::min(1.0, level));
    }
}

void FMSynth::SetModulationIndex(int modulator_id, int carrier_id, double index) {
    if (modulator_id >= 0 && modulator_id < operators.size() && 
        carrier_id >= 0 && carrier_id < operators.size()) {
        
        // Find if this modulation already exists
        bool found = false;
        for (auto& mod : operators[carrier_id].modulations) {
            if (mod.first == modulator_id) {
                mod.second = index;
                found = true;
                break;
            }
        }
        
        if (!found) {
            operators[carrier_id].modulations.push_back(std::make_pair(modulator_id, index));
        }
    }
}


// WaveTableSynthesis implementation
WaveTableSynth::WaveTableSynth() 
    : SynthArchitectureBase(SynthArchitecture::WAVE_TABLE), wave_table_position(0.0), wave_table_crossfade(0.0)
{
    // Initialize with a basic sine wave table
    std::vector<double> sine_table(512);
    for (size_t i = 0; i < sine_table.size(); i++) {
        sine_table[i] = sin(2.0 * M_PI * i / sine_table.size());
    }
    wave_tables.push_back(sine_table);
    
    // Add a sawtooth table
    std::vector<double> sawtooth_table(512);
    for (size_t i = 0; i < sawtooth_table.size(); i++) {
        sawtooth_table[i] = 2.0 * (double(i) / sawtooth_table.size()) - 1.0;
    }
    wave_tables.push_back(sawtooth_table);
    
    // Initialize other components
    adsrs.push_back(std::make_unique<ADSR>());
}

bool WaveTableSynth::NoteOn(int note, int velocity, int channel) {
    double freq = NoteToFrequency(note);
    current_note_frequencies.push_back(freq);
    
    // Initialize wave table indices for this note
    current_wave_table_indices.push_back(wave_table_position);
    
    // Trigger the ADSR envelope
    if (!adsrs.empty()) {
        adsrs[0]->NoteOn();
    }
    
    return true;
}

bool WaveTableSynth::NoteOff(int note, int channel) {
    double freq = NoteToFrequency(note);
    
    // Find and remove the frequency from active list
    auto it = std::find(current_note_frequencies.begin(), current_note_frequencies.end(), freq);
    if (it != current_note_frequencies.end()) {
        int idx = it - current_note_frequencies.begin();
        current_note_frequencies.erase(it);
        current_wave_table_indices.erase(current_wave_table_indices.begin() + idx);
        
        // If no more notes are active, trigger release
        if (current_note_frequencies.empty() && !adsrs.empty()) {
            adsrs[0]->NoteOff();
        }
    }
    
    return true;
}

bool WaveTableSynth::AllNotesOff() {
    current_note_frequencies.clear();
    current_wave_table_indices.clear();
    
    // Release all ADSRs
    for (auto& adsr : adsrs) {
        adsr->NoteOff();
    }
    
    return true;
}

bool WaveTableSynth::SetParameter(const std::string& name, double value) {
    if (name == "wave_table_position") {
        SetWaveTableIndex(value);
        return true;
    } else if (name == "wave_table_crossfade") {
        SetWaveTableCrossfade(value);
        return true;
    }
    
    // Fall back to base class implementation for other parameters
    return SynthArchitectureBase::SetParameter(name, value);
}

double WaveTableSynth::GetParameter(const std::string& name) const {
    if (name == "wave_table_position") {
        return wave_table_position;
    } else if (name == "wave_table_crossfade") {
        return wave_table_crossfade;
    }
    
    // Fall back to base class implementation for other parameters
    return SynthArchitectureBase::GetParameter(name);
}

std::vector<double> WaveTableSynth::GetAudioOutput() {
    std::vector<double> output;
    output.resize(audio_output->GetChannelCount(), 0.0);
    
    if (wave_tables.empty()) {
        return output;
    }
    
    // Process each active note
    for (size_t note_idx = 0; note_idx < current_note_frequencies.size(); note_idx++) {
        double freq = current_note_frequencies[note_idx];
        double& table_pos = current_wave_table_indices[note_idx];
        
        // Update table position based on frequency and sample rate
        // For simplicity, we'll assume 44.1kHz sample rate
        double phase_increment = (freq * 2 * M_PI) / 44100.0;
        table_pos += phase_increment / (2 * M_PI);  // Convert phase increment to table position increment
        if (table_pos >= 1.0) {
            table_pos -= 1.0;  // Wrap around
        }
        
        // Get the output by interpolating between wave tables based on position
        double final_output = 0.0;
        if (wave_tables.size() == 1) {
            // Only one table, just sample it
            size_t table_size = wave_tables[0].size();
            double scaled_pos = table_pos * table_size;
            size_t idx1 = static_cast<size_t>(scaled_pos) % table_size;
            size_t idx2 = (idx1 + 1) % table_size;
            double fraction = scaled_pos - floor(scaled_pos);
            
            final_output = wave_tables[0][idx1] * (1.0 - fraction) + wave_tables[0][idx2] * fraction;
        } else {
            // Multiple tables - crossfade between adjacent tables
            double table_pos_scaled = wave_table_position * (wave_tables.size() - 1);
            size_t table_idx1 = static_cast<size_t>(table_pos_scaled);
            size_t table_idx2 = std::min(table_idx1 + 1, wave_tables.size() - 1);
            double crossfade = table_pos_scaled - table_idx1;
            
            // Sample both tables and crossfade
            size_t table_size = wave_tables[0].size();  // Assume all tables are the same size
            double scaled_pos = table_pos * table_size;
            size_t idx1 = static_cast<size_t>(scaled_pos) % table_size;
            size_t idx2 = (idx1 + 1) % table_size;
            double fraction = scaled_pos - floor(scaled_pos);
            
            double sample1 = wave_tables[table_idx1][idx1] * (1.0 - fraction) + wave_tables[table_idx1][idx2] * fraction;
            double sample2 = wave_tables[table_idx2][idx1] * (1.0 - fraction) + wave_tables[table_idx2][idx2] * fraction;
            
            final_output = sample1 * (1.0 - crossfade) + sample2 * crossfade;
        }
        
        // Apply amplitude envelope from ADSR
        if (!adsrs.empty()) {
            final_output *= adsrs[0]->GetOutput();
        }
        
        // Add to the overall output
        for (int ch = 0; ch < audio_output->GetChannelCount(); ch++) {
            output[ch] += final_output;
        }
    }
    
    return output;
}

void WaveTableSynth::AddWaveTable(const std::vector<double>& wave_table) {
    wave_tables.push_back(wave_table);
}

void WaveTableSynth::SetWaveTableIndex(double index) {
    wave_table_position = std::max(0.0, std::min(1.0, index));
}

void WaveTableSynth::SetWaveTableCrossfade(double crossfade_pos) {
    wave_table_crossfade = std::max(0.0, std::min(1.0, crossfade_pos));
}


// AdditiveSynthesis implementation
AdditiveSynth::AdditiveSynth(int harmonics) 
    : SynthArchitectureBase(SynthArchitecture::ADDITIVE), harmonic_count(harmonics)
{
    // Initialize harmonics for each potential note
    harmonic_levels.resize(1);  // Start with space for one note
    harmonic_ratios.resize(1);
    harmonic_phases.resize(1);
    
    // Initialize with basic harmonic structure (harmonic series)
    for (int i = 0; i < harmonic_count; i++) {
        harmonic_ratios[0].push_back(i + 1);  // Harmonic series: 1, 2, 3, 4...
        harmonic_levels[0].push_back(1.0 / (i + 1));  // Decreasing amplitude
        harmonic_phases[0].push_back(0.0);  // Start with zero phase
    }
    
    // Initialize other components
    adsrs.push_back(std::make_unique<ADSR>());
}

bool AdditiveSynth::NoteOn(int note, int velocity, int channel) {
    double freq = NoteToFrequency(note);
    current_note_frequencies.push_back(freq);
    
    // Ensure we have harmonic data for this note
    if (harmonic_levels.size() <= current_note_frequencies.size()) {
        harmonic_levels.resize(current_note_frequencies.size());
        harmonic_ratios.resize(current_note_frequencies.size());
        harmonic_phases.resize(current_note_frequencies.size());
        
        // Copy the harmonic structure from the first note
        for (int h = 0; h < harmonic_count; h++) {
            harmonic_levels.back().push_back(harmonic_levels[0][h]);
            harmonic_ratios.back().push_back(harmonic_ratios[0][h]);
            harmonic_phases.back().push_back(harmonic_phases[0][h]);
        }
    }
    
    // Trigger the ADSR envelope
    if (!adsrs.empty()) {
        adsrs[0]->NoteOn();
    }
    
    return true;
}

bool AdditiveSynth::NoteOff(int note, int channel) {
    double freq = NoteToFrequency(note);
    
    // Find and remove the frequency from active list
    auto it = std::find(current_note_frequencies.begin(), current_note_frequencies.end(), freq);
    if (it != current_note_frequencies.end()) {
        int idx = it - current_note_frequencies.begin();
        current_note_frequencies.erase(it);
        
        // Remove the corresponding harmonic data
        if (idx < harmonic_levels.size()) {
            harmonic_levels.erase(harmonic_levels.begin() + idx);
            harmonic_ratios.erase(harmonic_ratios.begin() + idx);
            harmonic_phases.erase(harmonic_phases.begin() + idx);
        }
        
        // If no more notes are active, trigger release
        if (current_note_frequencies.empty() && !adsrs.empty()) {
            adsrs[0]->NoteOff();
        }
    }
    
    return true;
}

bool AdditiveSynth::AllNotesOff() {
    current_note_frequencies.clear();
    harmonic_levels.clear();
    harmonic_ratios.clear();
    harmonic_phases.clear();
    
    // Also resize our harmonic vectors
    harmonic_levels.resize(1);
    harmonic_ratios.resize(1);
    harmonic_phases.resize(1);
    
    // Initialize with basic harmonic structure
    if (harmonic_levels[0].size() != harmonic_count) {
        harmonic_levels[0].clear();
        harmonic_ratios[0].clear();
        harmonic_phases[0].clear();
        
        for (int i = 0; i < harmonic_count; i++) {
            harmonic_ratios[0].push_back(i + 1);  // Harmonic series: 1, 2, 3, 4...
            harmonic_levels[0].push_back(1.0 / (i + 1));  // Decreasing amplitude
            harmonic_phases[0].push_back(0.0);  // Start with zero phase
        }
    }
    
    // Release all ADSRs
    for (auto& adsr : adsrs) {
        adsr->NoteOff();
    }
    
    return true;
}

bool AdditiveSynth::SetParameter(const std::string& name, double value) {
    if (name.substr(0, 9) == "harmonic_") {
        size_t first_underscore = name.find('_', 9);
        if (first_underscore != std::string::npos) {
            std::string harmonic_str = name.substr(9, first_underscore - 9);
            std::string param_name = name.substr(first_underscore + 1);
            
            int harmonic = 0;
            try {
                harmonic = std::stoi(harmonic_str);
                if (harmonic >= 0 && harmonic < harmonic_count) {
                    if (param_name == "level") {
                        // Set the level for this harmonic across all active notes
                        for (auto& levels : harmonic_levels) {
                            if (harmonic < levels.size()) {
                                levels[harmonic] = std::max(0.0, std::min(1.0, value));
                            }
                        }
                        return true;
                    } else if (param_name == "frequency_ratio") {
                        // Set the frequency ratio for this harmonic across all active notes
                        for (auto& ratios : harmonic_ratios) {
                            if (harmonic < ratios.size()) {
                                ratios[harmonic] = std::max(0.01, value);  // Minimum 0.01 to avoid zero
                            }
                        }
                        return true;
                    } else if (param_name == "phase") {
                        // Set the phase for this harmonic across all active notes
                        for (auto& phases : harmonic_phases) {
                            if (harmonic < phases.size()) {
                                phases[harmonic] = fmod(value, 2 * M_PI);  // Normalize to 0-2π
                            }
                        }
                        return true;
                    }
                }
            } catch (...) {
                // Invalid harmonic number
            }
        }
    }
    
    // Fall back to base class implementation for other parameters
    return SynthArchitectureBase::SetParameter(name, value);
}

double AdditiveSynth::GetParameter(const std::string& name) const {
    if (name.substr(0, 9) == "harmonic_") {
        size_t first_underscore = name.find('_', 9);
        if (first_underscore != std::string::npos) {
            std::string harmonic_str = name.substr(9, first_underscore - 9);
            std::string param_name = name.substr(first_underscore + 1);
            
            int harmonic = 0;
            try {
                harmonic = std::stoi(harmonic_str);
                if (harmonic >= 0 && harmonic < harmonic_count && !harmonic_levels.empty()) {
                    if (param_name == "level") {
                        if (harmonic < harmonic_levels[0].size()) {
                            return harmonic_levels[0][harmonic];
                        }
                    } else if (param_name == "frequency_ratio") {
                        if (harmonic < harmonic_ratios[0].size()) {
                            return harmonic_ratios[0][harmonic];
                        }
                    } else if (param_name == "phase") {
                        if (harmonic < harmonic_phases[0].size()) {
                            return harmonic_phases[0][harmonic];
                        }
                    }
                }
            } catch (...) {
                // Invalid harmonic number
            }
        }
    }
    
    // Fall back to base class implementation for other parameters
    return SynthArchitectureBase::GetParameter(name);
}

std::vector<double> AdditiveSynth::GetAudioOutput() {
    std::vector<double> output;
    output.resize(audio_output->GetChannelCount(), 0.0);
    
    // Process each active note
    for (size_t note_idx = 0; note_idx < current_note_frequencies.size(); note_idx++) {
        double note_freq = current_note_frequencies[note_idx];
        double note_output = 0.0;
        
        // Sum all harmonics for this note
        for (int h = 0; h < harmonic_count; h++) {
            if (h < harmonic_levels[note_idx].size() && 
                h < harmonic_ratios[note_idx].size() && 
                h < harmonic_phases[note_idx].size()) {
                
                double harmonic_freq = note_freq * harmonic_ratios[note_idx][h];
                double amplitude = harmonic_levels[note_idx][h];
                double phase = harmonic_phases[note_idx][h];
                
                // For simplicity, we'll assume 44.1kHz sample rate and update phase accordingly
                // In a real implementation, this would be handled in a more sophisticated way
                static double phase_accumulator = 0.0;
                phase_accumulator += (harmonic_freq * 2 * M_PI) / 44100.0;
                if (phase_accumulator > 2 * M_PI) {
                    phase_accumulator -= 2 * M_PI;
                }
                
                note_output += amplitude * sin(phase_accumulator + phase);
            }
        }
        
        // Apply amplitude envelope from ADSR
        if (!adsrs.empty()) {
            note_output *= adsrs[0]->GetOutput();
        }
        
        // Add to the overall output
        for (int ch = 0; ch < audio_output->GetChannelCount(); ch++) {
            output[ch] += note_output;
        }
    }
    
    return output;
}

void AdditiveSynth::SetHarmonicLevel(int harmonic, double level) {
    if (harmonic >= 0 && harmonic < harmonic_count) {
        // Set the level for this harmonic across all active notes
        for (auto& levels : harmonic_levels) {
            if (harmonic < levels.size()) {
                levels[harmonic] = std::max(0.0, std::min(1.0, level));
            }
        }
    }
}

void AdditiveSynth::SetHarmonicFrequencyRatio(int harmonic, double ratio) {
    if (harmonic >= 0 && harmonic < harmonic_count) {
        // Set the frequency ratio for this harmonic across all active notes
        for (auto& ratios : harmonic_ratios) {
            if (harmonic < ratios.size()) {
                ratios[harmonic] = std::max(0.01, ratio);  // Minimum 0.01 to avoid zero
            }
        }
    }
}

void AdditiveSynth::SetHarmonicPhase(int harmonic, double phase) {
    if (harmonic >= 0 && harmonic < harmonic_count) {
        // Set the phase for this harmonic across all active notes
        for (auto& phases : harmonic_phases) {
            if (harmonic < phases.size()) {
                phases[harmonic] = fmod(phase, 2 * M_PI);  // Normalize to 0-2π
            }
        }
    }
}