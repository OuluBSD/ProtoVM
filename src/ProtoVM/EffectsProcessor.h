#ifndef _ProtoVM_EffectsProcessor_h_
#define _ProtoVM_EffectsProcessor_h_

#include "AnalogCommon.h"
#include <vector>

// Enum for different types of audio effects
enum class EffectType {
    REVERB,         // Reverb effect
    DELAY,          // Delay/echo effect
    CHORUS,         // Chorus effect
    PHASER,         // Phaser effect
    FLANGER,        // Flanger effect
    COMPRESSOR,     // Dynamic range compressor
    DISTORTION,     // Distortion/drive effect
    TREMOLO,        // Tremolo effect
    WAH_WAH,        // Wah-wah filter effect
    PARAMETRIC_EQ   // Parametric equalizer
};

// Structure for effect parameters
struct EffectParameter {
    std::string name;
    double min_value;
    double max_value;
    double default_value;
    double current_value;
    std::string unit;  // Unit of measurement
    
    EffectParameter(const std::string& n, double min, double max, double def, const std::string& u = "")
        : name(n), min_value(min), max_value(max), default_value(def), 
          current_value(def), unit(u) {}
};

class EffectsProcessor : public AnalogNodeBase {
public:
    typedef EffectsProcessor CLASSNAME;

    EffectsProcessor(EffectType type = EffectType::DELAY);
    virtual ~EffectsProcessor();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "EffectsProcessor"; }

    void SetType(EffectType type);
    EffectType GetType() const { return effect_type; }

    void SetInput(double input);
    double GetInput() const { return input_signal; }

    double GetOutput() const { return output; }

    // Add an effect parameter
    int AddParameter(const EffectParameter& param);
    
    // Get parameter by ID
    EffectParameter* GetParameter(int id);
    
    // Set parameter value by ID
    bool SetParameterValue(int id, double value);
    
    // Set parameter value by name
    bool SetParameterByName(const std::string& name, double value);
    
    // Get parameter value by ID
    double GetParameterValue(int id) const;
    
    // Get parameter value by name
    double GetParameterByName(const std::string& name) const;

    // Process the specific effect
    void ProcessReverb();
    void ProcessDelay();
    void ProcessChorus();
    void ProcessPhaser();
    void ProcessFlanger();
    void ProcessCompressor();
    void ProcessDistortion();
    void ProcessTremolo();
    void ProcessWahWah();
    void ProcessParametricEQ();

    // Enable/disable the effect
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }

    // Set wet/dry mix
    void SetWetDryMix(double mix);  // 0.0 = dry, 1.0 = wet
    double GetWetDryMix() const { return wet_dry_mix; }

private:
    EffectType effect_type;
    double input_signal;
    double output;
    bool is_enabled;
    double wet_dry_mix;  // 0.0 = completely dry, 1.0 = completely wet
    
    std::vector<EffectParameter> parameters;
    int parameter_count;
    
    // Effect-specific state variables
    std::vector<double> delay_buffer;     // For delay-based effects
    int delay_buffer_size;
    int read_index;
    int write_index;
    
    double lfo_phase;                   // For modulation effects
    double lfo_rate;                    // Rate for modulation effects
    double feedback;                    // Feedback for delay/reverb
    double filter_state[4];             // For filter-based effects
    double compressor_threshold;        // For compressor
    double compressor_ratio;            // For compressor
    double compressor_attack;           // For compressor
    double compressor_release;          // For compressor
    double last_output;                 // For feedback calculations

    // Initialize effect-specific parameters
    void InitializeParameters();
    
    // Internal processing methods
    double ApplyWetDryMix(double dry, double wet) const;
};

#endif