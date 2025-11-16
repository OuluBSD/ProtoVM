#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#include "Oscillator.h"
#include "AnalogSynthADSR.h"
#include "Portamento.h"
#include "Sequencer.h"
#include "LFO.h"
#include "Filter.h"
#include "ModulationMatrix.h"

class Synthesizer {
public:
    Synthesizer();

    void setWaveform(Waveform wf);
    void setADSRParams(double attack, double decay, double sustain, double release);
    void setPortamentoTime(double time);
    void setPortamentoEnabled(bool enabled);

    // LFO control methods
    void setLFO1Rate(double rate);
    void setLFO1Depth(double depth);
    void setLFO1Waveform(Waveform wf);
    void setLFO2Rate(double rate);
    void setLFO2Depth(double depth);
    void setLFO2Waveform(Waveform wf);

    // Filter control methods
    void setFilterCutoff(double cutoff);
    void setFilterResonance(double resonance);
    void setFilterType(int type);

    // Modulation matrix methods
    void addModulationConnection(ModulationSource source, ModulationDestination destination, double amount);
    void removeModulationConnection(ModulationSource source, ModulationDestination destination);
    void clearAllModulationConnections();

    void noteOn(double frequency, double velocity = 1.0);
    void noteOff();

    double getNextSample();

    void setSampleRate(int rate);

    // Preset methods
    PresetData getCurrentPreset(const std::string& name, const std::string& description = "") const;
    void loadPreset(const PresetData& preset);

private:
    Oscillator oscillator;
    ADSR envelope;
    Portamento portamento;
    Sequencer sequencer;
    LFO lfo1;
    LFO lfo2;
    Filter filter;
    ModulationMatrix modulationMatrix;

    int sampleRate;
    bool noteActive;
    double currentNote;
    double currentVelocity;
};

#endif // SYNTHESIZER_H