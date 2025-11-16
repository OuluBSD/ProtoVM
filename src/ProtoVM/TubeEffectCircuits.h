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