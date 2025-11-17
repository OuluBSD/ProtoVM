#ifndef _ProtoVM_PresetManager_h_
#define _ProtoVM_PresetManager_h_

#include <string>
#include <vector>
#include <map>
#include <memory>
// Temporarily removing JSON include until proper JSON library is set up
// #include <json/json.h>  // Using JSON library for serialization

// Forward declarations
class VCO;
class VCF;
class VCA;
class LFO;
class ADSR;
class ModulationMatrix;

// Structure to hold a complete synthesizer patch
struct PatchParameters {
    // VCO parameters
    struct VCOParams {
        int waveform_type;
        double frequency;
        double amplitude;
        double fm_amount;
        double pwm_duty_cycle;
        bool anti_aliasing;
    };
    std::vector<VCOParams> vco_params;
    
    // VCF parameters
    struct VCFParams {
        int filter_type;
        double cutoff_freq;
        double resonance;
        double env_amount;
        double key_track_amount;
    };
    VCFParams vcf_params;
    
    // VCA parameters
    struct VCAParams {
        double level;
        bool linear_response;
    };
    VCAParams vca_params;
    
    // LFO parameters
    struct LFOParams {
        int waveform_type;
        double frequency;
        double amplitude;
    };
    std::vector<LFOParams> lfo_params;
    
    // ADSR parameters
    struct ADSRParams {
        double attack;
        double decay;
        double sustain;
        double release;
    };
    std::vector<ADSRParams> adsr_params;
    
    // Modulation matrix parameters
    struct ModulationParams {
        std::vector<struct ConnectionParams> connections;
        struct ConnectionParams {
            int source;
            int destination;
            double amount;
            bool active;
            std::string name;
        };
    };
    ModulationParams modulation_params;
    
    // Additional parameters
    std::string name;
    std::string description;
    std::string author;
    int category;  // e.g., 0=Lead, 1=Bass, 2=Pads, 3=FX, etc.
    double created_timestamp;
};

class PresetManager {
public:
    PresetManager();
    virtual ~PresetManager();
    
    // Create a new preset from current synthesizer state
    bool CaptureCurrentState(const std::string& name, const std::string& description = "");
    
    // Create a preset from explicit parameters
    bool CreatePreset(const PatchParameters& params, const std::string& name);
    
    // Load a preset by name
    bool LoadPreset(const std::string& name);
    
    // Load a preset by index
    bool LoadPreset(int index);
    
    // Apply a preset to the synthesizer
    bool ApplyPreset(const PatchParameters& params);
    
    // Save all presets to a file
    bool SavePresetsToFile(const std::string& filepath);
    
    // Load presets from a file
    bool LoadPresetsFromFile(const std::string& filepath);
    
    // List all available presets
    std::vector<std::string> GetPresetNames() const;
    
    // Get preset by name
    PatchParameters* GetPreset(const std::string& name);
    
    // Get preset by index
    PatchParameters* GetPreset(int index);
    
    // Delete a preset by name
    bool DeletePreset(const std::string& name);
    
    // Delete a preset by index
    bool DeletePreset(int index);
    
    // Get number of presets
    int GetPresetCount() const { return presets.size(); }
    
    // Get current preset
    const PatchParameters* GetCurrentPreset() const { return current_preset; }
    
    // Serialize parameters (placeholder - not implemented without JSON dependency)
    void* SerializeParameters(const PatchParameters& params) const;

    // Deserialize parameters (placeholder - not implemented without JSON dependency)
    PatchParameters DeserializeParameters(void* json) const;
    
    // Set a callback function for when a preset is loaded
    void SetPresetLoadedCallback(std::function<void(const PatchParameters& params)> callback);
    
private:
    std::map<std::string, PatchParameters> presets;  // Presets by name
    std::vector<std::string> preset_order;           // Order of presets for indexing
    PatchParameters* current_preset;                 // Currently loaded preset
    std::function<void(const PatchParameters& params)> preset_loaded_callback;  // Callback when preset is loaded
    
    // Helper function to normalize preset name
    std::string NormalizePresetName(const std::string& name) const;
    
    // Helper function to check if name is valid
    bool IsValidPresetName(const std::string& name) const;
};

#endif