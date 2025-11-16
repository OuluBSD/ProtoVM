#ifndef _ProtoVM_TubePitchShifter_h_
#define _ProtoVM_TubePitchShifter_h_

#include "AnalogCommon.h"
#include "TubeModels.h"
#include "AnalogComponents.h"
#include <vector>
#include <memory>
#include <cmath>

// Class for tube-based pitch shifter circuits for pitch correction and creative effects
class TubePitchShifter : public ElectricNodeBase {
public:
    typedef TubePitchShifter CLASSNAME;

    enum PitchShifterType {
        MONO_PITCH_SHIFTER,      // Basic mono pitch shifter
        STEREO_PITCH_SHIFTER,    // Stereo pitch shifter
        HARMONIC_GENERATOR,      // Generates harmonics of input
        PITCH_CORRECTOR          // Corrects pitch to specific notes
    };

    TubePitchShifter(PitchShifterType type = MONO_PITCH_SHIFTER);
    virtual ~TubePitchShifter();

    virtual bool Process(int op, uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    virtual bool Tick() override;

    // Configuration methods for pitch shifter parameters
    void SetPitchShift(double semitones);      // Pitch shift in semitones (-12 to +12)
    void SetFormantPreservation(bool preserve); // Whether to preserve formants
    void SetBlend(double dryWet);              // Dry/wet blend (0.0 to 1.0)
    void SetFeedback(double feedback);         // Feedback amount (0.0 to 0.95)
    void SetHarmonicMix(double mix);           // Harmonic content mix
    void SetOctaveDivision(int divisions);     // For sub-octave effects (1 to 4)
    void SetTuneToNote(int note);              // For pitch corrector (MIDI note number)
    void SetCorrectionStrength(double strength); // Correction strength (0.0 to 1.0)
    void SetWindowSize(int samples);           // Window size for processing

    // Get parameters
    double GetPitchShift() const { return pitchShiftSemitones; }
    bool GetFormantPreservation() const { return formantPreservation; }
    double GetBlend() const { return dryWetBlend; }
    double GetFeedback() const { return feedback; }
    double GetHarmonicMix() const { return harmonicMix; }
    int GetOctaveDivision() const { return octaveDivision; }
    int GetTuneToNote() const { return tuneToNote; }
    double GetCorrectionStrength() const { return correctionStrength; }
    int GetWindowSize() const { return windowSize; }

    // Enable/disable features
    void EnableTubeCharacteristics(bool enable) { tubeCharacteristicsEnabled = enable; }
    void EnableHarmonicGeneration(bool enable) { harmonicGenerationEnabled = enable; }

private:
    PitchShifterType shifterType;

    // Pitch shifter parameters
    double pitchShiftSemitones = 0.0;    // Pitch shift in semitones (-24 to +24)
    bool formantPreservation = true;     // Whether to preserve formants during shift
    double dryWetBlend = 0.5;            // Dry/wet blend (0.0 = dry, 1.0 = wet)
    double feedback = 0.0;               // Feedback amount (0.0 to 0.95)
    double harmonicMix = 0.2;            // Harmonic content mix (0.0 to 1.0)
    int octaveDivision = 1;              // For sub-octave effects (1 to 4)
    int tuneToNote = 60;                 // For pitch corrector (MIDI note number: C4)
    double correctionStrength = 0.5;     // Correction strength (0.0 to 1.0)
    int windowSize = 1024;               // Window size for processing

    // Processing buffers and state
    std::vector<double> inputBuffer;     // Buffer for input samples
    std::vector<double> outputBuffer;    // Buffer for output samples
    std::vector<double> delayBuffer;     // Delay buffer for pitch shifting
    std::vector<double> phaseBuffer;     // Phase information for pitch shifting
    int bufferReadPos = 0;
    int bufferWritePos = 0;
    int delayWritePos = 0;
    double currentPhase = 0.0;
    double phaseIncrement = 1.0;         // Phase increment for pitch shifting
    double targetPhaseIncrement = 1.0;   // Target phase increment
    
    // Harmonic detection and generation
    double fundamentalFreq = 440.0;      // Detected fundamental frequency
    std::vector<double> harmonics;       // Harmonic components
    
    // Tube simulation parameters
    std::vector<std::unique_ptr<Tube>> tubes;
    double tubeGain = 20.0;              // Base tube gain for pitch effects

    // Circuit type parameters
    bool tubeCharacteristicsEnabled = true;   // Apply tube characteristics
    bool harmonicGenerationEnabled = false;   // Generate harmonics

    // Sample rate for calculations
    double sampleRate = 44100.0;

    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int controlPin = 2;                  // For external control of pitch shift
    int feedbackPin = 3;                 // For feedback input

    double inputSignal = 0.0;
    double outputSignal = 0.0;
    double controlSignal = 0.0;
    double feedbackSignal = 0.0;

    // Initialize pitch shifter based on type
    void InitializePitchShifter(PitchShifterType type);

    // Process signal through pitch shifting algorithm
    void ProcessSignal();

    // Estimate fundamental frequency
    double EstimateFundamentalFrequency();

    // Apply pitch shifting using phase vocoder approach
    double ApplyPitchShift();

    // Generate harmonics
    void GenerateHarmonics();

    // Apply tube characteristics
    void ApplyTubeCharacteristics();

    // Calculate phase increment based on pitch shift
    void UpdatePhaseIncrement();
};

#endif