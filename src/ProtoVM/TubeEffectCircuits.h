#ifndef TUBE_EFFECT_CIRCUITS_H
#define TUBE_EFFECT_CIRCUITS_H

#include "ElectricNodeBase.h"
#include <vector>
#include <memory>

// Class for tube-based compressor/limiter circuits
class TubeCompressor : public ElectricNodeBase {
public:
    enum CompressionType {
        CLASS_A_FETISH,      // Simulates FET-style compression with tubes
        TRIODE_LIMITER,      // Basic triode-based limiter
        PENTODE_COMPRESSOR,  // Variable-mu compression using pentodes
        VAR_MU_LIMITER       // Classic variable-mu style
    };
    
    TubeCompressor(CompressionType type = TRIODE_LIMITER);
    virtual ~TubeCompressor() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure compression parameters
    void setThreshold(double threshold);    // in dB
    void setRatio(double ratio);            // compression ratio (e.g., 4.0 for 4:1)
    void setAttackTime(double time);        // in seconds
    void setReleaseTime(double time);       // in seconds
    void setMakeupGain(double gain);        // in dB
    void setKneeWidth(double width);        // for soft knee compression
    void setSidechainCoupling(double coupling) { sidechainCoupling = coupling; }
    
    // Get parameters
    double getThreshold() const { return threshold; }
    double getRatio() const { return ratio; }
    double getAttackTime() const { return attackTime; }
    double getReleaseTime() const { return releaseTime; }
    double getMakeupGain() const { return makeupGain; }
    
    // Enable/disable features
    void enableSoftKnee(bool enable) { softKneeEnabled = enable; }
    void enableAutoMakeup(bool enable) { autoMakeupEnabled = enable; }
    void enableSidechainFilter(bool enable) { sidechainFilterEnabled = enable; }

private:
    CompressionType compressionType;
    
    // Compression parameters
    double threshold = -12.0;     // Threshold in dB
    double ratio = 4.0;           // Compression ratio
    double attackTime = 0.003;    // Attack time in seconds (3ms)
    double releaseTime = 0.1;     // Release time in seconds (100ms)
    double makeupGain = 0.0;      // Makeup gain in dB
    double kneeWidth = 2.0;       // Width of soft knee transition
    
    // Tube-specific parameters
    double tubeGain = 20.0;       // Base tube gain
    double tubeCompressionFactor; // How much the tube's characteristics contribute
    double sidechainCoupling = 0.8; // How much signal goes to sidechain
    
    // Dynamic parameters
    double detectorLevel = 0.0;   // Detected level for compression
    double compressorGain = 1.0;  // Current gain reduction
    double attackCoeff = 0.0;     // Coefficient for attack calculation
    double releaseCoeff = 0.0;    // Coefficient for release calculation
    
    // Circuit type parameters
    bool softKneeEnabled = true;
    bool autoMakeupEnabled = false;
    bool sidechainFilterEnabled = true;
    
    // Sample rate for time constants
    double sampleRate = 44100.0;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;           // For external control of compression
    int sidechainPin = 3;         // For external sidechain input
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double sidechainSignal = 0.0;
    
    // Initialize compressor based on type
    void initializeCompressor(CompressionType type);
    
    // Process signal through compression algorithm
    void processSignal();
    
    // Calculate compression gain based on input level
    double calculateCompressionGain(double inputLevel);
    
    // Update the detector level
    void updateDetector();
};

// Class for tube-based phaser circuits
class TubePhaser : public ElectricNodeBase {
public:
    enum PhaserType {
        CLASSIC_4_STAGE,     // 4-stage phaser like MXR Phase 90
        MODERN_6_STAGE,      // 6-stage phaser
        TUBE_TRIODE_PHAZE,   // Tube triode-based phaser
        VINTAGE_ANALOG      // Vintage analog phaser with tubes
    };
    
    TubePhaser(PhaserType type = CLASSIC_4_STAGE, int stages = 4);
    virtual ~TubePhaser() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure phaser parameters
    void setLFOFrequency(double freq);      // LFO rate in Hz
    void setDepth(double depth);            // 0.0 to 1.0
    void setFeedback(double feedback);      // 0.0 to 1.0 (can be negative)
    void setNotchCount(int count);          // Number of notches
    void setSpread(double spread);          // Spread between stages
    void setModulationType(int type);       // 0=sine, 1=triangle, 2=square
    
    // Get parameters
    double getLFOFrequency() const { return lfoFrequency; }
    double getDepth() const { return depth; }
    double getFeedback() const { return feedback; }
    int getNotchCount() const { return notchCount; }
    
    // Enable/disable features
    void enableTapTempo(bool enable) { tapTempoEnabled = enable; }
    void enableOscillation(bool enable) { oscillationEnabled = enable; }

private:
    PhaserType phaserType;
    int stageCount;
    
    // Phaser parameters
    double lfoFrequency = 0.5;      // LFO rate in Hz
    double depth = 0.7;             // Depth of phase modulation
    double feedback = 0.3;          // Feedback amount
    int notchCount = 6;             // Number of phase notches
    double spread = 1.0;            // Spread factor between stages
    int modulationType = 0;         // 0=sine, 1=triangle, 2=square
    
    // LFO parameters
    double lfoPhase = 0.0;
    double lfoDepth = 0.0;
    
    // All-pass filter parameters for each stage
    std::vector<double> allpassCoefficients;
    std::vector<std::vector<double>> delayBuffers;  // Delay buffers for each stage
    std::vector<int> delayBufferSizes;
    std::vector<int> writePositions;
    
    // Feedback parameters
    double feedbackBuffer = 0.0;
    double feedbackCoeff = 0.0;
    
    // Sample rate
    double sampleRate = 44100.0;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int ratePin = 2;                // For controlling LFO rate
    int depthPin = 3;               // For controlling depth
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double rateControl = 0.0;
    double depthControl = 0.0;
    
    // Features
    bool tapTempoEnabled = false;
    bool oscillationEnabled = false;
    
    // Initialize phaser with specific parameters
    void initializePhaser(PhaserType type, int stages);
    
    // Process signal through all all-pass stages
    void processSignal();
    
    // Update LFO phase
    void updateLFO();
    
    // Process through a single all-pass stage
    double processAllpassStage(int stage, double input, double coeff);
};

// Class for tube-based chorus circuits
class TubeChorus : public ElectricNodeBase {
public:
    TubeChorus(int voices = 2);
    virtual ~TubeChorus() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure chorus parameters
    void setLFOFrequency(double freq);      // LFO rate in Hz
    void setDepth(double depth);            // 0.0 to 1.0
    void setDelayTime(double time);         // Base delay time in seconds
    void setFeedback(double feedback);      // 0.0 to 1.0
    void setVoiceCount(int count) { voiceCount = count; }
    
    // Get parameters
    double getLFOFrequency() const { return lfoFrequency; }
    double getDepth() const { return depth; }
    double getDelayTime() const { return baseDelayTime; }
    double getFeedback() const { return feedback; }
    int getVoiceCount() const { return voiceCount; }

private:
    int voiceCount;
    
    // Chorus parameters
    double lfoFrequency = 1.0;      // LFO rate in Hz
    double depth = 0.3;             // Modulation depth
    double baseDelayTime = 0.012;   // Base delay time (12ms)
    double feedback = 0.0;          // Feedback amount
    double separation = 2.0;        // Separation between voices in ms
    
    // LFO for modulation
    std::vector<double> lfoPhases;
    std::vector<double> lfoOffsets; // Phase offsets for multiple voices
    
    // Delay lines for each voice
    std::vector<std::vector<double>> delayBuffers;
    std::vector<int> bufferSizes;
    std::vector<int> writePositions;
    
    // Sample rate
    double sampleRate = 44100.0;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int ratePin = 2;
    int depthPin = 3;
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    
    // Initialize chorus with specific parameters
    void initializeChorus(int voices);
    
    // Process signal through chorus effect
    void processSignal();
    
    // Update LFO for all voices
    void updateLFO();
    
    // Process through delay line for a voice
    double processDelayLine(int voice, double input);
};

// Class for tube-based expander circuits
class TubeExpander : public ElectricNodeBase {
public:
    enum ExpanderType {
        GATE,              // Noise gate expander
        BAND_GATE,         // Multiband expander
        DOWNWARD_EXPANDER, // Downward expander with ratio < 1
        UPWARD_EXPANDER    // Upward expander
    };

    TubeExpander(ExpanderType type = GATE);
    virtual ~TubeExpander() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure expander parameters
    void setThreshold(double threshold);     // in dB
    void setRatio(double ratio);             // expansion ratio (e.g., 2.0 for 2:1)
    void setAttackTime(double time);         // in seconds
    void setReleaseTime(double time);        // in seconds
    void setMakeupGain(double gain);         // in dB
    void setKneeWidth(double width);         // for soft knee expansion
    void setRange(double range);             // Maximum amount of gain reduction

    // Get parameters
    double getThreshold() const { return threshold; }
    double getRatio() const { return ratio; }
    double getAttackTime() const { return attackTime; }
    double getReleaseTime() const { return releaseTime; }
    double getMakeupGain() const { return makeupGain; }
    double getRange() const { return range; }

    // Enable/disable features
    void enableSoftKnee(bool enable) { softKneeEnabled = enable; }
    void enableAutoMakeup(bool enable) { autoMakeupEnabled = enable; }

private:
    ExpanderType expanderType;

    // Expansion parameters
    double threshold = -30.0;     // Threshold in dB (typically lower than compression)
    double ratio = 2.0;           // Expansion ratio (e.g., 2:1) - values < 1 would be upward expansion
    double attackTime = 0.005;    // Attack time in seconds (faster than compression)
    double releaseTime = 0.15;    // Release time in seconds (slower than compression)
    double makeupGain = 0.0;      // Makeup gain in dB
    double kneeWidth = 3.0;       // Width of soft knee transition
    double range = -24.0;         // Maximum gain reduction in dB

    // Tube-specific parameters
    double tubeGain = 20.0;       // Base tube gain
    double tubeExpansionFactor;   // How much the tube's characteristics contribute
    double sidechainCoupling = 0.8; // How much signal goes to sidechain

    // Dynamic parameters
    double detectorLevel = 0.0;   // Detected level for expansion
    double expanderGain = 1.0;    // Current gain reduction
    double attackCoeff = 0.0;     // Coefficient for attack calculation
    double releaseCoeff = 0.0;    // Coefficient for release calculation

    // Circuit type parameters
    bool softKneeEnabled = true;
    bool autoMakeupEnabled = false;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;           // For external control of expansion
    int sidechainPin = 3;         // For external sidechain input

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double sidechainSignal = 0.0;

    // Initialize expander based on type
    void initializeExpander(ExpanderType type);

    // Process signal through expansion algorithm
    void processSignal();

    // Calculate expansion gain based on input level
    double calculateExpansionGain(double inputLevel);

    // Update the detector level
    void updateDetector();
};

// Class for tube-based maximizer circuits
class TubeMaximizer : public ElectricNodeBase {
public:
    enum MaximizerType {
        PEEK_MAXIMIZER,      // Peak maximizer with fast attack
        RMS_MAXIMIZER,       // RMS-based maximizer for loudness
        INTEGRAL_MAXIMIZER,  // Integral-based using gain recovery
        DUAL_STAGE_MAXIMIZER // Two-stage maximizer (soft clip + hard limit)
    };

    TubeMaximizer(MaximizerType type = PEEK_MAXIMIZER);
    virtual ~TubeMaximizer() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure maximizer parameters
    void setCeiling(double ceiling);           // Ceiling in dB (e.g., -0.1, -1.0)
    void setAttackTime(double time);          // Attack time in seconds (very fast)
    void setReleaseTime(double time);         // Release time in seconds (adaptive)
    void setMakeupGain(double gain);          // Makeup gain in dB
    void setAdaptiveRelease(bool adaptive);   // Enable adaptive release
    void setLookAheadTime(double time);       // Look-ahead time in seconds (0.001-0.01)
    void setHarmonicContent(double content);  // Amount of harmonic content to add

    // Get parameters
    double getCeiling() const { return ceiling; }
    double getAttackTime() const { return attackTime; }
    double getReleaseTime() const { return releaseTime; }
    double getMakeupGain() const { return makeupGain; }
    bool getAdaptiveRelease() const { return adaptiveRelease; }
    double getLookAheadTime() const { return lookAheadTime; }
    double getHarmonicContent() const { return harmonicContent; }

    // Enable/disable features
    void enableSoftClipping(bool enable) { softClippingEnabled = enable; }
    void enableGainRecovery(bool enable) { gainRecoveryEnabled = enable; }

private:
    MaximizerType maximizerType;

    // Maximization parameters
    double ceiling = -0.1;              // Ceiling in dB (slightly below 0dBFS)
    double attackTime = 0.0005;         // Very fast attack (0.5ms) to catch peaks
    double releaseTime = 0.05;          // 50ms release (adaptive)
    double makeupGain = 0.0;            // Makeup gain in dB
    bool adaptiveRelease = true;        // Enable adaptive release
    double lookAheadTime = 0.002;       // 2ms look-ahead
    double harmonicContent = 0.1;       // Amount of harmonic content to add
    
    // Gain recovery parameters
    double gainRecoverySpeed = 0.995;   // Rate of gain recovery (slower = more gentle)

    // Tube-specific parameters
    double tubeGain = 20.0;             // Base tube gain
    double tubeMaximizationFactor;      // How much the tube's characteristics contribute

    // Dynamic parameters
    double currentGain = 1.0;           // Current gain applied
    double peakDetector = 0.0;          // Peak detection for limiting
    double gainRecoveryFactor = 1.0;    // Factor for gain recovery
    double attackCoeff = 0.0;           // Coefficient for attack calculation
    double releaseCoeff = 0.0;          // Coefficient for release calculation

    // Look-ahead delay buffer
    std::vector<double> delayBuffer;
    int delayBufferSize = 0;
    int delayWritePosition = 0;

    // Circuit type parameters
    bool softClippingEnabled = true;
    bool gainRecoveryEnabled = true;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                 // For external control of maximization
    int sidechainPin = 3;               // For external sidechain input

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double sidechainSignal = 0.0;

    // Initialize maximizer based on type
    void initializeMaximizer(MaximizerType type);

    // Process signal through maximization algorithm
    void processSignal();

    // Calculate limiting gain based on peak detection
    double calculateLimitingGain(double inputLevel);

    // Update gain recovery
    void updateGainRecovery();

    // Process look-ahead buffer
    double getLookAheadSignal();
};

// Class for LUFS-based loudness compressor circuits
class TubeLoudnessCompressor : public ElectricNodeBase {
public:
    enum LoudnessCompressorType {
        INTEGRATED_ONLY,      // Only integrated loudness control
        SHORT_TERM,           // Includes short-term loudness control
        MOMENTARY,            // Includes momentary loudness control
        TRUE_PEAK            // Includes True Peak limiting
    };

    TubeLoudnessCompressor(LoudnessCompressorType type = INTEGRATED_ONLY);
    virtual ~TubeLoudnessCompressor() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure loudness parameters
    void setIntegratedTarget(double lufs);      // Target loudness in LUFS (e.g., -16, -14, -12)
    void setRange(double range);                // Dynamic range in LU
    void setLRA(double lra);                    // Loudness Range in LU (1.0 to 20.0)
    void setTruePeakCeiling(double ceiling);    // True peak ceiling in dBTP
    void setOversampling(int factor);          // Oversampling factor (2, 4, 8)

    // Get parameters
    double getIntegratedTarget() const { return integratedTarget; }
    double getRange() const { return range; }
    double getLRA() const { return lra; }
    double getTruePeakCeiling() const { return truePeakCeiling; }
    int getOversampling() const { return oversamplingFactor; }

    // Get loudness measurements
    double getIntegratedLoudness() const { return integratedLoudness; }
    double getShortTermLoudness() const { return shortTermLoudness; }
    double getMomentaryLoudness() const { return momentaryLoudness; }
    
    // Enable/disable features
    void enableTruePeakLimiter(bool enable) { truePeakLimiterEnabled = enable; }
    void enableLoudnessNormalization(bool enable) { loudnessNormalizationEnabled = enable; }

private:
    LoudnessCompressorType compressorType;

    // Loudness parameters
    double integratedTarget = -14.0;    // Target integrated loudness in LUFS
    double range = 7.0;                 // Dynamic range in LU
    double lra = 7.0;                   // Loudness Range in LU (1.0 to 20.0)
    double truePeakCeiling = -1.0;      // True peak ceiling in dBTP
    int oversamplingFactor = 4;         // For accurate peak detection
    
    // Loudness measurement windows (in samples)
    int integratedWindow = 44100 * 3;   // 3 seconds for integrated (at 44.1kHz)
    int shortTermWindow = 44100 * 3;    // 3 seconds for short-term
    int momentaryWindow = 44100 * 0.4;  // 0.4 seconds for momentary
    
    // Filter parameters for K-filter (simulates human hearing)
    double kFilter_a1 = 0.0, kFilter_a2 = 0.0, kFilter_b0 = 0.0, kFilter_b1 = 0.0, kFilter_b2 = 0.0;
    
    // Loudness measurements
    double integratedLoudness = -70.0;    // Current integrated loudness in LUFS
    double shortTermLoudness = -70.0;     // Current short-term loudness in LUFS
    double momentaryLoudness = -70.0;     // Current momentary loudness in LUFS
    
    // Gain control
    double currentGain = 1.0;             // Current gain applied
    double targetGain = 1.0;              // Target gain based on loudness
    double smoothGain = 1.0;              // Smoothed gain
    
    // Processing buffers and states
    std::vector<double> signalBuffer;     // Buffer for loudness measurement
    std::vector<double> kFilteredBuffer;  // K-filtered buffer
    int bufferWritePos = 0;
    std::vector<double> kFilterState;     // State for K-filter (2 elements for biquad)
    int kFilterStateIndex = 0;
    
    // True peak detection and limiting
    std::vector<double> oversampledBuffer; // Buffer for oversampled processing
    int oversampledIndex = 0;
    bool truePeakLimiterEnabled = true;
    bool loudnessNormalizationEnabled = true;
    
    // Tube-specific parameters
    double tubeGain = 20.0;               // Base tube gain
    double tubeLoudnessFactor;            // How much the tube's characteristics contribute
    
    // Sample rate for time constants
    double sampleRate = 44100.0;
    double effectiveSampleRate = 44100.0; // Sample rate including oversampling

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                   // For external control
    int targetPin = 3;                    // For target loudness control

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double targetSignal = 0.0;

    // Initialize compressor based on type
    void initializeCompressor(LoudnessCompressorType type);

    // Process signal through loudness algorithm
    void processSignal();

    // Calculate K-weighted loudness
    double calculateKWeightedLoudness(const std::vector<double>& buffer, int start, int length);

    // Update loudness measurements
    void updateLoudnessMeasurements();

    // Calculate target gain based on loudness difference
    double calculateTargetGain();

    // Apply K-filter to simulate human hearing
    double applyKFilter(double input);

    // Calculate true peak
    double calculateTruePeak(const std::vector<double>& buffer, int start, int length);
};

// Class for LUFS-based loudness limiter circuits
class TubeLoudnessLimiter : public ElectricNodeBase {
public:
    enum LoudnessLimiterType {
        INTEGRATED_LIMITER,    // Limit based on integrated loudness
        SHORT_TERM_LIMITER,    // Include short-term loudness limiting
        MOMENTARY_LIMITER,     // Include momentary loudness limiting
        TRUE_PEAK_LIMITER      // True peak limiting
    };

    TubeLoudnessLimiter(LoudnessLimiterType type = INTEGRATED_LIMITER);
    virtual ~TubeLoudnessLimiter() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure loudness limiter parameters
    void setLUFSCeiling(double lufs);          // Loudness ceiling in LUFS
    void setTruePeakCeiling(double ceiling);   // True peak ceiling in dBTP
    void setOversampling(int factor);          // Oversampling factor (2, 4, 8)
    void setLimiterAttack(double time);        // Attack time (very fast for limiting)
    void setLimiterRelease(double time);       // Release time (adaptive)

    // Get parameters
    double getLUFSCeiling() const { return lufsCeiling; }
    double getTruePeakCeiling() const { return truePeakCeiling; }
    int getOversampling() const { return oversamplingFactor; }
    double getLimiterAttack() const { return attackTime; }
    double getLimiterRelease() const { return releaseTime; }

    // Get loudness measurements
    double getIntegratedLoudness() const { return integratedLoudness; }
    double getShortTermLoudness() const { return shortTermLoudness; }
    double getMomentaryLoudness() const { return momentaryLoudness; }
    
    // Enable/disable features
    void enableTruePeakLimiter(bool enable) { truePeakLimiterEnabled = enable; }
    void enableAdaptiveRelease(bool enable) { adaptiveReleaseEnabled = enable; }

private:
    LoudnessLimiterType limiterType;

    // Loudness parameters
    double lufsCeiling = -1.0;        // Loudness ceiling in LUFS (relative to target)
    double integratedTarget = -23.0;  // Target integrated loudness in LUFS (EBU R128 standard)
    double truePeakCeiling = -1.0;    // True peak ceiling in dBTP
    int oversamplingFactor = 4;       // For accurate peak detection
    double attackTime = 0.0005;       // Very fast attack (0.5ms) for limiting
    double releaseTime = 0.1;         // 100ms release (adaptive)
    
    // Loudness measurement windows (in samples)
    int integratedWindow = static_cast<int>(44100 * 3);    // 3 seconds for integrated
    int shortTermWindow = static_cast<int>(44100 * 3);     // 3 seconds for short-term
    int momentaryWindow = static_cast<int>(44100 * 0.4);   // 0.4 seconds for momentary
    
    // Filter parameters for K-filter (simulates human hearing)
    double kFilter_a1 = 0.0, kFilter_a2 = 0.0, kFilter_b0 = 0.0, kFilter_b1 = 0.0, kFilter_b2 = 0.0;
    
    // Loudness measurements
    double integratedLoudness = -70.0;    // Current integrated loudness in LUFS
    double shortTermLoudness = -70.0;     // Current short-term loudness in LUFS
    double momentaryLoudness = -70.0;     // Current momentary loudness in LUFS
    
    // Gain control
    double currentGain = 1.0;             // Current gain applied
    double maxGainReduction = 1.0;        // Maximum gain reduction applied
    double attackCoeff = 0.0;             // Attack coefficient
    double releaseCoeff = 0.0;            // Release coefficient
    
    // Processing buffers and states
    std::vector<double> signalBuffer;     // Buffer for loudness measurement
    std::vector<double> kFilteredBuffer;  // K-filtered buffer
    int bufferWritePos = 0;
    std::vector<double> kFilterState;     // State for K-filter (2 elements for biquad)
    int kFilterStateIndex = 0;
    
    // True peak detection and limiting
    std::vector<double> oversampledBuffer; // Buffer for oversampled processing
    int oversampledIndex = 0;
    bool truePeakLimiterEnabled = true;
    bool adaptiveReleaseEnabled = true;
    
    // Tube-specific parameters
    double tubeGain = 20.0;               // Base tube gain
    double tubeLoudnessFactor;            // How much the tube's characteristics contribute
    
    // Sample rate for time constants
    double sampleRate = 44100.0;
    double effectiveSampleRate = 44100.0; // Sample rate including oversampling

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                   // For external control
    int ceilingPin = 3;                   // For ceiling control

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double ceilingSignal = 0.0;

    // Initialize limiter based on type
    void initializeLimiter(LoudnessLimiterType type);

    // Process signal through loudness limiting algorithm
    void processSignal();

    // Calculate K-weighted loudness
    double calculateKWeightedLoudness(const std::vector<double>& buffer, int start, int length);

    // Update loudness measurements
    void updateLoudnessMeasurements();

    // Calculate required gain reduction based on loudness
    double calculateLimiterGain();

    // Apply K-filter to simulate human hearing
    double applyKFilter(double input);

    // Calculate true peak
    double calculateTruePeak(const std::vector<double>& buffer, int start, int length);
};

// Class for dedicated tube-based limiter circuits
class TubeLimiter : public ElectricNodeBase {
public:
    enum LimiterType {
        PLAIN_LIMITER,      // Simple peak limiter
        DEESSING_LIMITER,   // Limiter with de-essing capabilities
        RMS_LIMITER,        // RMS-based limiter
        VARI_MU_LIMITER     // Variable-Mu style limiter
    };

    TubeLimiter(LimiterType type = PLAIN_LIMITER);
    virtual ~TubeLimiter() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure limiter parameters
    void setThreshold(double threshold);    // in dB (typically close to 0dB)
    void setCeiling(double ceiling);        // in dB (hard limit - typically -0.1 to -1.0)
    void setAttackTime(double time);        // in seconds (very fast for limiting)
    void setReleaseTime(double time);       // in seconds (adaptive for natural sound)
    void setMakeupGain(double gain);        // in dB
    void setSoftKnee(double kneeWidth);     // width of soft knee transition
    void setOvershootProtection(bool enable); // Enable overshoot protection

    // Get parameters
    double getThreshold() const { return threshold; }
    double getCeiling() const { return ceiling; }
    double getAttackTime() const { return attackTime; }
    double getReleaseTime() const { return releaseTime; }
    double getMakeupGain() const { return makeupGain; }
    double getSoftKnee() const { return kneeWidth; }
    bool getOvershootProtection() const { return overshootProtection; }

    // Enable/disable features
    void enableAutoRelease(bool enable) { autoReleaseEnabled = enable; }
    void enableSoftClipping(bool enable) { softClippingEnabled = enable; }

private:
    LimiterType limiterType;

    // Limiting parameters
    double threshold = -0.5;        // Threshold in dB (close to 0dB)
    double ceiling = -0.1;          // Ceiling in dB (hard limit)
    double attackTime = 0.0005;     // Very fast attack (0.5ms)
    double releaseTime = 0.05;      // 50ms release (adaptive)
    double makeupGain = 0.0;        // Makeup gain in dB
    double kneeWidth = 1.0;         // Width of soft knee transition
    bool overshootProtection = true; // Enable overshoot protection

    // Tube-specific parameters
    double tubeGain = 25.0;         // Higher tube gain for limiter
    double tubeLimitingFactor;      // How much the tube's characteristics contribute
    double sidechainCoupling = 0.9; // Higher coupling for limiter

    // Dynamic parameters
    double detectorLevel = 0.0;     // Detected level for limiting
    double limiterGain = 1.0;       // Current gain reduction
    double attackCoeff = 0.0;       // Coefficient for attack calculation
    double releaseCoeff = 0.0;      // Coefficient for release calculation

    // Circuit type parameters
    bool autoReleaseEnabled = true; // Adaptive release
    bool softClippingEnabled = true; // Soft clipping post-limiting

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;             // For external control of limiting
    int sidechainPin = 3;           // For external sidechain input

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double sidechainSignal = 0.0;

    // Initialize limiter based on type
    void initializeLimiter(LimiterType type);

    // Process signal through limiting algorithm
    void processSignal();

    // Calculate limiting gain based on input level
    double calculateLimitingGain(double inputLevel);

    // Update the detector level
    void updateDetector();
};

// Class for tube-based harmonic exciter circuits
class TubeHarmonicExciter : public ElectricNodeBase {
public:
    enum ExciterType {
        ODD_HARMONIC,          // Emphasizes odd harmonics (rich, warm)
        EVEN_HARMONIC,         // Emphasizes even harmonics (sweet, musical)
        BALANCED_HARMONIC,     // Balanced mix of odd and even
        FORMANT_EXCITER        // Exciter with formant control for tonal shaping
    };

    TubeHarmonicExciter(ExciterType type = ODD_HARMONIC);
    virtual ~TubeHarmonicExciter() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure exciter parameters
    void setAmount(double amount);              // 0.0 to 1.0, amount of harmonic excitation
    void setOddEvenBalance(double balance);     // -1.0 to 1.0, -1 = all odd, 1 = all even
    void setFrequencyRange(double low, double high); // Frequency range in Hz
    void setHarmonicOrder(int order);           // Maximum harmonic order to generate
    void setToneControl(double tone);           // Tone shaping parameter
    void setFormantFrequency(double freq);      // Formant frequency (for FORMANT_EXCITER type)

    // Get parameters
    double getAmount() const { return amount; }
    double getOddEvenBalance() const { return oddEvenBalance; }
    double getLowFrequency() const { return lowFrequency; }
    double getHighFrequency() const { return highFrequency; }
    int getHarmonicOrder() const { return harmonicOrder; }
    double getToneControl() const { return toneControl; }
    double getFormantFrequency() const { return formantFreq; }

    // Enable/disable features
    void enableTubeSaturation(bool enable) { tubeSaturationEnabled = enable; }
    void enableAdaptiveExcitation(bool enable) { adaptiveExcitationEnabled = enable; }

private:
    ExciterType exciterType;

    // Exciter parameters
    double amount = 0.3;                // Amount of harmonic excitation (0.0 to 1.0)
    double oddEvenBalance = 0.0;        // Balance between odd/even harmonics (-1.0 to 1.0)
    double lowFrequency = 100.0;        // Low frequency of range to excite (Hz)
    double highFrequency = 8000.0;      // High frequency of range to excite (Hz)
    int harmonicOrder = 7;              // Maximum harmonic order to generate
    double toneControl = 0.5;           // Tone shaping parameter
    double formantFreq = 1000.0;        // Formant frequency (for formant exciter)

    // Internal state
    double previousInput = 0.0;
    double previousOutput = 0.0;
    double adaptiveGain = 1.0;          // Adaptive gain based on signal characteristics
    double harmonicResonance = 0.8;     // Resonance of harmonic generation

    // Processing parameters
    bool tubeSaturationEnabled = true;
    bool adaptiveExcitationEnabled = true;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int amountPin = 2;                  // For external amount control
    int balancePin = 3;                 // For external balance control

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double amountControl = 0.0;
    double balanceControl = 0.0;

    // Initialize exciter based on type
    void initializeExciter(ExciterType type);

    // Process signal through excitation algorithm
    void processSignal();

    // Generate harmonics for the input signal
    double generateHarmonics(double input, double freq);

    // Calculate frequency of the input signal (simplified)
    double estimateFrequency();
};

// Class for tube-based tape-harmonics emulator circuits
class TubeTapeHarmonics : public ElectricNodeBase {
public:
    enum TapeType {
        FERRIC_456,           // Ampex 456, 3M/Scotch 112, Quantegy 400
        FERRIC_911,           // Ampex 911, 3M/Scotch 114, Quantegy 450
        CHROME_TYPE_2,        // Chrome Type II (more highs)
        METAL_TYPE_4,         // Metal Type IV (highest highs)
        VINTAGE_REEL_TO_REEL  // Vintage reel-to-reel sound
    };

    TubeTapeHarmonics(TapeType type = FERRIC_456);
    virtual ~TubeTapeHarmonics() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure tape parameters
    void setAmount(double amount);              // 0.0 to 1.0, amount of tape effect
    void setBias(double bias);                  // Bias level (0.0 to 1.0)
    void setSpeed(double speed);                // Tape speed factor (0.5 to 2.0)
    void setNoiseLevel(double noise);           // Noise level (0.0 to 1.0)
    void setCompression(double compression);    // Magnetic compression (0.0 to 1.0)
    void setWowFlutter(double wow);             // Wow and flutter amount (0.0 to 1.0)

    // Get parameters
    double getAmount() const { return amount; }
    double getBias() const { return bias; }
    double getSpeed() const { return speed; }
    double getNoiseLevel() const { return noiseLevel; }
    double getCompression() const { return compression; }
    double getWowFlutter() const { return wowFlutter; }

    // Enable/disable features
    void enableTapeCompression(bool enable) { tapeCompressionEnabled = enable; }
    void enableNoise(bool enable) { noiseEnabled = enable; }
    void enableWowFlutter(bool enable) { wowFlutterEnabled = enable; }

private:
    TapeType tapeType;

    // Tape parameters
    double amount = 0.4;                // Amount of tape effect (0.0 to 1.0)
    double bias = 0.7;                  // Bias level (controls harmonic content)
    double speed = 1.0;                 // Tape speed factor (affects frequency response)
    double noiseLevel = 0.02;           // Noise level (0.0 to 1.0)
    double compression = 0.3;           // Magnetic compression (0.0 to 1.0)
    double wowFlutter = 0.05;           // Wow and flutter amount (0.0 to 1.0)

    // Internal state
    double previousInput = 0.0;
    double previousOutput = 0.0;
    double tapeHeadPosition = 0.0;      // For simulating tape head position
    double wowPhase = 0.0;              // Phase for wow and flutter
    double noiseBuffer[10] = {0.0};     // Buffer for noise generation
    int noiseIndex = 0;

    // Processing parameters
    bool tapeCompressionEnabled = true;
    bool noiseEnabled = true;
    bool wowFlutterEnabled = true;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int amountPin = 2;                  // For external amount control
    int biasPin = 3;                    // For external bias control

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double amountControl = 0.0;
    double biasControl = 0.0;

    // Initialize tape simulator based on type
    void initializeTape(TapeType type);

    // Process signal through tape algorithm
    void processSignal();

    // Apply tape compression
    double applyTapeCompression(double input);

    // Generate tape noise
    double generateTapeNoise();

    // Apply wow and flutter
    double applyWowFlutter(double input, double phase);
};

// Class for tube-based parametric equalizer circuits
class TubeParametricEQ : public ElectricNodeBase {
public:
    enum EQBandType {
        LOW_SHELF,    // Low shelving filter
        PEAKING,      // Peaking filter for midrange
        HIGH_SHELF,   // High shelving filter
        LOW_PASS,     // Low-pass filter
        HIGH_PASS,    // High-pass filter
        BAND_PASS     // Band-pass filter
    };

    // Structure for a single EQ band
    struct EQBand {
        EQBandType type = PEAKING;
        double frequency = 1000.0;  // Center/corner frequency in Hz
        double gain = 0.0;          // Gain in dB (-15 to 15)
        double q = 0.707;           // Quality factor (resonance)
        bool enabled = true;        // Whether this band is active
        
        // Filter coefficients (calculated in process)
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a0 = 1.0, a1 = 0.0, a2 = 0.0;
        
        // Filter state (for IIR filter implementation)
        double x1 = 0.0, x2 = 0.0;  // Previous input values
        double y1 = 0.0, y2 = 0.0;  // Previous output values
    };

    TubeParametricEQ(int numBands = 4);  // Default to 4 bands
    virtual ~TubeParametricEQ() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure EQ parameters
    void setNumBands(int numBands);                       // Change number of bands
    void setBandType(int bandIndex, EQBandType type);    // Set type for a specific band
    void setBandFrequency(int bandIndex, double freq);   // Set frequency for a band
    void setBandGain(int bandIndex, double gain);        // Set gain for a band
    void setBandQ(int bandIndex, double q);              // Set Q for a band
    void enableBand(int bandIndex, bool enable);         // Enable/disable a band

    // Get parameters
    int getNumBands() const { return static_cast<int>(bands.size()); }
    EQBandType getBandType(int bandIndex) const { return bands[bandIndex].type; }
    double getBandFrequency(int bandIndex) const { return bands[bandIndex].frequency; }
    double getBandGain(int bandIndex) const { return bands[bandIndex].gain; }
    double getBandQ(int bandIndex) const { return bands[bandIndex].q; }
    bool isBandEnabled(int bandIndex) const { return bands[bandIndex].enabled; }

    // Enable/disable features
    void enableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void setSaturation(double saturation) { tubeSaturation = std::max(0.0, std::min(1.0, saturation)); }

private:
    std::vector<EQBand> bands;
    int numBands;

    // Processing parameters
    bool tubeCharacteristicsEnabled = true;
    double tubeSaturation = 0.3;  // How much tube saturation to apply

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;

    double inputSignal = 0.0;
    double outputSignal = 0.0;

    // Initialize EQ with specified number of bands
    void initializeEQ(int numBands);

    // Process signal through parametric EQ algorithm
    void processSignal();

    // Calculate filter coefficients for a band
    void calculateFilterCoefficients(int bandIndex);

    // Process sample through a single band
    double processBand(int bandIndex, double input);
};

// Class for tube-based stereo widener circuits
class TubeStereoWidener : public ElectricNodeBase {
public:
    enum WidenerType {
        MID_SIDE_WIDENER,     // Mid-side based widener with delay
        BAND_PASS_WIDENER,    // 10-band splitter with frequency-specific delays
        ALL_PASS_WIDENER,     // All-pass filter based widener
        PHASE_WIDENER         // Phase-based widener
    };

    TubeStereoWidener(WidenerType type = MID_SIDE_WIDENER);
    virtual ~TubeStereoWidener() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure widener parameters
    void setAmount(double amount);              // 0.0 to 1.0, amount of widening
    void setDelayTime(double time);             // Delay time in ms (0.0 to 10.0)
    void setWidth(double width);                // Stereo width (0.0 to 2.0, 1.0 = normal)
    void setMidSideRatio(double ratio);         // Balance between mid and side processing
    void setFrequencyBand(int band, double freq); // Set frequency for each band (for BAND_PASS_WIDENER)

    // Get parameters
};

// Class for tube-based mid/side splitter circuits
class TubeSideMidSplitter : public ElectricNodeBase {
public:
    enum SplitterType {
        PASSIVE_SPLITTER,      // Simple passive L/R to M/S conversion
        ACTIVE_SPLITTER,       // Active splitter with amplification stages
        TUBE_SPLITTER,         // Tube-based splitter with tube characteristics
        VARIABLE_SPLITTER      // Variable ratio of mid/side emphasis
    };

    TubeSideMidSplitter(SplitterType type = TUBE_SPLITTER);
    virtual ~TubeSideMidSplitter() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure splitter parameters
    void setMidSideRatio(double ratio);           // 0.0 = all side, 1.0 = all mid
    void setTubeSaturation(double saturation);    // 0.0 to 1.0, amount of tube saturation
    void setHarmonicContent(double content);      // 0.0 to 1.0, amount of harmonic enhancement
    void setGain(double gain);                    // Overall gain

    // Get parameters
    double getMidSideRatio() const { return midSideRatio; }
    double getTubeSaturation() const { return tubeSaturation; }
    double getHarmonicContent() const { return harmonicContent; }
    double getGain() const { return gain; }

    // Enable/disable features
    void enableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void enableHarmonicEnhancement(bool enable) { harmonicEnhancementEnabled = enable; }

private:
    SplitterType splitterType;

    // Splitter parameters
    double midSideRatio = 0.5;          // Balance between mid and side (0.0=all side, 1.0=all mid)
    double tubeSaturation = 0.3;        // Amount of tube saturation
    double harmonicContent = 0.2;       // Amount of harmonic enhancement
    double gain = 1.0;                  // Overall gain

    // Processing parameters
    bool tubeCharacteristicsEnabled = true;
    bool harmonicEnhancementEnabled = true;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections (stereo in/out)
    int leftInput = 0;
    int rightInput = 1;
    int midOutput = 2;
    int sideOutput = 3;
    int midSideControl = 4;             // For external control of mid/side ratio

    double leftInputSignal = 0.0;
    double rightInputSignal = 0.0;
    double midOutputSignal = 0.0;
    double sideOutputSignal = 0.0;
    double midSideControlSignal = 0.0;

    // Initialize splitter based on type
    void initializeSplitter(SplitterType type);

    // Process signal through mid/side splitting algorithm
    void processSignal();

    // Apply tube characteristics to the signals
    double applyTubeCharacteristics(double signal, double tubeSaturation);
};

// Class for tube-based flanger circuits
    double getAmount() const { return amount; }
    double getDelayTime() const { return delayTimeMs; }
    double getWidth() const { return width; }
    double getMidSideRatio() const { return midSideRatio; }

    // Enable/disable features
    void enableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void setBandCount(int count) { bandCount = std::max(2, std::min(10, count)); }

private:
    WidenerType widenerType;

    // Widener parameters
    double amount = 0.5;                // Amount of widening (0.0 to 1.0)
    double delayTimeMs = 1.0;           // Delay time in milliseconds
    double width = 1.0;                 // Stereo width factor
    double midSideRatio = 0.5;          // Balance between mid and side (0.0=all mid, 1.0=all side)
    int bandCount = 10;                 // Number of frequency bands (for BAND_PASS_WIDENER)
    std::vector<double> bandFrequencies; // Frequencies for each band

    // Processing parameters
    bool tubeCharacteristicsEnabled = true;
    double tubeSaturation = 0.2;        // How much tube saturation to apply

    // Delay buffers for stereo widening
    std::vector<double> leftDelayBuffer;
    std::vector<double> rightDelayBuffer;
    int delayBufferSize = 0;
    int leftWritePos = 0;
    int rightWritePos = 0;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections (stereo in/out)
    int leftInput = 0;
    int rightInput = 1;
    int leftOutput = 2;
    int rightOutput = 3;
    int controlInput = 4;               // For external control of amount

    double leftInputSignal = 0.0;
    double rightInputSignal = 0.0;
    double leftOutputSignal = 0.0;
    double rightOutputSignal = 0.0;
    double controlSignal = 0.0;

    // Initialize widener based on type
    void initializeWidener(WidenerType type);

    // Process signal through stereo widening algorithm
    void processSignal();

    // Convert stereo to mid-side and back
    void stereoToMidSide(double left, double right, double& mid, double& side);
    void midSideToStereo(double mid, double side, double& left, double& right);

    // Process frequency bands (for band-pass widener)
    void processFrequencyBands(double& left, double& right);
};

// Class for tube-based auto-wah circuits
class TubeAutoWah : public ElectricNodeBase {
public:
    enum AutoWahType {
        CONTOUR_FILTER,        // Classic envelope-controlled filter
        TRACKING_FILTER,       // Pitch-tracking auto-wah
        DUAL_FILTER,           // Two filters with different tracking
        RESONANT_WAH           // Highly resonant auto-wah
    };

    TubeAutoWah(AutoWahType type = CONTOUR_FILTER);
    virtual ~TubeAutoWah() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure auto-wah parameters
    void setSensitivity(double sensitivity);         // 0.0 to 1.0, envelope detection sensitivity
    void setAttackTime(double time);                 // Attack time in seconds (0.01 to 0.5)
    void setReleaseTime(double time);                // Release time in seconds (0.1 to 1.0)
    void setMinFrequency(double freq);               // Minimum filter frequency in Hz
    void setMaxFrequency(double freq);               // Maximum filter frequency in Hz
    void setResonance(double res);                   // Filter resonance (Q factor)
    void setWetDryMix(double mix);                   // Wet/dry mix (0.0 to 1.0)

    // Get parameters
    double getSensitivity() const { return sensitivity; }
    double getAttackTime() const { return attackTime; }
    double getReleaseTime() const { return releaseTime; }
    double getMinFrequency() const { return minFrequency; }
    double getMaxFrequency() const { return maxFrequency; }
    double getResonance() const { return resonance; }
    double getWetDryMix() const { return wetDryMix; }

    // Enable/disable features
    void enableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void setHarmonicContent(double content) { harmonicContent = std::max(0.0, std::min(1.0, content)); }

private:
    AutoWahType autoWahType;

    // Auto-wah parameters
    double sensitivity = 0.7;             // Sensitivity of envelope detection
    double attackTime = 0.03;             // Attack time in seconds
    double releaseTime = 0.2;             // Release time in seconds
    double minFrequency = 200.0;          // Minimum filter frequency
    double maxFrequency = 1200.0;         // Maximum filter frequency
    double resonance = 3.0;               // Filter resonance/Q
    double wetDryMix = 0.8;               // Wet/dry mix (1.0 = fully wet)
    double harmonicContent = 0.2;         // Harmonic content added by tube simulation

    // Filter coefficients (for time-varying filter)
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a0 = 1.0, a1 = 0.0, a2 = 0.0;

    // Filter state
    double x1 = 0.0, x2 = 0.0;            // Previous input values
    double y1 = 0.0, y2 = 0.0;            // Previous output values

    // Envelope follower state
    double envelope = 0.0;                // Current envelope value
    double attackCoeff = 0.0;             // Attack coefficient
    double releaseCoeff = 0.0;            // Release coefficient
    double lastInput = 0.0;               // Previous input for peak detection

    // Processing parameters
    bool tubeCharacteristicsEnabled = true;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int sensitivityPin = 2;               // For external sensitivity control

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double sensitivityControl = 0.0;

    // Initialize auto-wah based on type
    void initializeAutoWah(AutoWahType type);

    // Process signal through auto-wah algorithm
    void processSignal();

    // Update the filter coefficients based on current envelope
    void updateFilterCoefficients();

    // Process sample through the filter
    double processFilter(double input);

    // Update the envelope follower
    void updateEnvelope();
};

// Class for tube-based Moog resonant filter circuits
class TubeMoogFilter : public ElectricNodeBase {
public:
    enum FilterType {
        LOW_PASS,       // Classic Moog low-pass
        HIGH_PASS,      // High-pass derived from low-pass
        BAND_PASS,      // Band-pass derived from low-pass
        BAND_REJECT,    // Band-reject (notch) filter
        ALL_PASS        // All-pass filter for phase effects
    };

    TubeMoogFilter(FilterType type = LOW_PASS);
    virtual ~TubeMoogFilter() = default;

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configure Moog filter parameters
    void setCutoff(double freq);           // Cutoff frequency in Hz (20 to 20000)
    void setResonance(double res);         // Resonance (0.0 to 1.0, but 0.0 to 0.99 in practice)
    void setDrive(double drive);           // Drive/input gain for distortion (1.0 to 10.0)
    void setType(FilterType type);         // Set filter type (LOW_PASS, etc.)
    void setStability(double stability);   // Stability control (0.0 to 1.0)

    // Get parameters
    double getCutoff() const { return cutoffFreq; }
    double getResonance() const { return resonance; }
    double getDrive() const { return drive; }
    FilterType getType() const { return filterType; }
    double getStability() const { return stability; }

    // Enable/disable features
    void enableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void setSaturation(double saturation) { tubeSaturation = std::max(0.0, std::min(1.0, saturation)); }

private:
    FilterType filterType;

    // Filter parameters
    double cutoffFreq = 1000.0;          // Cutoff frequency in Hz
    double resonance = 0.5;              // Resonance amount (0.0 to 0.99)
    double drive = 1.0;                  // Input drive/gain
    double stability = 0.8;              // Stability control
    double tubeSaturation = 0.3;         // Tube saturation amount

    // Moog filter state (4-stage ladder filter simulation)
    double stage1 = 0.0, stage2 = 0.0, stage3 = 0.0, stage4 = 0.0;
    double inputLP = 0.0;                // Low-pass input with feedback
    double lastOutput = 0.0;             // Last output for feedback

    // Processing parameters
    bool tubeCharacteristicsEnabled = true;

    // Sample rate for time constants
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int cutoffPin = 2;                   // For external cutoff control
    int resonancePin = 3;                // For external resonance control

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double cutoffControl = 0.0;
    double resonanceControl = 0.0;

    // Initialize Moog filter based on type
    void initializeFilter(FilterType type);

    // Process signal through Moog filter algorithm
    void processSignal();

    // Calculate one sample of the Moog filter
    double processMoogFilter(double input);
};

// Class for tube-based flanger circuits
class TubeFlanger : public ElectricNodeBase {
public:
    TubeFlanger();
    virtual ~TubeFlanger() = default;
    
    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;
    
    // Configure flanger parameters
    void setLFOFrequency(double freq);      // LFO rate in Hz
    void setDepth(double depth);            // 0.0 to 1.0
    void setFeedback(double feedback);      // -1.0 to 1.0
    void setBaseDelay(double delay);        // Base delay in seconds
    void setManual(double manual);          // Manual setting for delay position
    
    // Get parameters
    double getLFOFrequency() const { return lfoFrequency; }
    double getDepth() const { return depth; }
    double getFeedback() const { return feedback; }
    double getBaseDelay() const { return baseDelay; }
    double getManual() const { return manualSetting; }

private:
    // Flanger parameters
    double lfoFrequency = 0.25;     // LFO rate in Hz (typically slower than chorus)
    double depth = 0.6;             // Modulation depth
    double feedback = 0.5;          // Feedback amount
    double baseDelay = 0.001;       // Base delay (1ms)
    double manualSetting = 0.5;     // Manual delay position
    
    // LFO parameters
    double lfoPhase = 0.0;
    
    // Delay line
    std::vector<double> delayBuffer;
    int bufferSize;
    int writePosition = 0;
    
    // Feedback parameters
    double feedbackBuffer = 0.0;
    
    // Sample rate
    double sampleRate = 44100.0;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int ratePin = 2;
    int depthPin = 3;
    int feedbackPin = 4;
    
    double inputSignal = 0.0;
    double outputSignal = 0.0;
    
    // Initialize flanger
    void initializeFlanger();
    
    // Process signal through flanger effect
    void processSignal();
    
    // Update LFO
    void updateLFO();
    
    // Process through delay line
    double processDelayLine(double input);
};

#endif // TUBE_EFFECT_CIRCUITS_H