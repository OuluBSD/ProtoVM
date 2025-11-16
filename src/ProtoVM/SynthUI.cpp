#include "SynthUI.h"
#include <algorithm>

SynthUI::SynthUI(PolyphonyManager* synth_engine)
    : synth_engine(synth_engine)
{
    // Initialize default parameters
    InitializeDefaultParameters();
    
    // Map default controls
    MapDefaultControls();
}

SynthUI::~SynthUI() {
    // Clean up handled by parent class
}

bool SynthUI::Tick() {
    // Update the synth engine with current UI values
    if (synth_engine) {
        UpdateSynthEngine();
    }
    
    return true;
}

int SynthUI::AddParameter(const UIParameter& param) {
    parameters.push_back(param);
    param_mappings.resize(parameters.size()); // Add a new mapping vector for this parameter
    return static_cast<int>(parameters.size() - 1);
}

UIParameter* SynthUI::GetParameter(int id) {
    if (id >= 0 && id < static_cast<int>(parameters.size())) {
        return &parameters[id];
    }
    return nullptr;
}

bool SynthUI::SetParameterValue(int id, double value) {
    if (id >= 0 && id < static_cast<int>(parameters.size())) {
        // Clamp value to valid range
        value = std::max(parameters[id].min_value, std::min(parameters[id].max_value, value));
        parameters[id].current_value = value;
        return true;
    }
    return false;
}

bool SynthUI::SetParameterByName(const std::string& name, double value) {
    int id = FindParameterByName(name);
    if (id >= 0) {
        return SetParameterValue(id, value);
    }
    return false;
}

double SynthUI::GetParameterValue(int id) const {
    if (id >= 0 && id < static_cast<int>(parameters.size())) {
        return parameters[id].current_value;
    }
    return 0.0;
}

double SynthUI::GetParameterByName(const std::string& name) const {
    int id = FindParameterByName(name);
    if (id >= 0) {
        return GetParameterValue(id);
    }
    return 0.0;
}

void SynthUI::AddControlMapping(int param_id, const UIControlMapping& mapping) {
    if (param_id >= 0 && param_id < static_cast<int>(param_mappings.size())) {
        param_mappings[param_id].push_back(mapping);
    }
}

void SynthUI::UpdateSynthEngine() {
    if (!synth_engine) return;
    
    // Update all components based on parameters
    UpdateVCOs();
    UpdateVCFs();
    UpdateVCAs();
    UpdateLFOs();
    UpdateADSRs();
}

void SynthUI::CreateDefaultLayout() {
    // Create a standard subtractive synthesizer layout
    // This would define positions and visual elements in a GUI context
    // For now, we just ensure all default parameters are defined
    InitializeDefaultParameters();
}

void SynthUI::HandleEvent(const std::string& event_type, int param_id, double value) {
    if (event_type == "VALUE_CHANGED") {
        SetParameterValue(param_id, value);
    }
}

void SynthUI::SetSynthEngine(PolyphonyManager* engine) {
    synth_engine = engine;
}

void SynthUI::InitializeDefaultParameters() {
    parameters.clear();
    param_mappings.clear();
    
    // VCO parameters
    AddParameter(UIParameter("VCO1 Frequency", 20.0, 20000.0, 440.0, UIControlType::KNOB, "Hz"));
    AddParameter(UIParameter("VCO1 Waveform", 0.0, 4.0, 1.0, UIControlType::WAVEFORM_SELECTOR));
    AddParameter(UIParameter("VCO1 Detune", -50.0, 50.0, 0.0, UIControlType::KNOB, "cents"));
    AddParameter(UIParameter("VCO2 Frequency", 20.0, 20000.0, 440.0, UIControlType::KNOB, "Hz"));
    AddParameter(UIParameter("VCO2 Waveform", 0.0, 4.0, 2.0, UIControlType::WAVEFORM_SELECTOR));
    AddParameter(UIParameter("VCO2 Detune", -50.0, 50.0, 0.0, UIControlType::KNOB, "cents"));
    
    // VCF parameters
    AddParameter(UIParameter("Filter Cutoff", 20.0, 20000.0, 2000.0, UIControlType::KNOB, "Hz"));
    AddParameter(UIParameter("Filter Resonance", 0.1, 10.0, 0.7, UIControlType::KNOB));
    AddParameter(UIParameter("Filter Type", 0.0, 4.0, 0.0, UIControlType::WAVEFORM_SELECTOR));
    AddParameter(UIParameter("Filter Drive", 0.0, 2.0, 0.5, UIControlType::KNOB));
    
    // VCA parameters
    AddParameter(UIParameter("Amplifier Level", 0.0, 1.0, 0.8, UIControlType::SLIDER));
    
    // ADSR parameters
    AddParameter(UIParameter("Attack Time", 0.001, 5.0, 0.1, UIControlType::KNOB, "s"));
    AddParameter(UIParameter("Decay Time", 0.001, 5.0, 0.2, UIControlType::KNOB, "s"));
    AddParameter(UIParameter("Sustain Level", 0.0, 1.0, 0.7, UIControlType::KNOB));
    AddParameter(UIParameter("Release Time", 0.001, 5.0, 0.3, UIControlType::KNOB, "s"));
    
    // LFO parameters
    AddParameter(UIParameter("LFO Rate", 0.1, 20.0, 5.0, UIControlType::KNOB, "Hz"));
    AddParameter(UIParameter("LFO Depth", 0.0, 1.0, 0.3, UIControlType::KNOB));
    AddParameter(UIParameter("LFO Destination", 0.0, 2.0, 0.0, UIControlType::WAVEFORM_SELECTOR));
    
    // Modulation parameters
    AddParameter(UIParameter("Pitch Mod Wheel", 0.0, 2.0, 0.0, UIControlType::KNOB, "semitones"));
    AddParameter(UIParameter("Filter Mod Wheel", 0.0, 1.0, 0.0, UIControlType::KNOB));
    AddParameter(UIParameter("Vibrato Depth", 0.0, 1.0, 0.0, UIControlType::KNOB));
    AddParameter(UIParameter("Portamento Time", 0.0, 1.0, 0.0, UIControlType::KNOB, "s"));
    
    // Voice/polyphony parameters
    AddParameter(UIParameter("Voice Count", 1.0, 16.0, 8.0, UIControlType::KNOB));
    AddParameter(UIParameter("Voice Allocation", 0.0, 3.0, 0.0, UIControlType::WAVEFORM_SELECTOR));
    AddParameter(UIParameter("Legato Mode", 0.0, 1.0, 0.0, UIControlType::TOGGLE));
}

void SynthUI::MapDefaultControls() {
    // Map VCO parameters
    AddControlMapping(VCO1_FREQ, UIControlMapping("VCO1", "Frequency"));
    AddControlMapping(VCO1_WAVEFORM, UIControlMapping("VCO1", "Waveform"));
    AddControlMapping(VCO1_DETUNE, UIControlMapping("VCO1", "Detune"));
    AddControlMapping(VCO2_FREQ, UIControlMapping("VCO2", "Frequency"));
    AddControlMapping(VCO2_WAVEFORM, UIControlMapping("VCO2", "Waveform"));
    AddControlMapping(VCO2_DETUNE, UIControlMapping("VCO2", "Detune"));
    
    // Map VCF parameters
    AddControlMapping(FILTER_CUTOFF, UIControlMapping("VCF", "Cutoff"));
    AddControlMapping(FILTER_RESONANCE, UIControlMapping("VCF", "Resonance"));
    AddControlMapping(FILTER_TYPE, UIControlMapping("VCF", "Type"));
    AddControlMapping(FILTER_DRIVE, UIControlMapping("VCF", "Drive"));
    
    // Map VCA parameters
    AddControlMapping(AMP_LEVEL, UIControlMapping("VCA", "Level"));
    
    // Map ADSR parameters
    AddControlMapping(ATTACK_TIME, UIControlMapping("ADSR", "Attack"));
    AddControlMapping(DECAY_TIME, UIControlMapping("ADSR", "Decay"));
    AddControlMapping(SUSTAIN_LEVEL, UIControlMapping("ADSR", "Sustain"));
    AddControlMapping(RELEASE_TIME, UIControlMapping("ADSR", "Release"));
    
    // Map LFO parameters
    AddControlMapping(LFO_RATE, UIControlMapping("LFO", "Rate"));
    AddControlMapping(LFO_DEPTH, UIControlMapping("LFO", "Depth"));
    AddControlMapping(LFO_DESTINATION, UIControlMapping("LFO", "Destination"));
    
    // Map modulation parameters
    AddControlMapping(PITCH_MOD_WHEEL, UIControlMapping("Modulation", "Pitch"));
    AddControlMapping(FILTER_MOD_WHEEL, UIControlMapping("Modulation", "Filter"));
    AddControlMapping(VIBRATO_DEPTH, UIControlMapping("Modulation", "Vibrato"));
    AddControlMapping(PORTAMENTO_TIME, UIControlMapping("Modulation", "Portamento"));
    
    // Map voice parameters
    AddControlMapping(VOICE_COUNT, UIControlMapping("Polyphony", "VoiceCount"));
    AddControlMapping(VOICE_ALLOCATION, UIControlMapping("Polyphony", "Allocation"));
    AddControlMapping(LEGATO_MODE, UIControlMapping("Polyphony", "Legato"));
}

void SynthUI::UpdateVCOs() {
    // Update VCO parameters in the synth engine
    if (!synth_engine) return;
    
    // For each active voice, update its VCOs
    int active_voices = synth_engine->GetActiveVoicesCount();
    for (int i = 0; i < synth_engine->GetMaxVoices(); i++) {
        Voice* voice = synth_engine->GetVoice(i);
        if (!voice || !voice->path) continue;
        
        // Update VCO1
        AnalogNodeBase* vco1_base = voice->path->GetComponent(0);  // VCO is first component
        if (vco1_base) {
            // Cast to VCO and update parameters
            VCO* vco1 = dynamic_cast<VCO*>(vco1_base);
            if (vco1) {
                // Update frequency
                vco1->SetBaseFrequency(GetParameterValue(VCO1_FREQ));
                
                // Update waveform (this would need proper handling)
                VCOType waveform = static_cast<VCOType>(static_cast<int>(GetParameterValue(VCO1_WAVEFORM)));
                vco1->SetType(waveform);
                
                // Update detune (this would need proper implementation)
                // vco1->SetDetune(GetParameterValue(VCO1_DETUNE));
            }
        }
        
        // Update VCO2 (if available - would need to be added to the signal path)
        // This is more complex as it requires the signal path to have a second VCO
    }
}

void SynthUI::UpdateVCFs() {
    // Update VCF parameters in the synth engine
    if (!synth_engine) return;
    
    // For each active voice, update its VCF
    for (int i = 0; i < synth_engine->GetMaxVoices(); i++) {
        Voice* voice = synth_engine->GetVoice(i);
        if (!voice || !voice->path) continue;
        
        // Update VCF
        AnalogNodeBase* vcf_base = voice->path->GetComponent(2);  // Assuming VCF is 3rd component
        if (vcf_base) {
            VCF* vcf = dynamic_cast<VCF*>(vcf_base);
            if (vcf) {
                vcf->SetCutoffFrequency(GetParameterValue(FILTER_CUTOFF));
                vcf->SetResonance(GetParameterValue(FILTER_RESONANCE));
                
                // Set filter type
                FilterType filter_type = static_cast<FilterType>(static_cast<int>(GetParameterValue(FILTER_TYPE)));
                vcf->SetType(filter_type);
                
                // Set drive
                vcf->SetDrive(GetParameterValue(FILTER_DRIVE));
            }
        }
    }
}

void SynthUI::UpdateVCAs() {
    // Update VCA parameters in the synth engine
    if (!synth_engine) return;
    
    // For each active voice, update its VCA
    for (int i = 0; i < synth_engine->GetMaxVoices(); i++) {
        Voice* voice = synth_engine->GetVoice(i);
        if (!voice || !voice->path) continue;
        
        // Update VCA
        AnalogNodeBase* vca_base = voice->path->GetComponent(3);  // Assuming VCA is 4th component
        if (vca_base) {
            VCA* vca = dynamic_cast<VCA*>(vca_base);
            if (vca) {
                vca->SetGain(GetParameterValue(AMP_LEVEL));
            }
        }
    }
}

void SynthUI::UpdateLFOs() {
    // Update LFO parameters in the synth engine
    if (!synth_engine) return;
    
    // For each active voice, update its LFO
    for (int i = 0; i < synth_engine->GetMaxVoices(); i++) {
        Voice* voice = synth_engine->GetVoice(i);
        if (!voice || !voice->path) continue;
        
        // Update LFO
        AnalogNodeBase* lfo_base = voice->path->GetComponent(1);  // Assuming LFO is 2nd component
        if (lfo_base) {
            LFO* lfo = dynamic_cast<LFO*>(lfo_base);
            if (lfo) {
                lfo->SetFrequency(GetParameterValue(LFO_RATE));
                
                // Update waveform based on destination parameter
                LFOType lfo_type = static_cast<LFOType>(static_cast<int>(GetParameterValue(LFO_DESTINATION)));
                lfo->SetType(lfo_type);
            }
        }
    }
}

void SynthUI::UpdateADSRs() {
    // Update ADSR parameters in the synth engine
    if (!synth_engine) return;
    
    // For each active voice, update its ADSR
    for (int i = 0; i < synth_engine->GetMaxVoices(); i++) {
        Voice* voice = synth_engine->GetVoice(i);
        if (!voice || !voice->path) continue;
        
        // Update ADSR
        AnalogNodeBase* adsr_base = voice->path->GetComponent(4);  // Assuming ADSR is 5th component
        if (adsr_base) {
            ADSR* adsr = dynamic_cast<ADSR*>(adsr_base);
            if (adsr) {
                adsr->SetAttack(GetParameterValue(ATTACK_TIME));
                adsr->SetDecay(GetParameterValue(DECAY_TIME));
                adsr->SetSustain(GetParameterValue(SUSTAIN_LEVEL));
                adsr->SetRelease(GetParameterValue(RELEASE_TIME));
            }
        }
    }
}

int SynthUI::FindParameterByName(const std::string& name) const {
    for (size_t i = 0; i < parameters.size(); i++) {
        if (parameters[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}