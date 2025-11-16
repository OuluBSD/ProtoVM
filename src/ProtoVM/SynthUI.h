#ifndef _ProtoVM_SynthUI_h_
#define _ProtoVM_SynthUI_h_

#include "AnalogCommon.h"
#include "PolyphonyManager.h"
#include "VCO.h"
#include "VCF.h"
#include "VCA.h"
#include "LFO.h"
#include "ADSR.h"
#include <map>
#include <string>

// Enum for different UI control types
enum class UIControlType {
    KNOB,           // Rotary knob
    SLIDER,         // Vertical/horizontal slider
    BUTTON,         // Push button
    TOGGLE,         // On/off switch
    WAVEFORM_SELECTOR,  // Waveform selection
    KEYBOARD,       // Piano-style keyboard
    XY_PAD,         // 2D control pad
    SEQUENCER_GRID  // Step sequencer
};

// Structure for UI parameters
struct UIParameter {
    std::string name;
    double min_value;
    double max_value;
    double default_value;
    double current_value;
    UIControlType control_type;
    std::string unit;  // Unit of measurement (Hz, %, etc.)
    
    UIParameter(const std::string& n, double min, double max, double def, UIControlType ct, const std::string& u = "")
        : name(n), min_value(min), max_value(max), default_value(def), 
          current_value(def), control_type(ct), unit(u) {}
};

// Structure for UI control mapping to synth components
struct UIControlMapping {
    std::string component_name;  // "VCO1", "VCF", "ADSR", etc.
    std::string parameter_name;  // "Frequency", "Cutoff", "Attack", etc.
    int component_index;         // Index in the component array (-1 if not applicable)
    
    UIControlMapping(const std::string& comp, const std::string& param, int idx = -1)
        : component_name(comp), parameter_name(param), component_index(idx) {}
};

class SynthUI : public AnalogNodeBase {
public:
    typedef SynthUI CLASSNAME;

    SynthUI(PolyphonyManager* synth_engine);
    virtual ~SynthUI();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "SynthUI"; }

    // Add a parameter to the UI
    int AddParameter(const UIParameter& param);
    
    // Get parameter by ID
    UIParameter* GetParameter(int id);
    
    // Set parameter value by ID
    bool SetParameterValue(int id, double value);
    
    // Set parameter value by name
    bool SetParameterByName(const std::string& name, double value);
    
    // Get parameter value by ID
    double GetParameterValue(int id) const;
    
    // Get parameter value by name
    double GetParameterByName(const std::string& name) const;
    
    // Add a control mapping
    void AddControlMapping(int param_id, const UIControlMapping& mapping);
    
    // Update the synth engine with current UI values
    void UpdateSynthEngine();
    
    // Get the number of parameters
    int GetParameterCount() const { return static_cast<int>(parameters.size()); }
    
    // Create a default layout for a subtractive synthesizer
    void CreateDefaultLayout();
    
    // Handle UI events (keyboard, mouse, etc.) - simplified for now
    void HandleEvent(const std::string& event_type, int param_id, double value);
    
    // Get UI data for rendering (simplified interface)
    const std::vector<UIParameter>& GetParameters() const { return parameters; }
    
    // Set/get synth engine
    void SetSynthEngine(PolyphonyManager* engine);
    PolyphonyManager* GetSynthEngine() const { return synth_engine; }
    
    // Default parameter IDs for common synthesizer controls
    enum DefaultParameterIDs {
        // VCO parameters
        VCO1_FREQ = 0,
        VCO1_WAVEFORM,
        VCO1_DETUNE,
        VCO2_FREQ,
        VCO2_WAVEFORM,
        VCO2_DETUNE,
        
        // VCF parameters
        FILTER_CUTOFF,
        FILTER_RESONANCE,
        FILTER_TYPE,
        FILTER_DRIVE,
        
        // VCA parameters
        AMP_LEVEL,
        
        // ADSR parameters
        ATTACK_TIME,
        DECAY_TIME,
        SUSTAIN_LEVEL,
        RELEASE_TIME,
        
        // LFO parameters
        LFO_RATE,
        LFO_DEPTH,
        LFO_DESTINATION,
        
        // Modulation parameters
        PITCH_MOD_WHEEL,
        FILTER_MOD_WHEEL,
        VIBRATO_DEPTH,
        PORTAMENTO_TIME,
        
        // Voice/polyphony parameters
        VOICE_COUNT,
        VOICE_ALLOCATION,
        LEGATO_MODE,
        
        // Effects (to be implemented later)
        REVERB_AMOUNT,
        CHORUS_DEPTH,
        
        // End marker
        PARAMETER_COUNT
    };

private:
    PolyphonyManager* synth_engine;
    std::vector<UIParameter> parameters;
    std::vector<std::vector<UIControlMapping>> param_mappings;  // Each parameter can map to multiple synth controls
    
    // Internal methods
    void InitializeDefaultParameters();
    void MapDefaultControls();
    
    // Update specific synth components
    void UpdateVCOs();
    void UpdateVCFs();
    void UpdateVCAs();
    void UpdateLFOs();
    void UpdateADSRs();
    
    // Find parameter by name
    int FindParameterByName(const std::string& name) const;
};

#endif