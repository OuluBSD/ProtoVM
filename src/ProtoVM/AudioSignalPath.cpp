#include "AudioSignalPath.h"
#include <cmath>
#include <algorithm>

AudioSignalPath::AudioSignalPath(SignalPathType type)
    : type(type)
    , final_output(0.0)
    , master_volume(0.8)
{
    // Initialize based on signal path type
    switch (type) {
        case SignalPathType::SYNTH_VOICE:
            // Basic monophonic voice: VCO -> VCF -> VCA
            {
                // Create VCO
                VCO* vco = new VCO(VCOType::SAWTOOTH, 440.0);
                components.push_back(vco);
                
                // Create VCF
                VCF* vcf = new VCF(FilterType::LOWPASS, FilterImplementation::MOOG_LADDER, 2000.0, 0.7);
                components.push_back(vcf);
                
                // Create VCA
                VCA* vca = new VCA(VCACharacteristic::EXPONENTIAL, 1.0);
                components.push_back(vca);
                
                // Set up default routing: VCO -> VCF -> VCA -> output
                SignalRoute route1 = {0, 1, 1.0, true};  // VCO (0) -> VCF (1)
                SignalRoute route2 = {1, 2, 1.0, true};  // VCF (1) -> VCA (2)
                SignalRoute route3 = {2, -1, 1.0, true}; // VCA (2) -> output (-1)
                
                routing.push_back(route1);
                routing.push_back(route2);
                routing.push_back(route3);
                
                inputs.resize(3, 0.0);  // 3 components
            }
            break;
            
        case SignalPathType::DUAL_OSC_VOICE:
            // Two VCOs mixed into VCF -> VCA
            {
                // Create two VCOs
                VCO* vco1 = new VCO(VCOType::SAWTOOTH, 440.0);
                VCO* vco2 = new VCO(VCOType::PULSE, 440.0);
                components.push_back(vco1);
                components.push_back(vco2);
                
                // Create mixer (would be implemented as a simple component)
                // For now, we'll just handle mixing in software
                components.push_back(nullptr);  // Placeholder for mixer
                
                // Create VCF
                VCF* vcf = new VCF(FilterType::LOWPASS, FilterImplementation::MOOG_LADDER, 2000.0, 0.7);
                components.push_back(vcf);
                
                // Create VCA
                VCA* vca = new VCA(VCACharacteristic::EXPONENTIAL, 1.0);
                components.push_back(vca);
                
                // Set up routing
                SignalRoute route1 = {0, 2, 0.5, true};  // VCO1 (0) -> Mixer (2)
                SignalRoute route2 = {1, 2, 0.5, true};  // VCO2 (1) -> Mixer (2)
                SignalRoute route3 = {2, 3, 1.0, true};  // Mixer (2) -> VCF (3)
                SignalRoute route4 = {3, 4, 1.0, true};  // VCF (3) -> VCA (4)
                SignalRoute route5 = {4, -1, 1.0, true}; // VCA (4) -> output (-1)
                
                routing.push_back(route1);
                routing.push_back(route2);
                routing.push_back(route3);
                routing.push_back(route4);
                routing.push_back(route5);
                
                inputs.resize(5, 0.0);  // 5 components (with placeholder)
            }
            break;
            
        case SignalPathType::VINTAGE_MONO_SYNTH:
            // Vintage monophonic synth: VCO -> VCF -> VCA with additional modulation
            {
                // Create VCO
                VCO* vco = new VCO(VCOType::SAWTOOTH, 440.0);
                components.push_back(vco);
                
                // Create LFO
                LFO* lfo = new LFO(LFOType::SINE, 5.0);
                components.push_back(lfo);
                
                // Create VCF
                VCF* vcf = new VCF(FilterType::LOWPASS, FilterImplementation::MOOG_LADDER, 2000.0, 0.7);
                components.push_back(vcf);
                
                // Create VCA
                VCA* vca = new VCA(VCACharacteristic::EXPONENTIAL, 1.0);
                components.push_back(vca);
                
                // Create ADSR
                ADSR* adsr = new ADSR(0.1, 0.2, 0.7, 0.3);
                components.push_back(adsr);
                
                // Set up routing: VCO -> VCF -> VCA, with LFO modulating VCF, ADSR modulating VCA
                SignalRoute route1 = {0, 2, 1.0, true};  // VCO (0) -> VCF (2)
                SignalRoute route2 = {2, 3, 1.0, true};  // VCF (2) -> VCA (3)
                SignalRoute route3 = {4, 3, 0.5, true};  // ADSR (4) -> VCA (3) (gain modulation)
                SignalRoute route4 = {4, 2, 0.3, true};  // ADSR (4) -> VCF (2) (filter modulation)
                SignalRoute route5 = {1, 2, 0.2, true};  // LFO (1) -> VCF (2) (filter modulation)
                SignalRoute route6 = {5, -1, 1.0, true}; // VCA (3) -> output (-1)
                
                routing.push_back(route1);
                routing.push_back(route2);
                routing.push_back(route3);
                routing.push_back(route4);
                routing.push_back(route5);
                routing.push_back(route6);
                
                inputs.resize(5, 0.0);  // 5 components
            }
            break;
            
        case SignalPathType::MODULAR_PATCH:
        case SignalPathType::CUSTOM_PATH:
        default:
            // Initialize with no components
            inputs.resize(0);
            break;
    }
}

bool AudioSignalPath::Tick() {
    // Process signal path based on type
    switch (type) {
        case SignalPathType::SYNTH_VOICE:
            ProcessSynthVoice();
            break;
            
        case SignalPathType::DUAL_OSC_VOICE:
            ProcessDualOscVoice();
            break;
            
        case SignalPathType::VINTAGE_MONO_SYNTH:
            ProcessVintageMonoSynth();
            break;
            
        case SignalPathType::MODULAR_PATCH:
            ProcessModularPatch();
            break;
            
        case SignalPathType::CUSTOM_PATH:
            ProcessCustomPath();
            break;
    }
    
    return true;
}

void AudioSignalPath::SetType(SignalPathType type) {
    // Clean up existing components
    for (auto* comp : components) {
        if (comp) {
            delete comp;
        }
    }
    components.clear();
    routing.clear();
    inputs.clear();
    
    // Set the new type and reinitialize
    this->type = type;
    
    // Reinitialize based on new type
    switch (type) {
        case SignalPathType::SYNTH_VOICE:
            // Basic monophonic voice: VCO -> VCF -> VCA
            {
                VCO* vco = new VCO(VCOType::SAWTOOTH, 440.0);
                components.push_back(vco);
                
                VCF* vcf = new VCF(FilterType::LOWPASS, FilterImplementation::MOOG_LADDER, 2000.0, 0.7);
                components.push_back(vcf);
                
                VCA* vca = new VCA(VCACharacteristic::EXPONENTIAL, 1.0);
                components.push_back(vca);
                
                SignalRoute route1 = {0, 1, 1.0, true};
                SignalRoute route2 = {1, 2, 1.0, true};
                SignalRoute route3 = {2, -1, 1.0, true};
                
                routing.push_back(route1);
                routing.push_back(route2);
                routing.push_back(route3);
                
                inputs.resize(3, 0.0);
            }
            break;
            
        default:
            // For other types, just clear
            break;
    }
}

void AudioSignalPath::AddComponent(AnalogNodeBase* component) {
    components.push_back(component);
    inputs.resize(components.size(), 0.0);
}

AnalogNodeBase* AudioSignalPath::GetComponent(int index) {
    if (index >= 0 && index < static_cast<int>(components.size())) {
        return components[index];
    }
    return nullptr;
}

void AudioSignalPath::SetInput(int input_index, double input) {
    if (input_index >= 0 && input_index < static_cast<int>(inputs.size())) {
        inputs[input_index] = input;
    }
}

bool AudioSignalPath::Connect(int source_idx, int dest_idx, double gain) {
    // Check if the connection already exists
    for (auto& route : routing) {
        if (route.source_component == source_idx && route.destination == dest_idx) {
            route.gain = gain;
            route.active = true;
            return true;
        }
    }
    
    // If not found, add a new connection
    if (source_idx >= 0 && source_idx < static_cast<int>(components.size()) &&
        dest_idx >= -1 && dest_idx < static_cast<int>(components.size())) {
        SignalRoute new_route = {source_idx, dest_idx, gain, true};
        routing.push_back(new_route);
        return true;
    }
    
    return false;
}

bool AudioSignalPath::Disconnect(int source_idx, int dest_idx) {
    for (auto it = routing.begin(); it != routing.end(); ++it) {
        if (it->source_component == source_idx && it->destination == dest_idx) {
            routing.erase(it);
            return true;
        }
    }
    return false;
}

void AudioSignalPath::SetRouting(const std::vector<SignalRoute>& routes) {
    routing = routes;
}

void AudioSignalPath::UpdateSignalPath() {
    // This method would update the signal path based on current connections
    // For complex routing, we might need to recompute the path each tick
    ApplyRouting();
}

void AudioSignalPath::ProcessSynthVoice() {
    // Process VCO
    if (components.size() >= 1 && components[0] != nullptr) {
        components[0]->Tick();  // VCO
    }
    
    // Get VCO output and process VCF
    if (components.size() >= 2 && components[1] != nullptr) {
        // Get VCO output and set as input to VCF
        VCO* vco = dynamic_cast<VCO*>(components[0]);
        VCF* vcf = dynamic_cast<VCF*>(components[1]);
        if (vco && vcf) {
            vcf->SetInput(vco->GetOutput());
        }
        
        components[1]->Tick();  // VCF
    }
    
    // Process VCA
    if (components.size() >= 3 && components[2] != nullptr) {
        // Get VCF output and set as input to VCA
        VCF* vcf = dynamic_cast<VCF*>(components[1]);
        VCA* vca = dynamic_cast<VCA*>(components[2]);
        if (vcf && vca) {
            vca->SetInput(vcf->GetOutput());
        }
        
        components[2]->Tick();  // VCA
    }
    
    // Set final output
    if (components.size() >= 3 && components[2] != nullptr) {
        VCA* vca = dynamic_cast<VCA*>(components[2]);
        if (vca) {
            final_output = vca->GetOutput() * master_volume;
        }
    } else {
        final_output = 0.0;
    }
}

void AudioSignalPath::ProcessDualOscVoice() {
    // Process both VCOs
    if (components.size() >= 2 && components[0] != nullptr && components[1] != nullptr) {
        components[0]->Tick();  // VCO1
        components[1]->Tick();  // VCO2
    }
    
    // Mix outputs of both VCOs
    double mixed_output = 0.0;
    VCO* vco1 = nullptr;
    VCO* vco2 = nullptr;
    
    if (components[0] != nullptr) {
        vco1 = dynamic_cast<VCO*>(components[0]);
        if (vco1) mixed_output += vco1->GetOutput() * 0.5;
    }
    
    if (components[1] != nullptr) {
        vco2 = dynamic_cast<VCO*>(components[1]);
        if (vco2) mixed_output += vco2->GetOutput() * 0.5;
    }
    
    // Process VCF
    if (components.size() >= 4 && components[3] != nullptr) {
        VCF* vcf = dynamic_cast<VCF*>(components[3]);
        if (vcf) {
            vcf->SetInput(mixed_output);
        }
        components[3]->Tick();  // VCF
    }
    
    // Process VCA
    if (components.size() >= 5 && components[4] != nullptr) {
        VCF* vcf = dynamic_cast<VCF*>(components[3]);
        VCA* vca = dynamic_cast<VCA*>(components[4]);
        if (vcf && vca) {
            vca->SetInput(vcf->GetOutput());
        }
        components[4]->Tick();  // VCA
    }
    
    // Set final output
    if (components.size() >= 5 && components[4] != nullptr) {
        VCA* vca = dynamic_cast<VCA*>(components[4]);
        if (vca) {
            final_output = vca->GetOutput() * master_volume;
        }
    } else {
        final_output = 0.0;
    }
}

void AudioSignalPath::ProcessVintageMonoSynth() {
    // Process VCO
    if (components.size() >= 1 && components[0] != nullptr) {
        components[0]->Tick();
    }
    
    // Process LFO
    if (components.size() >= 2 && components[1] != nullptr) {
        components[1]->Tick();
    }
    
    // Process ADSR
    if (components.size() >= 5 && components[4] != nullptr) {
        components[4]->Tick();
    }
    
    // Process VCF with modulation from LFO and ADSR
    if (components.size() >= 3 && components[2] != nullptr) {
        VCO* vco = dynamic_cast<VCO*>(components[0]);
        LFO* lfo = dynamic_cast<LFO*>(components[1]);
        ADSR* adsr = dynamic_cast<ADSR*>(components[4]);
        VCF* vcf = dynamic_cast<VCF*>(components[2]);
        
        if (vco && vcf) {
            // Apply LFO modulation to cutoff
            if (lfo) {
                double lfo_mod = lfo->GetOutput() * 0.1; // 10% modulation depth
                vcf->SetCutoffFrequency(vcf->GetCutoffFrequency() * (1.0 + lfo_mod));
            }
            
            // Apply ADSR filter envelope
            if (adsr) {
                double env_mod = adsr->GetOutput() * 0.3; // 30% modulation depth
                vcf->SetCutoffFrequency(vcf->GetCutoffFrequency() * (1.0 + env_mod));
            }
            
            vcf->SetInput(vco->GetOutput());
        }
        
        components[2]->Tick();
    }
    
    // Process VCA with ADSR modulation
    if (components.size() >= 4 && components[3] != nullptr) {
        VCF* vcf = dynamic_cast<VCF*>(components[2]);
        ADSR* adsr = dynamic_cast<ADSR*>(components[4]);
        VCA* vca = dynamic_cast<VCA*>(components[3]);
        
        if (vcf && vca) {
            vca->SetInput(vcf->GetOutput());
        }
        
        if (adsr && vca) {
            // Use ADSR output to control VCA gain
            double env_level = adsr->GetOutput();
            vca->SetGain(env_level * 0.8); // Scale to prevent clipping
        }
        
        components[3]->Tick();
    }
    
    // Set final output
    if (components.size() >= 4 && components[3] != nullptr) {
        VCA* vca = dynamic_cast<VCA*>(components[3]);
        if (vca) {
            final_output = vca->GetOutput() * master_volume;
        }
    } else {
        final_output = 0.0;
    }
}

void AudioSignalPath::ProcessModularPatch() {
    // Process all routed connections in the patch
    ApplyRouting();
}

void AudioSignalPath::ProcessCustomPath() {
    // Process custom path based on routing configuration
    ApplyRouting();
}

void AudioSignalPath::ApplyRouting() {
    // Reset inputs for this tick
    std::fill(inputs.begin(), inputs.end(), 0.0);
    
    // Process each route
    for (const auto& route : routing) {
        if (!route.active) continue;
        
        // Get source output
        if (route.source_component >= 0 && 
            route.source_component < static_cast<int>(components.size()) &&
            components[route.source_component] != nullptr) {
            
            double source_output = 0.0;
            
            // Get output depending on component type
            VCO* vco = dynamic_cast<VCO*>(components[route.source_component]);
            LFO* lfo = dynamic_cast<LFO*>(components[route.source_component]);
            ADSR* adsr = dynamic_cast<ADSR*>(components[route.source_component]);
            VCF* vcf = dynamic_cast<VCF*>(components[route.source_component]);
            VCA* vca = dynamic_cast<VCA*>(components[route.source_component]);
            
            if (vco) source_output = vco->GetOutput();
            else if (lfo) source_output = lfo->GetOutput();
            else if (adsr) source_output = adsr->GetOutput();
            else if (vcf) source_output = vcf->GetOutput();
            else if (vca) source_output = vca->GetOutput();
            
            // Apply gain
            source_output *= route.gain;
            
            // Send to destination
            if (route.destination == -1) {
                // To final output
                final_output += source_output;
            } else if (route.destination < static_cast<int>(inputs.size())) {
                // To another component's input
                inputs[route.destination] += source_output;
                
                // Set this as input for the next tick
                if (route.destination < static_cast<int>(components.size()) && 
                    components[route.destination] != nullptr) {
                    
                    VCF* dest_vcf = dynamic_cast<VCF*>(components[route.destination]);
                    VCA* dest_vca = dynamic_cast<VCA*>(components[route.destination]);
                    
                    if (dest_vcf) dest_vcf->SetInput(inputs[route.destination]);
                    else if (dest_vca) dest_vca->SetInput(inputs[route.destination]);
                }
            }
        }
    }
    
    // Process all components
    for (auto& comp : components) {
        if (comp) {
            comp->Tick();
        }
    }
    
    // Apply master volume
    final_output *= master_volume;
}

double AudioSignalPath::GetFrequencyResponse(double frequency) const {
    // Calculate approximate frequency response at the given frequency
    // This is a simplified model - in a real implementation, we would
    // run the signal through the path and measure the response
    
    // For a basic response, we'll consider the last VCF in the chain
    for (int i = components.size() - 1; i >= 0; i--) {
        VCF* vcf = dynamic_cast<VCF*>(components[i]);
        if (vcf) {
            // Simplified low-pass filter response
            double cutoff = vcf->GetCutoffFrequency();
            double resonance = vcf->GetResonance();
            
            // Calculate response based on filter type
            double response = 1.0 / sqrt(1 + pow(frequency / cutoff, 2));
            
            // Add resonance peak if frequency is near cutoff
            if (frequency > cutoff * 0.8 && frequency < cutoff * 1.2) {
                response *= (1.0 + resonance * 0.2);
            }
            
            return response;
        }
    }
    
    // If no filter, return 0dB (1.0)
    return 1.0;
}

double AudioSignalPath::GetLatency() const {
    // Calculate approximate total latency of the signal path
    // This is roughly the number of components in series
    // times the processing time per component
    
    // For a simple estimate, return rough latency based on path complexity
    switch (type) {
        case SignalPathType::SYNTH_VOICE:
            return 3.0 * (1.0 / 44100.0);  // ~3 components at 44.1kHz sample rate
        case SignalPathType::DUAL_OSC_VOICE:
            return 5.0 * (1.0 / 44100.0);  // ~5 components
        case SignalPathType::VINTAGE_MONO_SYNTH:
            return 5.0 * (1.0 / 44100.0);  // ~5 components
        case SignalPathType::MODULAR_PATCH:
        case SignalPathType::CUSTOM_PATH:
        default:
            return static_cast<double>(components.size()) * (1.0 / 44100.0);
    }
}