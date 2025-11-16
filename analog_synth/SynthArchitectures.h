#ifndef SYNTH_ARCHITECTURES_H
#define SYNTH_ARCHITECTURES_H

#include "Synthesizer.h"
#include "Oscillator.h"
#include "AnalogSynthADSR.h"
#include "Filter.h"
#include "LFO.h"

// Base class for different synthesizer architectures
class SynthArchitecture {
public:
    virtual ~SynthArchitecture() = default;
    virtual void noteOn(double frequency, double velocity = 1.0) = 0;
    virtual void noteOff() = 0;
    virtual double getNextSample() = 0;
    virtual void setSampleRate(int rate) = 0;
    
    // Set master volume
    void setVolume(double vol) { volume = vol; }
    double getVolume() const { return volume; }

protected:
    double volume = 1.0;
    int sampleRate = 44100;
};

// Subtractive synthesizer architecture
// Uses oscillators -> filter -> amplifier with modulation
class SubtractiveSynth : public SynthArchitecture {
public:
    SubtractiveSynth();
    virtual ~SubtractiveSynth() = default;
    
    virtual void noteOn(double frequency, double velocity = 1.0) override;
    virtual void noteOff() override;
    virtual double getNextSample() override;
    virtual void setSampleRate(int rate) override;
    
    // Set oscillator parameters
    void setOscCount(int count); // 1-4 oscillators
    void setOscWaveform(int oscIndex, Waveform wf);
    void setOscDetune(int oscIndex, double detune);  // Detune in cents
    
    // Set filter parameters
    void setFilterCutoff(double cutoff);
    void setFilterResonance(double resonance);
    void setFilterType(int type);  // 0=lowpass, 1=hipass, 2=bandpass, 3=notch
    
    // Set ADSR parameters for filter and amplitude
    void setFilterADSR(double attack, double decay, double sustain, double release);
    void setAmpADSR(double attack, double decay, double sustain, double release);
    
    // Set LFO parameters
    void setLFO1Rate(double rate);
    void setLFO1Depth(double depth);
    void setLFO1Destination(int dest);  // 0=osc, 1=filter, 2=amp
    
private:
    std::vector<Oscillator> oscillators;
    Filter filter;
    ADSR filterEnvelope;
    ADSR ampEnvelope;
    LFO lfo1;
    
    double baseFrequency;
    bool noteActive;
    
    // Modulation parameters
    int lfoDestination;  // 0=osc, 1=filter, 2=amp
    double lfoDepth;
    
    // For multiple oscillator management
    void updateOscillatorFrequencies(double baseFreq);
};

// FM synthesizer architecture
// Uses modulator -> carrier oscillator arrangement
class FMSynth : public SynthArchitecture {
public:
    FMSynth();
    virtual ~FMSynth() = default;
    
    virtual void noteOn(double frequency, double velocity = 1.0) override;
    virtual void noteOff() override;
    virtual double getNextSample() override;
    virtual void setSampleRate(int rate) override;
    
    // Set operator parameters (simplified version with 2 operators)
    void setCarrierWaveform(Waveform wf);
    void setModulatorWaveform(Waveform wf);
    
    // Set FM parameters
    void setCarrierFrequency(double freq);
    void setModulatorFrequency(double freq);
    void setModulationIndex(double index);  // How much the modulator affects the carrier
    
    // Set ADSR for carrier and modulator
    void setCarrierADSR(double attack, double decay, double sustain, double release);
    void setModulatorADSR(double attack, double decay, double sustain, double release);

private:
    Oscillator carrier;
    Oscillator modulator;
    ADSR carrierEnvelope;
    ADSR modulatorEnvelope;
    
    double modulationIndex;
    double baseFrequency;
    bool noteActive;
    
    // Calculate the FM synthesis output
    double calculateFMSample();
};

// Wavetable synthesizer architecture
// Uses pre-defined waveforms stored in tables
class WavetableSynth : public SynthArchitecture {
public:
    WavetableSynth();
    virtual ~WavetableSynth() = default;
    
    virtual void noteOn(double frequency, double velocity = 1.0) override;
    virtual void noteOff() override;
    virtual double getNextSample() override;
    virtual void setSampleRate(int rate) override;
    
    // Add a waveform to the wavetable
    void addWaveform(const std::string& name, const std::vector<double>& waveform);
    
    // Set active waveform
    void setActiveWaveform(const std::string& name);
    
    // Set interpolation parameters
    void setInterpolationEnabled(bool enabled);
    
    // Set ADSR parameters
    void setADSR(double attack, double decay, double sustain, double release);

private:
    std::map<std::string, std::vector<double>> wavetables;
    std::string activeWaveform;
    std::vector<double> currentWaveform;
    
    double phase;
    double phaseIncrement;
    bool interpolate;
    
    ADSR ampEnvelope;
    bool noteActive;
    
    // Retrieve sample from wavetable with interpolation
    double getWavetableSample();
};

#endif // SYNTH_ARCHITECTURES_H