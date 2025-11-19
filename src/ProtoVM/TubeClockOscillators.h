#ifndef TUBE_CLOCK_OSCILLATORS_H
#define TUBE_CLOCK_OSCILLATORS_H

#include "Common.h"
#include "TubeComponents.h"
#include "TubeFiltersOscillators.h"
#include <vector>
#include <memory>

// Class for tube-based clock oscillator
class TubeClockOscillator : public ElectricNodeBase {
public:
    enum OscillatorType {
        HARTLEY,
        COLPITTS,
        PIERCE,
        WIEN_BRIDGE,
        PHASE_SHIFT,
        RELAXATION,
        RING
    };
    
    TubeClockOscillator(OscillatorType type = WIEN_BRIDGE, double frequency = 1000.0);
    virtual ~TubeClockOscillator() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Configure oscillator
    void setFrequency(double freq);
    void setWaveform(TubeOscillatorCircuit::Waveform wf) { waveform = wf; }
    void setAmplitude(double amp) { amplitude = amp; }
    void setOscillatorType(OscillatorType type) { oscillatorType = type; }
    void setEnable(bool enable) { enabled = enable; }
    void setSync(bool sync) { syncEnabled = sync; }
    void setSyncSignal(double sync) { syncSignal = sync; }
    
    // Get parameters
    double getFrequency() const { return frequency; }
    double getAmplitude() const { return amplitude; }
    bool isEnabled() const { return enabled; }
    bool isRunning() const { return oscillating; }
    
    // Get output
    double getOutput() const { return currentOutput; }
    bool getClockOutput() const { return clockOutput; }

private:
    OscillatorType oscillatorType;
    TubeOscillatorCircuit::Waveform waveform;
    
    // Oscillation parameters
    double frequency;
    double amplitude = 1.0;
    bool enabled = true;
    bool oscillating = false;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double sampleRate = 44100.0;
    
    // Sync parameters
    bool syncEnabled = false;
    double syncSignal = 0.0;
    
    // Output state
    double currentOutput = 0.0;
    bool clockOutput = false;
    double previousOutput = 0.0;
    bool previousClock = false;
    
    // For square wave clock output generation
    double threshold = 0.0;
    bool positiveGoing = true;
    
    // Pin connections
    int outputPin = 0;
    int clockOutputPin = 1;  // Digital clock output
    int frequencyControlPin = 2;
    int enablePin = 3;
    int syncPin = 4;
    int bPlusPin = 5;
    
    // Tube oscillator components
    std::unique_ptr<TubeOscillatorCircuit> oscillator;
    
    // Calculate the next sample of the oscillator
    double generateNextSample();
    
    // Initialize oscillator based on type
    void initializeOscillator();
    
    // Update phase increment based on frequency
    void updatePhaseIncrement();
};

// Class for tube-based clock divider for generating lower frequencies
class TubeFrequencyDivider : public ElectricNodeBase {
public:
    TubeFrequencyDivider(int divideFactor = 2);
    virtual ~TubeFrequencyDivider() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Configure divider
    void setDivideFactor(int factor);
    void setEnable(bool enable) { enabled = enable; }
    void setReset(bool reset) { 
        if (reset) {
            resetCounter();
        }
    }
    
    // Get parameters
    int getDivideFactor() const { return divideFactor; }
    bool isEnabled() const { return enabled; }
    bool getOutput() const { return output; }
    
    // Reset the counter
    void resetCounter();

private:
    int divideFactor;
    int currentCount = 0;
    bool output = false;
    bool enabled = true;
    bool previousInput = false;
    
    // Pin connections
    int inputPin = 0;
    int outputPin = 1;
    int enablePin = 2;
    int resetPin = 3;
    
    void processInput(bool input);
};

// Class for tube-based Phase-Locked Loop (PLL) simulation
class TubePLL : public ElectricNodeBase {
public:
    TubePLL();
    virtual ~TubePLL() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Configure PLL
    void setReferenceFrequency(double freq) { referenceFreq = freq; }
    void setVCOFrequency(double freq) { vco.setFrequency(freq); }
    void setLoopFilterCutoff(double cutoff) { loopFilterCutoff = cutoff; }
    void setPhaseDetectorType(int type) { phaseDetectorType = type; }
    
    // Get parameters
    double getReferenceFrequency() const { return referenceFreq; }
    double getVCOFrequency() const { return vco.getFrequency(); }
    bool getOutput() const { return output; }
    
    // Access to internal components
    TubeVCO* getVCO() { return &vco; }

private:
    // Reference clock parameters
    double referenceFreq = 1000.0;
    double sampleRate = 44100.0;
    bool referenceClock = false;
    double referencePhase = 0.0;
    
    // VCO parameters
    TubeVCO vco;
    double vcoFreq = 1000.0;
    double controlVoltage = 0.0;
    
    // Phase detector parameters
    bool referenceEdge = false;
    bool feedbackEdge = false;
    double phaseError = 0.0;
    int phaseDetectorType = 0;  // 0=Simple XOR, 1=Multiplier, 2=Phase/Frequency
    
    // Loop filter parameters
    double loopFilterCutoff = 10.0;
    double loopFilterState = 0.0;
    
    // Output
    bool output = false;
    
    // Pin connections
    int referencePin = 0;
    int outputPin = 1;
    int controlPin = 2;
    int resetPin = 3;
    
    bool previousRefClock = false;
    bool previousVCOClock = false;
    
    void processPhaseDetection();
    void processLoopFilter();
    void updateVCO();
};

// Class for frequency synthesizer using tubes
class TubeFrequencySynthesizer : public ElectricNodeBase {
public:
    enum SynthesisMethod {
        DIRECT_ANALOG,      // Direct analog synthesis
        PLL_BASED,         // PLL-based synthesis
        COUNTER_BASED     // Counter-based division/multiplication
    };
    
    TubeFrequencySynthesizer(SynthesisMethod method = PLL_BASED);
    virtual ~TubeFrequencySynthesizer() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Configure synthesizer
    void setOutputFrequency(double freq);
    void setReferenceFrequency(double freq) { referenceFreq = freq; }
    void setSynthesisMethod(SynthesisMethod method) { synthesisMethod = method; }
    void setEnable(bool enable) { enabled = enable; }
    
    // Get parameters
    double getOutputFrequency() const { return outputFreq; }
    double getReferenceFrequency() const { return referenceFreq; }
    bool isEnabled() const { return enabled; }
    bool isLocked() const { return locked; }
    
    // Access to internal components
    TubePLL* getPLL() { return pll.get(); }
    TubeClockOscillator* getOscillator() { return oscillator.get(); }

private:
    SynthesisMethod synthesisMethod;
    
    // Parameters
    double referenceFreq = 1000.0;  // Reference frequency
    double outputFreq = 1000.0;     // Desired output frequency
    bool enabled = true;
    bool locked = false;
    
    // Internal components
    std::unique_ptr<TubePLL> pll;
    std::unique_ptr<TubeClockOscillator> oscillator;
    std::unique_ptr<TubeFrequencyDivider> predivider;
    std::unique_ptr<TubeFrequencyDivider> postdivider;
    
    // Division ratios for frequency synthesis
    int nCounter = 1;  // Divide by N in feedback
    int rCounter = 1;  // Divide by R at reference
    int aCounter = 0;  // Integer part of division in some architectures
    
    // Current output
    bool output = false;
    
    // Pin connections
    int referencePin = 0;
    int outputPin = 1;
    int controlPin = 2;
    int enablePin = 3;
    int resetPin = 4;
    
    void configurePLL();
    void updateOutput();
};

// Class for a complete tube-based clock generation system
class TubeClockSystem : public ElectricNodeBase {
public:
    TubeClockSystem();
    virtual ~TubeClockSystem() = default;
    
    virtual bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override;
    virtual bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;
    /*virtual bool GetRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override;*/
    virtual bool Tick() override;
    
    // Set master frequency
    void setMasterFrequency(double freq) { 
        masterOsc->setFrequency(freq);
        masterFreq = freq;
    }
    
    // Get derived frequencies
    double getMasterFrequency() const { return masterFreq; }
    double getHalfFrequency() const { return masterFreq / 2.0; }
    double getQuarterFrequency() const { return masterFreq / 4.0; }
    double getEighthFrequency() const { return masterFreq / 8.0; }
    
    // Enable/disable system
    void setEnable(bool enable) { 
        enabled = enable;
        masterOsc->setEnable(enable);
        for (auto& divider : freqDividers) {
            divider->setEnable(enable);
        }
    }
    
    // Get clock outputs
    bool getMasterClock() const { return masterOsc->getClockOutput(); }
    bool getHalfClock() const { return freqDividers[0]->getOutput(); }
    bool getQuarterClock() const { return freqDividers[1]->getOutput(); }
    bool getEighthClock() const { return freqDividers[2]->getOutput(); }

private:
    double masterFreq = 1000.0;  // Default 1kHz master clock
    bool enabled = true;
    
    // Master oscillator
    std::unique_ptr<TubeClockOscillator> masterOsc;
    
    // Frequency dividers for generating harmonics
    std::vector<std::unique_ptr<TubeFrequencyDivider>> freqDividers;
    
    // Pin connections
    int masterClockPin = 0;
    int halfClockPin = 1;
    int quarterClockPin = 2;
    int eighthClockPin = 3;
    int enablePin = 4;
    int resetAllPin = 5;
    
    void initializeSystem();
};

#endif // TUBE_CLOCK_OSCILLATORS_H