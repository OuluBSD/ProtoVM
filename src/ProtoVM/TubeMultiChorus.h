#ifndef _ProtoVM_TubeMultiChorus_h_
#define _ProtoVM_TubeMultiChorus_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include "LFO.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based multichorus (phase-constellation chorus) circuits
class TubeMultiChorus : public ElectricNodeBase {
public:
    typedef TubeMultiChorus CLASSNAME;

    // Different types of multichorus configurations
    enum MultiChorusType {
        PHASE_CONSTELLATION_CHORUS,  // Classic phase-constellation chorus
        VINTAGE_JET_STREAM_CHORUS,   // Vintage jet-stream chorus effect
        MODERN_GLASS_CHORUS,         // Modern glass chorus effect
        STEREO_FIELD_CHORUS          // Wide stereo field chorus
    };

    TubeMultiChorus(MultiChorusType type = PHASE_CONSTELLATION_CHORUS, int voice_count = 8);
    virtual ~TubeMultiChorus();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeMultiChorus"; }

    // Configuration methods
    void SetLFOFrequency(double freq);
    void SetLFOAmount(double amount);
    void SetDelayDepth(double depth);
    void SetVoiceCount(int count);
    void SetDetuneDepth(double detune);
    void SetFeedback(double feedback);
    void SetSpread(double spread);  // Controls how voices are distributed in stereo field
    void SetConstellationSize(int size);  // Number of phase positions in constellation
    
    // Getter methods
    double GetLFOFrequency() const { return lfo_frequency; }
    double GetLFOAmount() const { return lfo_amount; }
    double GetDelayDepth() const { return delay_depth; }
    int GetVoiceCount() const { return voice_count; }
    double GetDetuneDepth() const { return detune_depth; }
    double GetFeedback() const { return feedback; }
    double GetSpread() const { return spread; }
    int GetConstellationSize() const { return constellation_size; }

private:
    MultiChorusType chorus_type;
    
    // Parameters
    double lfo_frequency;
    double lfo_amount;
    double delay_depth;
    int voice_count;
    double detune_depth;
    double feedback;
    double spread;  // Stereo spread control
    int constellation_size;  // Number of phase positions
    
    // LFO for each voice
    std::vector<std::unique_ptr<LFO>> modulation_lfos;
    
    // Delay lines for each voice
    std::vector<std::vector<double>> delay_lines;
    std::vector<size_t> write_indices;
    std::vector<std::vector<double>> allpass_delays;  // Allpass filter delays for phase constellations
    std::vector<std::vector<double>> allpass_outputs;
    
    // Output mixers
    double left_output;
    double right_output;
    
    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tube_characteristics;
    
    // Processing state
    double input_signal;
    double current_time;
    
    // Constants
    static constexpr size_t MAX_DELAY_SIZE = 2205;  // ~50ms at 44.1kHz
    static constexpr int MAX_VOICES = 16;
    static constexpr double MIN_DELAY_DEPTH = 0.0001;  // 0.1ms
    static constexpr double MAX_DELAY_DEPTH = 0.05;    // 50ms
    static constexpr double MIN_DETUNE_DEPTH = 0.0;    // No detune
    static constexpr double MAX_DETUNE_DEPTH = 0.5;    // Maximum detune
    static constexpr double MIN_FEEDBACK = -0.9;       // Minimum feedback
    static constexpr double MAX_FEEDBACK = 0.9;        // Maximum feedback
    static constexpr double MIN_SPREAD = 0.0;          // Mono
    static constexpr double MAX_SPREAD = 1.0;          // Full stereo
    
    void InitializeChorus(MultiChorusType type);
    void ProcessSignal();
    double GetDelayedSample(int voice, size_t delay_samples);
    void UpdateLFOs();
    void ApplyTubeCharacteristics();
};

#endif