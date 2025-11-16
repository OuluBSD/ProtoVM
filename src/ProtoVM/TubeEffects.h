#ifndef _ProtoVM_TubeEffects_h_
#define _ProtoVM_TubeEffects_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"  // For basic analog components
#include "LFO.h"              // For phaser modulation
#include <vector>
#include <memory>

// Enum for effect types
enum class EffectType {
    COMPRESSOR,          // Tube compressor
    LIMITER,             // Tube limiter
    PHASER,              // Tube phaser
    FLANGER,             // Tube flanger
    CHORUS,              // Tube chorus
    TUBE_DRIVE,          // Tube overdrive/distortion
    TREMOLO             // Tube tremolo
};

// Tube compressor/limiter characteristics
enum class CompressionType {
    VARI_MU,            // Variable mu (gain) compression
    FIXED_RATIO,        // Fixed compression ratio
    LIMITING,           // Hard limiting
    PEAK_LIMITING       // Peak limiting
};

// Tube effect base class
class TubeEffect : public AnalogNodeBase {
public:
    typedef TubeEffect CLASSNAME;

    TubeEffect(EffectType type);
    virtual ~TubeEffect();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "TubeEffect"; }

    // Set/get input signal
    void SetInputSignal(double signal) { input_signal = signal; }
    double GetInputSignal() const { return input_signal; }
    
    // Get output signal
    double GetOutputSignal() const { return output_signal; }
    
    // Get/set effect parameters
    void SetBypass(bool bypass) { bypass_effect = bypass; }
    bool IsBypassed() const { return bypass_effect; }
    
    void SetWetDryMix(double mix) { wet_dry_mix = std::max(0.0, std::min(1.0, mix)); }
    double GetWetDryMix() const { return wet_dry_mix; }
    
    void SetGain(double gain) { effect_gain = std::max(0.1, std::min(100.0, gain)); }
    double GetGain() const { return effect_gain; }
    
    void SetOutputLevel(double level) { output_level = std::max(0.0, std::min(2.0, level)); }
    double GetOutputLevel() const { return output_level; }
    
    // Get/set effect type
    void SetEffectType(EffectType type) { effect_type = type; }
    EffectType GetEffectType() const { return effect_type; }
    
    // Enable/disable the effect
    void SetEnabled(bool enabled) { is_enabled = enabled; }
    bool IsEnabled() const { return is_enabled; }

protected:
    EffectType effect_type;
    double input_signal;      // Input signal to the effect
    double output_signal;     // Output signal from the effect
    bool bypass_effect;       // Whether the effect is bypassed
    double wet_dry_mix;       // Wet/dry mix (0.0 = dry, 1.0 = wet)
    double effect_gain;       // Internal gain of the effect
    double output_level;      // Final output level
    bool is_enabled;          // Whether the effect is enabled
    
    // Vector of tubes used in the effect
    std::vector<std::unique_ptr<Tube>> effect_tubes;
    
    // Internal processing method
    virtual void ProcessSignal() = 0;
    
    // Common method to apply tube characteristics
    virtual void ApplyTubeCharacteristics(double& signal);
    
    // Method to mix wet and dry signals
    virtual double ApplyWetDryMix(double dry_signal, double wet_signal);
    
    static constexpr double MIN_GAIN = 0.1;
    static constexpr double MAX_GAIN = 100.0;
    static constexpr double MIN_OUTPUT_LEVEL = 0.0;
    static constexpr double MAX_OUTPUT_LEVEL = 2.0;
};

// Tube compressor/limiter
class TubeCompressor : public TubeEffect {
public:
    typedef TubeCompressor CLASSNAME;

    TubeCompressor(CompressionType type = CompressionType::VARI_MU);
    virtual ~TubeCompressor() {}

    virtual String GetClassName() const override { return "TubeCompressor"; }
    
    // Compressor-specific parameters
    void SetThreshold(double threshold) { compression_threshold = std::max(0.01, std::min(2.0, threshold)); }
    double GetThreshold() const { return compression_threshold; }
    
    void SetRatio(double ratio) { compression_ratio = std::max(1.0, std::min(20.0, ratio)); }
    double GetRatio() const { return compression_ratio; }
    
    void SetAttackTime(double time) { attack_time = std::max(0.001, std::min(0.5, time)); }  // 1ms to 500ms
    double GetAttackTime() const { return attack_time; }
    
    void SetReleaseTime(double time) { release_time = std::max(0.01, std::min(5.0, time)); }  // 10ms to 5s
    double GetReleaseTime() const { return release_time; }
    
    void SetKnee(double knee) { compression_knee = std::max(0.0, std::min(1.0, knee)); }  // 0.0 = hard, 1.0 = soft knee
    double GetKnee() const { return compression_knee; }
    
    void SetMakeupGain(double gain) { makeup_gain = std::max(1.0, std::min(20.0, gain)); }
    double GetMakeupGain() const { return makeup_gain; }
    
    void SetAutoMakeup(bool auto_makeup) { auto_makeup = auto_makeup; }
    bool IsAutoMakeup() const { return auto_makeup; }
    
    void SetCompressionType(CompressionType type) { compression_type = type; }
    CompressionType GetCompressionType() const { return compression_type; }
    
    // Get compression statistics
    double GetCurrentGainReduction() const { return current_gain_reduction; }
    double GetCurrentLevel() const { return current_level; }

protected:
    CompressionType compression_type;
    double compression_threshold;   // Threshold in volts
    double compression_ratio;       // Compression ratio (e.g., 4:1)
    double attack_time;             // Attack time in seconds
    double release_time;            // Release time in seconds    
    double compression_knee;        // Knee softness (0.0 = hard, 1.0 = soft)
    double makeup_gain;             // Makeup gain
    bool auto_makeup;              // Whether to automatically apply makeup gain
    
    // Dynamic state
    double current_gain_reduction;  // Current gain reduction applied
    double current_level;           // Current signal level
    double envelope_detector;       // Detected signal level
    double sidechain_filter;        // Sidechain filter state
    
    virtual void ProcessSignal() override;
    virtual void UpdateCompressorState();
    
    static constexpr double MIN_THRESHOLD = 0.01;   // Minimum threshold
    static constexpr double MAX_THRESHOLD = 2.0;    // Maximum threshold
    static constexpr double MIN_RATIO = 1.0;        // 1:1 (no compression)
    static constexpr double MAX_RATIO = 20.0;       // Maximum ratio
    static constexpr double MIN_ATTACK_TIME = 0.001;  // 1ms
    static constexpr double MAX_ATTACK_TIME = 0.5;    // 500ms
    static constexpr double MIN_RELEASE_TIME = 0.01;  // 10ms
    static constexpr double MAX_RELEASE_TIME = 5.0;   // 5s
};

// Tube phaser effect
class TubePhaser : public TubeEffect {
public:
    typedef TubePhaser CLASSNAME;

    TubePhaser(int stages = 4);
    virtual ~TubePhaser() {}

    virtual String GetClassName() const override { return "TubePhaser"; }
    
    // Phaser-specific parameters
    void SetLFOFrequency(double freq) { lfo_frequency = std::max(0.1, std::min(10.0, freq)); }
    double GetLFOFrequency() const { return lfo_frequency; }
    
    void SetLFOAmount(double amount) { lfo_amount = std::max(0.0, std::min(1.0, amount)); }
    double GetLFOAmount() const { return lfo_amount; }
    
    void SetFeedback(double feedback) { phaser_feedback = std::max(-0.9, std::min(0.9, feedback)); }
    double GetFeedback() const { return phaser_feedback; }
    
    void SetNotchCount(int count) { notch_count = std::max(2, std::min(24, count)); }
    int GetNotchCount() const { return notch_count; }
    
    void SetCenterFrequency(double freq) { center_frequency = std::max(20.0, std::min(20000.0, freq)); }
    double GetCenterFrequency() const { return center_frequency; }
    
    void SetPhaseDepth(double depth) { phase_depth = std::max(0.0, std::min(1.0, depth)); }
    double GetPhaseDepth() const { return phase_depth; }
    
    // Set number of allpass stages
    void SetStageCount(int count) { stage_count = std::max(2, std::min(24, count)); }
    int GetStageCount() const { return stage_count; }

protected:
    std::unique_ptr<LFO> modulation_lfo;  // For controlling the phasing
    double lfo_frequency;                 // LFO frequency for modulation
    double lfo_amount;                    // Amount of LFO modulation
    double phaser_feedback;               // Feedback amount
    int notch_count;                      // Number of notches
    double center_frequency;              // Center frequency of phasing
    double phase_depth;                   // Depth of the phase effect
    int stage_count;                      // Number of allpass stages
    
    // Allpass filter stages for phasing
    std::vector<double> allpass_stages;   // Delay values for each stage
    std::vector<double> allpass_outputs;  // Output of each stage
    
    virtual void ProcessSignal() override;
    virtual void ProcessAllpassStage(int stage, double& signal, double modulation);
    virtual void UpdateLFO();
    
    static constexpr double MIN_LFO_FREQ = 0.1;    // 0.1 Hz
    static constexpr double MAX_LFO_FREQ = 10.0;   // 10 Hz
    static constexpr int MIN_STAGES = 2;           // Minimum stages
    static constexpr int MAX_STAGES = 24;          // Maximum stages
    static constexpr double MIN_FEEDBACK = -0.9;   // Minimum feedback
    static constexpr double MAX_FEEDBACK = 0.9;    // Maximum feedback
};

// Tube flanger effect (similar to phaser but with longer delay)
class TubeFlanger : public TubeEffect {
public:
    typedef TubeFlanger CLASSNAME;

    TubeFlanger();
    virtual ~TubeFlanger() {}

    virtual String GetClassName() const override { return "TubeFlanger"; }
    
    // Flanger-specific parameters
    void SetLFOFrequency(double freq) { lfo_frequency = std::max(0.1, std::min(5.0, freq)); }
    double GetLFOFrequency() const { return lfo_frequency; }
    
    void SetLFOAmount(double amount) { lfo_amount = std::max(0.0, std::min(1.0, amount)); }
    double GetLFOAmount() const { return lfo_amount; }
    
    void SetFeedback(double feedback) { flanger_feedback = std::max(-0.9, std::min(0.9, feedback)); }
    double GetFeedback() const { return flanger_feedback; }
    
    void SetDelayDepth(double depth) { delay_depth = std::max(0.0001, std::min(0.01, depth)); }  // 0.1ms to 10ms
    double GetDelayDepth() const { return delay_depth; }
    
    void SetCenterDelay(double delay) { center_delay = std::max(0.0001, std::min(0.01, delay)); }  // 0.1ms to 10ms
    double GetCenterDelay() const { return center_delay; }

protected:
    std::unique_ptr<LFO> modulation_lfo;  // For controlling the flanging
    double lfo_frequency;                 // LFO frequency for modulation
    double lfo_amount;                    // Amount of LFO modulation
    double flanger_feedback;              // Feedback amount
    double delay_depth;                   // Depth of delay modulation
    double center_delay;                  // Center delay time
    
    // Delay line for flanging
    std::vector<double> delay_line;
    size_t write_index;
    size_t read_index;
    size_t max_delay_samples;
    
    virtual void ProcessSignal() override;
    virtual void UpdateLFO();
    virtual double GetDelayedSample(size_t delay_samples);
    
    static constexpr size_t MAX_DELAY_SIZE = 441;  // ~10ms at 44.1kHz
    static constexpr double MIN_DELAY_DEPTH = 0.0001;  // 0.1ms
    static constexpr double MAX_DELAY_DEPTH = 0.01;    // 10ms
};

// Tube chorus effect
class TubeChorus : public TubeEffect {
public:
    typedef TubeChorus CLASSNAME;

    TubeChorus(int voices = 2);
    virtual ~TubeChorus() {}

    virtual String GetClassName() const override { return "TubeChorus"; }
    
    // Chorus-specific parameters
    void SetLFOFrequency(double freq) { lfo_frequency = std::max(0.1, std::min(10.0, freq)); }
    double GetLFOFrequency() const { return lfo_frequency; }
    
    void SetLFOAmount(double amount) { lfo_amount = std::max(0.0, std::min(1.0, amount)); }
    double GetLFOAmount() const { return lfo_amount; }
    
    void SetDelayDepth(double depth) { delay_depth = std::max(0.0001, std::min(0.005, depth)); }  // 0.1ms to 5ms
    double GetDelayDepth() const { return delay_depth; }
    
    void SetVoiceCount(int count) { voice_count = std::max(1, std::min(8, count)); }
    int GetVoiceCount() const { return voice_count; }
    
    void SetDetuneDepth(double detune) { detune_depth = std::max(0.0, std::min(0.5, detune)); }
    double GetDetuneDepth() const { return detune_depth; }

protected:
    std::vector<std::unique_ptr<LFO>> modulation_lfos;  // LFOs for each voice
    double lfo_frequency;                 // Base LFO frequency
    double lfo_amount;                    // Amount of LFO modulation
    double delay_depth;                   // Depth of delay modulation
    int voice_count;                      // Number of chorus voices
    double detune_depth;                  // Detune depth between voices
    
    // Delay lines for each voice
    std::vector<std::vector<double>> delay_lines;
    std::vector<size_t> write_indices;
    std::vector<size_t> read_indices;
    size_t max_delay_samples;
    
    virtual void ProcessSignal() override;
    virtual void UpdateLFOs();
    virtual double GetDelayedSample(int voice, size_t delay_samples);
    
    static constexpr size_t MAX_DELAY_SIZE = 220;  // ~5ms at 44.1kHz
    static constexpr int MAX_VOICES = 8;           // Maximum number of voices
    static constexpr double MIN_DETUNE_DEPTH = 0.0;  // No detune
    static constexpr double MAX_DETUNE_DEPTH = 0.5;  // Maximum detune
};

#endif