#ifndef _ProtoVM_TubeTremolo_h_
#define _ProtoVM_TubeTremolo_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include "LFO.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based tremolo circuits for amplitude modulation effects
class TubeTremolo : public ElectricNodeBase {
public:
    typedef TubeTremolo CLASSNAME;

    enum TremoloType {
        PHOTOCELL_TREMOLO,     // Classic photcell-based tremolo
        TUBE_VARIATION,        // Tube voltage variation tremolo
        RATIO_CHANGER,         // Sine/square wave variation
        VIBRATO_TREMOLO        // Vibrato-like tremolo
    };

    TubeTremolo(TremoloType type = PHOTOCELL_TREMOLO);
    virtual ~TubeTremolo();

    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;

    // Configuration methods for tremolo parameters
    void SetLFOFrequency(double freq);           // LFO frequency in Hz
    void SetLFOAmount(double amount);            // LFO depth (0.0 to 1.0)
    void SetLFOShape(LFOType shape);             // LFO wave shape
    void SetDepth(double depth);                 // Tremolo depth (0.0 to 1.0)
    void SetTone(double tone);                   // Tone control (0.0 to 1.0)
    void SetBias(double bias);                   // DC bias for tremolo (0.0 to 1.0)
    void SetEnvelopeAmount(double amount);       // Envelope follower amount
    void SetMix(double dryWet);                  // Dry/wet mix (0.0 to 1.0)
    void SetAsymmetry(double asym);              // Waveform asymmetry (0.0 to 1.0)

    // Get parameters
    double GetLFOFrequency() const { return lfoFrequency; }
    double GetLFOAmount() const { return lfoAmount; }
    LFOType GetLFOShape() const { return lfoShape; }
    double GetDepth() const { return depth; }
    double GetTone() const { return tone; }
    double GetBias() const { return bias; }
    double GetEnvelopeAmount() const { return envelopeAmount; }
    double GetMix() const { return dryWetMix; }
    double GetAsymmetry() const { return asymmetry; }

    // Enable/disable features
    void EnableEnvelopeFollower(bool enable) { envelopeFollowerEnabled = enable; }
    void EnableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }

private:
    TremoloType tremoloType;

    // Tremolo parameters
    double lfoFrequency = 4.0;          // LFO frequency in Hz
    double lfoAmount = 1.0;             // LFO depth (0.0 to 1.0)
    LFOType lfoShape = LFOType::SINE;   // LFO wave shape
    double depth = 0.8;                 // Tremolo depth (0.0 to 1.0)
    double tone = 0.5;                  // Tone control (0.0 to 1.0)
    double bias = 0.5;                  // DC bias for tremolo (0.0 to 1.0)
    double envelopeAmount = 0.0;        // Envelope follower amount (0.0 to 1.0)
    double dryWetMix = 0.5;             // Dry/wet mix (0.0 to 1.0)
    double asymmetry = 0.0;             // Waveform asymmetry (0.0 to 1.0)

    // Modulation components
    std::unique_ptr<LFO> modulationLFO; // For LFO-based tremolo
    double currentModulation = 0.0;     // Current modulation value
    
    // Envelope follower for auto-tremolo
    double envelopeDetector = 0.0;      // Detected envelope level
    double envelopeCoeff = 0.0;         // Coefficient for envelope tracking
    
    // Tone control parameters
    double toneCoeff = 0.0;             // Coefficient for tone control
    
    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tubeGain = 20.0;             // Base tube gain for tremolo

    // Circuit parameters
    bool envelopeFollowerEnabled = false;  // Enable envelope follower
    bool tubeCharacteristicsEnabled = true; // Apply tube characteristics

    // Sample rate for calculations
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                 // For external control of tremolo

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;

    // Initialize tremolo based on type
    void InitializeTremolo(TremoloType type);

    // Process signal through tremolo algorithm
    void ProcessSignal();

    // Update envelope detector
    void UpdateEnvelopeDetector();

    // Apply tone shaping
    double ApplyToneShaping(double input);

    // Apply tube characteristics
    void ApplyTubeCharacteristics();
};

#endif