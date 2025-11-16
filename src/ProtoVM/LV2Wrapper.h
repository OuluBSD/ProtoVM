#ifndef _ProtoVM_LV2Wrapper_h_
#define _ProtoVM_LV2Wrapper_h_

#include "AnalogCommon.h"
#include "ParameterAutomation.h"
#include <vector>
#include <string>
#include <map>

// Forward declarations
class AudioSignalPath;
class TubeAudioInput;
class TubeAudioOutput;

// Enum for different LV2 port types
enum class LV2PortType {
    AUDIO_INPUT,
    AUDIO_OUTPUT,
    CONTROL_INPUT,
    CONTROL_OUTPUT
};

// Enum for different plugin types that can be wrapped
enum class PluginType {
    COMPRESSOR,
    LIMITER,
    EXPANDER,
    GATE,
    EQUALIZER,
    FILTER,
    DELAY,
    REVERB,
    CHORUS,
    FLANGER,
    PHASER,
    TREMOLO,
    DISTORTION,
    OVERDRIVE,
    HARMONIC_EXCITER,
    AMPLIFIER_SIM,
    MODULAR_FX
};

// Structure for LV2 port information
struct LV2Port {
    std::string symbol;          // Port symbol (e.g., "input", "output", "gain")
    std::string name;            // Human-readable name
    LV2PortType type;            // Port type
    float min_value;             // Minimum value for control ports
    float max_value;             // Maximum value for control ports
    float default_value;         // Default value for control ports
    float value;                 // Current value for control ports
    bool is_input;               // True if input port, false if output port
    
    LV2Port(const std::string& sym = "", const std::string& n = "", LV2PortType t = LV2PortType::CONTROL_INPUT,
            float min = 0.0f, float max = 1.0f, float def = 0.5f, bool inp = true)
        : symbol(sym), name(n), type(t), min_value(min), max_value(max), 
          default_value(def), value(def), is_input(inp) {}
};

// Structure for LV2 plugin metadata
struct LV2PluginMetadata {
    std::string uri;                    // Plugin URI
    std::string name;                   // Plugin name
    std::string label;                  // Short plugin label
    std::string author;                 // Plugin author
    std::string description;            // Plugin description
    PluginType plugin_type;            // Type of plugin
    std::string license;                // License string
    std::string version;                // Version string
    
    LV2PluginMetadata()
        : plugin_type(PluginType::MODULAR_FX), license("GPL"), version("1.0.0") {}
};

// Base class for LV2-compatible audio effects
class LV2AudioEffect : public TimeVaryingEffect {
public:
    typedef LV2AudioEffect CLASSNAME;
    
    LV2AudioEffect(const std::string& name = "LV2AudioEffect");
    virtual ~LV2AudioEffect();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "LV2AudioEffect"; }

    // Initialize as LV2 plugin with metadata
    bool InitializeAsLV2Plugin(const LV2PluginMetadata& metadata);

    // LV2 port management
    void AddPort(const LV2Port& port);
    bool SetPortValue(const std::string& symbol, float value);
    float GetPortValue(const std::string& symbol) const;
    
    // Get port by symbol
    const LV2Port* GetPort(const std::string& symbol) const;
    LV2Port* GetPort(const std::string& symbol);
    
    // Get all ports
    const std::vector<LV2Port>& GetPorts() const { return ports; }
    
    // Get metadata
    const LV2PluginMetadata& GetMetadata() const { return metadata; }
    
    // Process audio buffer (for LV2 compatibility)
    virtual void ProcessAudioBuffer(const float** inputs, float** outputs, 
                                   uint32_t sample_count, uint32_t channel_count);

    // Activation/deactivation
    virtual void Activate();
    virtual void Deactivate();
    
    // Get/set active state
    bool IsActive() const { return active; }
    void SetActive(bool state) { active = state; }

    // Preset management
    virtual bool LoadPreset(const std::string& preset_name);
    virtual bool SavePreset(const std::string& preset_name);
    virtual std::vector<std::string> GetPresetList() const;

    // Get parameter value by index (for UI)
    float GetParameterValueByIndex(uint32_t index) const;
    
    // Set parameter value by index (from UI)
    void SetParameterValueByIndex(uint32_t index, float value);

protected:
    // Internal audio processing - to be implemented by derived classes
    virtual void InternalProcess(const float** inputs, float** outputs, 
                                uint32_t sample_count) = 0;

private:
    LV2PluginMetadata metadata;
    std::vector<LV2Port> ports;
    std::map<std::string, size_t> port_index_map;  // For fast lookup
    bool active;
    std::vector<std::string> presets;              // List of available presets
};

// Factory class for creating LV2-compatible effects
class LV2EffectFactory {
public:
    LV2EffectFactory();
    virtual ~LV2EffectFactory();

    // Create an effect instance by type
    static LV2AudioEffect* CreateEffect(PluginType type);

    // Register a custom effect type
    static void RegisterEffectType(PluginType type, 
                                  std::function<LV2AudioEffect*()> constructor);

    // Get plugin metadata for a type
    static LV2PluginMetadata GetPluginMetadata(PluginType type);

private:
    static std::map<PluginType, std::function<LV2AudioEffect*()>> effect_constructors;
    static std::map<PluginType, LV2PluginMetadata> plugin_metadata;
};

// Utility functions for LV2 compatibility
namespace LV2Utils {
    // Convert ProtoVM parameter type to LV2 type
    std::string ParameterTypeToLV2(const ParameterMetadata& param);
    
    // Register common effects with the factory
    void RegisterCommonEffects();
    
    // Initialize LV2 wrapper system
    bool Initialize();
    
    // Cleanup LV2 wrapper system
    void Cleanup();
}

// Base class for LV2-compatible compressors
class LV2Compressor : public LV2AudioEffect {
public:
    typedef LV2Compressor CLASSNAME;
    
    LV2Compressor(const std::string& name = "LV2Compressor");
    virtual ~LV2Compressor() {}

    virtual String GetClassName() const override { return "LV2Compressor"; }

protected:
    virtual void InternalProcess(const float** inputs, float** outputs, 
                                uint32_t sample_count) override;
    
private:
    // Compressor-specific parameters
    float threshold_db;
    float ratio;
    float attack_ms;
    float release_ms;
    float makeup_gain_db;
    float knee_width_db;
    
    // Internal compressor state
    float envelope;
    float last_gain;
    
    void UpdateCompressorParams();
};

// Base class for LV2-compatible amplifiers
class LV2AmpSimulator : public LV2AudioEffect {
public:
    typedef LV2AmpSimulator CLASSNAME;
    
    LV2AmpSimulator(const std::string& name = "LV2AmpSimulator");
    virtual ~LV2AmpSimulator() {}

    virtual String GetClassName() const override { return "LV2AmpSimulator"; }

protected:
    virtual void InternalProcess(const float** inputs, float** outputs, 
                                uint32_t sample_count) override;
    
private:
    // Amplifier-specific parameters
    float preamp_gain;
    float master_volume;
    float tone_controls[3];  // Bass, mid, treble
    float presence;
    float resonance;
    
    // Tube model parameters
    float tube_type;
    float bias;
    float power_scaling;
    
    void UpdateAmpParams();
};

// Base class for LV2-compatible reverbs
class LV2Reverb : public LV2AudioEffect {
public:
    typedef LV2Reverb CLASSNAME;
    
    LV2Reverb(const std::string& name = "LV2Reverb");
    virtual ~LV2Reverb() {}

    virtual String GetClassName() const override { return "LV2Reverb"; }

protected:
    virtual void InternalProcess(const float** inputs, float** outputs, 
                                uint32_t sample_count) override;
    
private:
    // Reverb-specific parameters
    float room_size;      // 0.0 to 1.0
    float damping;        // 0.0 to 1.0
    float wet_level;      // 0.0 to 1.0
    float dry_level;      // 0.0 to 1.0
    float width;          // Stereo width, 0.0 to 1.0
    float freeze_mode;    // 0.0 to 1.0
    
    // Internal reverb state
    std::vector<float> delay_lines[4];  // Multiple delay lines for reverb
    size_t read_positions[4];
    
    void UpdateReverbParams();
};

#endif