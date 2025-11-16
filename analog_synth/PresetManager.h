#ifndef PRESET_MANAGER_H
#define PRESET_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <sstream>
#include "Synthesizer.h"

struct PresetData {
    // Oscillator settings
    Waveform waveform = Waveform::SAWTOOTH;
    
    // ADSR settings
    double attack = 0.1;
    double decay = 0.2;
    double sustain = 0.7;
    double release = 0.3;
    
    // Portamento settings
    double portamentoTime = 0.1;
    bool portamentoEnabled = true;
    
    // LFO settings
    double lfo1Rate = 5.0;
    double lfo1Depth = 0.3;
    Waveform lfo1Waveform = Waveform::SINE;
    double lfo2Rate = 0.5;
    double lfo2Depth = 0.1;
    Waveform lfo2Waveform = Waveform::TRIANGLE;
    
    // Filter settings
    double filterCutoff = 0.7;
    double filterResonance = 0.3;
    int filterType = 0;  // 0=lowpass, 1=hipass, 2=bandpass, 3=notch
    
    // Modulation matrix connections
    std::vector<std::tuple<ModulationSource, ModulationDestination, double>> modulationConnections;
    
    // Name of the preset
    std::string name = "Default";
    
    // Description of the preset
    std::string description = "Default preset";
};

class PresetManager {
public:
    PresetManager();
    ~PresetManager();
    
    // Create a preset from current synthesizer state
    PresetData createPresetFromSynth(const Synthesizer& synth, const std::string& name, const std::string& description = "");
    
    // Apply a preset to a synthesizer
    void applyPresetToSynth(const PresetData& preset, Synthesizer& synth);
    
    // Save a preset to a file
    bool savePresetToFile(const PresetData& preset, const std::string& filePath);
    
    // Load a preset from a file
    bool loadPresetFromFile(const std::string& filePath, PresetData& preset);
    
    // Add preset to internal library
    void addPreset(const PresetData& preset);
    
    // Get preset by name
    std::shared_ptr<const PresetData> getPresetByName(const std::string& name) const;
    
    // Get all preset names
    std::vector<std::string> getAllPresetNames() const;
    
    // Load all factory presets
    void loadFactoryPresets();
    
private:
    std::map<std::string, std::shared_ptr<PresetData>> presets;
    
    // Helper function to serialize preset data to string
    std::string serializePreset(const PresetData& preset) const;
    
    // Helper function to deserialize preset data from string
    bool deserializePreset(const std::string& data, PresetData& preset) const;
};

#endif // PRESET_MANAGER_H