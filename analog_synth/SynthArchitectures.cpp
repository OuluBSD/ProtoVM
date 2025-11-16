#include "SynthArchitectures.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Subtractive Synth Implementation
SubtractiveSynth::SubtractiveSynth() : 
    baseFrequency(440.0),  // A4
    noteActive(false),
    lfoDestination(0),     // Default: modulate oscillator
    lfoDepth(0.0) {
    // Start with one oscillator
    oscillators.emplace_back();
    oscillators[0].setWaveform(Waveform::SAWTOOTH);
}

void SubtractiveSynth::noteOn(double frequency, double velocity) {
    baseFrequency = frequency;
    updateOscillatorFrequencies(frequency);
    
    filterEnvelope.noteOn();
    ampEnvelope.noteOn();
    
    noteActive = true;
}

void SubtractiveSynth::noteOff() {
    filterEnvelope.noteOff();
    ampEnvelope.noteOff();
    
    noteActive = false;
}

void SubtractiveSynth::updateOscillatorFrequencies(double baseFreq) {
    for (size_t i = 0; i < oscillators.size(); ++i) {
        // Apply detuning in cents: f * 2^(cents/1200)
        double detuneFactor = std::pow(2.0, 0.0 / 1200.0); // For now, no detuning
        oscillators[i].setFrequency(baseFreq * detuneFactor);
    }
}

double SubtractiveSynth::getNextSample() {
    // Get LFO value for modulation
    double lfoValue = lfo1.getNextSample();
    
    // Calculate modulation amount
    double modulation = lfoDepth * lfoValue;
    
    // Mix oscillator outputs
    double mixedOutput = 0.0;
    for (auto& osc : oscillators) {
        double oscSample = osc.getNextSample();
        
        // Apply oscillator modulation if selected
        if (lfoDestination == 0) { // Oscillator modulation
            double modulatedFreq = osc.getFrequency() * (1.0 + modulation);
            osc.setFrequency(modulatedFreq);
            oscSample = osc.getNextSample();
        }
        
        mixedOutput += oscSample;
    }
    
    // Average if we have multiple oscillators
    if (oscillators.size() > 1) {
        mixedOutput /= oscillators.size();
    }
    
    // Apply filter modulation if selected
    if (lfoDestination == 1) { // Filter modulation
        double baseCutoff = 0.5; // Should get from filter
        double modulatedCutoff = baseCutoff + (modulation * 0.5); // Limit modulation to 50%
        // Ensure cutoff is within valid range
        modulatedCutoff = std::max(0.0, std::min(1.0, modulatedCutoff));
        filter.setCutoff(modulatedCutoff);
    }
    
    // Apply filter to the mixed oscillator output
    double filteredOutput = filter.processSample(mixedOutput);
    
    // Apply amplitude modulation if selected
    if (lfoDestination == 2) { // Amplitude modulation
        double ampModulation = (modulation + 1.0) / 2.0; // Convert -1,1 to 0,1 range
        filteredOutput *= ampModulation;
    }
    
    // Get envelope values
    double filterEnvValue = filterEnvelope.getNextSample();
    double ampEnvValue = ampEnvelope.getNextSample();
    
    // Apply filter envelope to cutoff
    double baseCutoff = 0.5; // Should be stored parameter
    double modulatedCutoff = baseCutoff * (1.0 + (filterEnvValue - 1.0) * 0.8); // Apply most of the envelope
    filter.setCutoff(std::max(0.01, std::min(0.99, modulatedCutoff)));
    
    // Apply amplitude envelope
    double finalOutput = filteredOutput * ampEnvValue;
    
    // Apply master volume
    finalOutput *= volume;
    
    return finalOutput;
}

void SubtractiveSynth::setSampleRate(int rate) {
    sampleRate = rate;
    filter.setSampleRate(rate);
}

void SubtractiveSynth::setOscCount(int count) {
    if (count < 1) count = 1;
    if (count > 8) count = 8; // Maximum 8 oscillators
    
    oscillators.resize(count);
    
    // Set default parameters for new oscillators
    for (size_t i = oscillators.size(); i < static_cast<size_t>(count); ++i) {
        oscillators[i].setWaveform(Waveform::SAWTOOTH);
    }
    
    // Update frequencies if a note is active
    if (noteActive) {
        updateOscillatorFrequencies(baseFrequency);
    }
}

void SubtractiveSynth::setOscWaveform(int oscIndex, Waveform wf) {
    if (oscIndex >= 0 && static_cast<size_t>(oscIndex) < oscillators.size()) {
        oscillators[oscIndex].setWaveform(wf);
    }
}

void SubtractiveSynth::setOscDetune(int oscIndex, double detune) {
    // Implementation would update oscillator frequencies based on detune
    // For now, we'll just store the detune value
    if (noteActive) {
        updateOscillatorFrequencies(baseFrequency);
    }
}

void SubtractiveSynth::setFilterCutoff(double cutoff) {
    filter.setCutoff(cutoff);
}

void SubtractiveSynth::setFilterResonance(double resonance) {
    filter.setResonance(resonance);
}

void SubtractiveSynth::setFilterType(int type) {
    filter.setType(type);
}

void SubtractiveSynth::setFilterADSR(double attack, double decay, double sustain, double release) {
    filterEnvelope.setAttack(attack);
    filterEnvelope.setDecay(decay);
    filterEnvelope.setSustain(sustain);
    filterEnvelope.setRelease(release);
}

void SubtractiveSynth::setAmpADSR(double attack, double decay, double sustain, double release) {
    ampEnvelope.setAttack(attack);
    ampEnvelope.setDecay(decay);
    ampEnvelope.setSustain(sustain);
    ampEnvelope.setRelease(release);
}

void SubtractiveSynth::setLFO1Rate(double rate) {
    lfo1.setRate(rate);
}

void SubtractiveSynth::setLFO1Depth(double depth) {
    lfoDepth = depth;
}

void SubtractiveSynth::setLFO1Destination(int dest) {
    lfoDestination = dest;
}


// FM Synth Implementation
FMSynth::FMSynth() : 
    modulationIndex(1.0),
    baseFrequency(440.0),
    noteActive(false) {
    // Set default waveforms
    carrier.setWaveform(Waveform::SINE);
    modulator.setWaveform(Waveform::SINE);
}

void FMSynth::noteOn(double frequency, double velocity) {
    baseFrequency = frequency;
    carrier.setFrequency(frequency);
    modulator.setFrequency(frequency); // For now, same frequency
    
    carrierEnvelope.noteOn();
    modulatorEnvelope.noteOn();
    
    noteActive = true;
}

void FMSynth::noteOff() {
    carrierEnvelope.noteOff();
    modulatorEnvelope.noteOff();
    
    noteActive = false;
}

double FMSynth::calculateFMSample() {
    // Get modulator sample
    double modulatorSample = modulator.getNextSample();
    
    // Use modulator to modulate carrier frequency
    double modulatedFreq = baseFrequency * (1.0 + modulationIndex * modulatorSample);
    carrier.setFrequency(modulatedFreq);
    
    // Get carrier sample
    double carrierSample = carrier.getNextSample();
    
    return carrierSample;
}

double FMSynth::getNextSample() {
    // Calculate modulator envelope value
    double modEnvValue = modulatorEnvelope.getNextSample();
    
    // Apply modulation to the modulation index
    double currentModIndex = modulationIndex * modEnvValue;
    
    // Get modulator sample
    double modulatorSample = modulator.getNextSample();
    
    // Use modulator to modulate carrier frequency
    double modulatedFreq = baseFrequency * (1.0 + currentModIndex * modulatorSample);
    carrier.setFrequency(modulatedFreq);
    
    // Get carrier sample
    double carrierSample = carrier.getNextSample();
    
    // Apply amplitude envelope
    double ampEnvValue = carrierEnvelope.getNextSample();
    double finalOutput = carrierSample * ampEnvValue;
    
    // Apply master volume
    finalOutput *= volume;
    
    return finalOutput;
}

void FMSynth::setSampleRate(int rate) {
    sampleRate = rate;
}

void FMSynth::setCarrierWaveform(Waveform wf) {
    carrier.setWaveform(wf);
}

void FMSynth::setModulatorWaveform(Waveform wf) {
    modulator.setWaveform(wf);
}

void FMSynth::setCarrierFrequency(double freq) {
    carrier.setFrequency(freq);
}

void FMSynth::setModulatorFrequency(double freq) {
    modulator.setFrequency(freq);
}

void FMSynth::setModulationIndex(double index) {
    modulationIndex = index;
}

void FMSynth::setCarrierADSR(double attack, double decay, double sustain, double release) {
    carrierEnvelope.setAttack(attack);
    carrierEnvelope.setDecay(decay);
    carrierEnvelope.setSustain(sustain);
    carrierEnvelope.setRelease(release);
}

void FMSynth::setModulatorADSR(double attack, double decay, double sustain, double release) {
    modulatorEnvelope.setAttack(attack);
    modulatorEnvelope.setDecay(decay);
    modulatorEnvelope.setSustain(sustain);
    modulatorEnvelope.setRelease(release);
}


// Wavetable Synth Implementation
WavetableSynth::WavetableSynth() : 
    phase(0.0),
    phaseIncrement(0.0),
    interpolate(true),
    noteActive(false) {
    // Initialize with basic waveforms
    addWaveform("sine", []() {
        std::vector<double> wave(256);
        for (int i = 0; i < 256; ++i) {
            wave[i] = std::sin(2.0 * M_PI * i / 256.0);
        }
        return wave;
    }());
    
    addWaveform("sawtooth", []() {
        std::vector<double> wave(256);
        for (int i = 0; i < 256; ++i) {
            wave[i] = 2.0 * (i / 256.0) - 1.0;
        }
        return wave;
    }());
    
    addWaveform("square", []() {
        std::vector<double> wave(256);
        for (int i = 0; i < 256; ++i) {
            wave[i] = i < 128 ? 1.0 : -1.0;
        }
        return wave;
    }());
    
    addWaveform("triangle", []() {
        std::vector<double> wave(256);
        for (int i = 0; i < 256; ++i) {
            if (i < 64) {
                wave[i] = i / 64.0;
            } else if (i < 192) {
                wave[i] = 1.0 - ((i - 64) / 128.0) * 2.0;
            } else {
                wave[i] = -1.0 + ((i - 192) / 64.0) * 1.0;
            }
        }
        return wave;
    }());
    
    activeWaveform = "sine";
    currentWaveform = wavetables[activeWaveform];
}

void WavetableSynth::addWaveform(const std::string& name, const std::vector<double>& waveform) {
    wavetables[name] = waveform;
}

void WavetableSynth::setActiveWaveform(const std::string& name) {
    auto it = wavetables.find(name);
    if (it != wavetables.end()) {
        activeWaveform = name;
        currentWaveform = it->second;
    }
}

void WavetableSynth::setInterpolationEnabled(bool enabled) {
    interpolate = enabled;
}

void WavetableSynth::setADSR(double attack, double decay, double sustain, double release) {
    ampEnvelope.setAttack(attack);
    ampEnvelope.setDecay(decay);
    ampEnvelope.setSustain(sustain);
    ampEnvelope.setRelease(release);
}

double WavetableSynth::getWavetableSample() {
    if (currentWaveform.empty()) {
        return 0.0;
    }
    
    int tableSize = currentWaveform.size();
    
    if (interpolate) {
        // Linear interpolation
        double index = phase * tableSize;
        int index1 = static_cast<int>(index) % tableSize;
        int index2 = (index1 + 1) % tableSize;
        double fraction = index - static_cast<double>(index1);
        
        double sample1 = currentWaveform[index1];
        double sample2 = currentWaveform[index2];
        
        return sample1 + fraction * (sample2 - sample1);
    } else {
        // No interpolation
        int index = static_cast<int>(phase * tableSize) % tableSize;
        return currentWaveform[index];
    }
}

double WavetableSynth::getNextSample() {
    // Get sample from wavetable
    double sample = getWavetableSample();
    
    // Advance phase
    phase += phaseIncrement;
    if (phase >= 1.0) {
        phase -= 1.0;
    }
    
    // Apply amplitude envelope
    double ampEnvValue = ampEnvelope.getNextSample();
    double finalOutput = sample * ampEnvValue;
    
    // Apply master volume
    finalOutput *= volume;
    
    return finalOutput;
}

void WavetableSynth::noteOn(double frequency, double velocity) {
    baseFrequency = frequency;
    // Calculate phase increment based on frequency and sample rate
    phaseIncrement = frequency / sampleRate;
    
    ampEnvelope.noteOn();
    noteActive = true;
}

void WavetableSynth::noteOff() {
    ampEnvelope.noteOff();
    noteActive = false;
}

void WavetableSynth::setSampleRate(int rate) {
    sampleRate = rate;
    // Update phase increment if a note is active
    if (noteActive) {
        phaseIncrement = baseFrequency / sampleRate;
    }
}