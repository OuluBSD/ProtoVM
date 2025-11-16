#ifndef _ProtoVM_AudioSignalPath_h_
#define _ProtoVM_AudioSignalPath_h_

#include "AnalogCommon.h"
#include "VCO.h"
#include "VCF.h"
#include "VCA.h"
#include "LFO.h"
#include "ADSR.h"
#include <vector>

// Enum for different types of audio signal paths
enum class SignalPathType {
    SYNTH_VOICE,           // Single synthesizer voice (VCO->VCF->VCA)
    DUAL_OSC_VOICE,        // Two VCOs mixed into VCF->VCA
    VINTAGE_MONO_SYNTH,    // Vintage monophonic synthesizer path
    MODULAR_PATCH,         // Modular synthesizer patch with multiple routings
    CUSTOM_PATH            // Custom signal path configuration
};

// Structure to define routing in the signal path
struct SignalRoute {
    int source_component;   // Index of source component
    int destination;        // Index of destination component or output
    double gain;            // Gain/attenuation of the route
    bool active;            // Whether the route is active
    
    SignalRoute() : source_component(0), destination(0), gain(1.0), active(true) {}
};

class AudioSignalPath : public AnalogNodeBase {
public:
    typedef AudioSignalPath CLASSNAME;

    AudioSignalPath(SignalPathType type = SignalPathType::SYNTH_VOICE);
    virtual ~AudioSignalPath() {}

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "AudioSignalPath"; }

    void SetType(SignalPathType type);
    SignalPathType GetType() const { return type; }

    // Add a component to the signal path
    void AddComponent(AnalogNodeBase* component);
    
    // Get a component by index
    AnalogNodeBase* GetComponent(int index);
    
    // Set input to the signal path (for external input)
    void SetInput(int input_index, double input);
    
    // Get output from the signal path
    double GetOutput() const { return final_output; }
    
    // Connect components (source_idx to dest_idx)
    bool Connect(int source_idx, int dest_idx, double gain = 1.0);
    
    // Disconnect components
    bool Disconnect(int source_idx, int dest_idx);
    
    // Set routing configuration
    void SetRouting(const std::vector<SignalRoute>& routes);
    
    // Get routing configuration
    const std::vector<SignalRoute>& GetRouting() const { return routing; }
    
    // Set master volume
    void SetMasterVolume(double volume) { master_volume = volume; }
    double GetMasterVolume() const { return master_volume; }
    
    // Update the signal path based on current component positions and connections
    void UpdateSignalPath();
    
    // Get frequency response at a specific frequency (simplified model)
    double GetFrequencyResponse(double frequency) const;
    
    // Calculate approximate total latency of the signal path
    double GetLatency() const;

private:
    SignalPathType type;
    std::vector<AnalogNodeBase*> components;
    std::vector<SignalRoute> routing;
    std::vector<double> inputs;  // Buffer for component inputs
    double final_output;
    double master_volume;
    
    // Process each type of signal path
    void ProcessSynthVoice();
    void ProcessDualOscVoice();
    void ProcessVintageMonoSynth();
    void ProcessModularPatch();
    void ProcessCustomPath();
    
    // Helper methods
    void ProcessComponent(int comp_idx);
    void ApplyRouting();
    void CalculateFrequencyResponse();
};

#endif