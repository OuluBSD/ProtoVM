#include "PresetManager.h"
#include <iostream>
#include <fstream>

PresetManager::PresetManager() {
    loadFactoryPresets();
}

PresetManager::~PresetManager() {
    // Clean up if needed
}

PresetData PresetManager::createPresetFromSynth(const Synthesizer& synth, const std::string& name, const std::string& description) {
    PresetData preset;
    preset.name = name;
    preset.description = description;
    
    // Note: We can't directly get the current values from Synthesizer as we don't have getter methods
    // In a real implementation, Synthesizer would need getter methods for all parameters
    // For now, we'll just use default values, but in a real implementation, you'd have:
    // preset.waveform = synth.getWaveform();
    // preset.attack = synth.getAttack();
    // etc.
    
    // For this implementation, we'll just return a default preset with the given name
    preset.name = name;
    preset.description = description;
    
    return preset;
}

void PresetManager::applyPresetToSynth(const PresetData& preset, Synthesizer& synth) {
    synth.loadPreset(preset);
}

bool PresetManager::savePresetToFile(const PresetData& preset, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string serialized = serializePreset(preset);
    file << serialized;
    file.close();
    
    return true;
}

bool PresetManager::loadPresetFromFile(const std::string& filePath, PresetData& preset) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string content = buffer.str();
    return deserializePreset(content, preset);
}

void PresetManager::addPreset(const PresetData& preset) {
    presets[preset.name] = std::make_shared<PresetData>(preset);
}

std::shared_ptr<const PresetData> PresetManager::getPresetByName(const std::string& name) const {
    auto it = presets.find(name);
    if (it != presets.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> PresetManager::getAllPresetNames() const {
    std::vector<std::string> names;
    for (const auto& pair : presets) {
        names.push_back(pair.first);
    }
    return names;
}

void PresetManager::loadFactoryPresets() {
    // Add some factory presets
    
    // Sawtooth Lead
    PresetData sawLead;
    sawLead.name = "Sawtooth Lead";
    sawLead.description = "Classic sawtooth lead sound";
    sawLead.waveform = Waveform::SAWTOOTH;
    sawLead.attack = 0.01;
    sawLead.decay = 0.2;
    sawLead.sustain = 0.8;
    sawLead.release = 0.3;
    sawLead.filterCutoff = 0.6;
    sawLead.filterResonance = 0.4;
    sawLead.lfo1Rate = 2.5;
    sawLead.lfo1Depth = 0.1;
    sawLead.modulationConnections.push_back(std::make_tuple(ModulationSource::LFO1, ModulationDestination::OSC_FREQUENCY, 0.05));
    addPreset(sawLead);
    
    // Square Wave Bass
    PresetData squareBass;
    squareBass.name = "Square Wave Bass";
    squareBass.description = "Punchy square wave bass";
    squareBass.waveform = Waveform::SQUARE;
    squareBass.attack = 0.01;
    squareBass.decay = 0.1;
    squareBass.sustain = 0.7;
    squareBass.release = 0.2;
    squareBass.filterCutoff = 0.3;
    squareBass.filterResonance = 0.6;
    addPreset(squareBass);
    
    // Sine Wave Pad
    PresetData sinePad;
    sinePad.name = "Sine Wave Pad";
    sinePad.description = "Warm sine wave pad";
    sinePad.waveform = Waveform::SINE;
    sinePad.attack = 0.5;
    sinePad.decay = 0.3;
    sinePad.sustain = 0.9;
    sinePad.release = 0.5;
    sinePad.filterCutoff = 0.4;
    sinePad.filterResonance = 0.2;
    addPreset(sinePad);
    
    // Triangle Arpeggio
    PresetData triArp;
    triArp.name = "Triangle Arpeggio";
    triArp.description = "Bouncy triangle arpeggio";
    triArp.waveform = Waveform::TRIANGLE;
    triArp.attack = 0.02;
    triArp.decay = 0.15;
    triArp.sustain = 0.5;
    triArp.release = 0.1;
    triArp.filterCutoff = 0.7;
    triArp.filterResonance = 0.3;
    triArp.lfo1Rate = 8.0;
    triArp.lfo1Depth = 0.15;
    triArp.modulationConnections.push_back(std::make_tuple(ModulationSource::LFO1, ModulationDestination::FILTER_CUTOFF, 0.2));
    addPreset(triArp);
}

std::string PresetManager::serializePreset(const PresetData& preset) const {
    std::stringstream ss;
    
    ss << "NAME:" << preset.name << "\n";
    ss << "DESC:" << preset.description << "\n";
    ss << "WAVEFORM:" << static_cast<int>(preset.waveform) << "\n";
    ss << "ADSR:" << preset.attack << "," << preset.decay << "," << preset.sustain << "," << preset.release << "\n";
    ss << "PORTAMENTO:" << preset.portamentoTime << "," << preset.portamentoEnabled << "\n";
    ss << "LFO1:" << preset.lfo1Rate << "," << preset.lfo1Depth << "," << static_cast<int>(preset.lfo1Waveform) << "\n";
    ss << "LFO2:" << preset.lfo2Rate << "," << preset.lfo2Depth << "," << static_cast<int>(preset.lfo2Waveform) << "\n";
    ss << "FILTER:" << preset.filterCutoff << "," << preset.filterResonance << "," << preset.filterType << "\n";
    
    ss << "MODULATIONS:" << preset.modulationConnections.size() << "\n";
    for (const auto& conn : preset.modulationConnections) {
        ss << "MOD:" << static_cast<int>(std::get<0>(conn)) << "," << static_cast<int>(std::get<1>(conn)) << "," << std::get<2>(conn) << "\n";
    }
    
    return ss.str();
}

bool PresetManager::deserializePreset(const std::string& data, PresetData& preset) const {
    std::stringstream ss(data);
    std::string line;
    
    while (std::getline(ss, line)) {
        if (line.substr(0, 5) == "NAME:") {
            preset.name = line.substr(5);
        } else if (line.substr(0, 5) == "DESC:") {
            preset.description = line.substr(5);
        } else if (line.substr(0, 9) == "WAVEFORM:") {
            int wf = std::stoi(line.substr(9));
            preset.waveform = static_cast<Waveform>(wf);
        } else if (line.substr(0, 5) == "ADSR:") {
            std::stringstream params(line.substr(5));
            std::string param;
            std::getline(params, param, ',');
            preset.attack = std::stod(param);
            std::getline(params, param, ',');
            preset.decay = std::stod(param);
            std::getline(params, param, ',');
            preset.sustain = std::stod(param);
            std::getline(params, param, ',');
            preset.release = std::stod(param);
        } else if (line.substr(0, 12) == "PORTAMENTO:") {
            std::stringstream params(line.substr(12));
            std::string param;
            std::getline(params, param, ',');
            preset.portamentoTime = std::stod(param);
            std::getline(params, param, ',');
            preset.portamentoEnabled = std::stoi(param);
        } else if (line.substr(0, 4) == "LFO1:") {
            std::stringstream params(line.substr(4));
            std::string param;
            std::getline(params, param, ',');
            preset.lfo1Rate = std::stod(param);
            std::getline(params, param, ',');
            preset.lfo1Depth = std::stod(param);
            std::getline(params, param, ',');
            preset.lfo1Waveform = static_cast<Waveform>(std::stoi(param));
        } else if (line.substr(0, 4) == "LFO2:") {
            std::stringstream params(line.substr(4));
            std::string param;
            std::getline(params, param, ',');
            preset.lfo2Rate = std::stod(param);
            std::getline(params, param, ',');
            preset.lfo2Depth = std::stod(param);
            std::getline(params, param, ',');
            preset.lfo2Waveform = static_cast<Waveform>(std::stoi(param));
        } else if (line.substr(0, 7) == "FILTER:") {
            std::stringstream params(line.substr(7));
            std::string param;
            std::getline(params, param, ',');
            preset.filterCutoff = std::stod(param);
            std::getline(params, param, ',');
            preset.filterResonance = std::stod(param);
            std::getline(params, param, ',');
            preset.filterType = std::stoi(param);
        } else if (line.substr(0, 12) == "MODULATIONS:") {
            int count = std::stoi(line.substr(12));
            for (int i = 0; i < count; i++) {
                std::getline(ss, line);
                if (line.substr(0, 4) == "MOD:") {
                    std::stringstream modParams(line.substr(4));
                    std::string param;
                    std::getline(modParams, param, ',');
                    ModulationSource src = static_cast<ModulationSource>(std::stoi(param));
                    std::getline(modParams, param, ',');
                    ModulationDestination dest = static_cast<ModulationDestination>(std::stoi(param));
                    std::getline(modParams, param, ',');
                    double amount = std::stod(param);
                    preset.modulationConnections.push_back(std::make_tuple(src, dest, amount));
                }
            }
        }
    }
    
    return true;
}