#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

#include "Oscillator.h"
#include "AnalogSynthADSR.h"
#include "Portamento.h"
#include "Sequencer.h"

class Synthesizer {
public:
    Synthesizer();
    
    void setWaveform(Waveform wf);
    void setADSRParams(double attack, double decay, double sustain, double release);
    void setPortamentoTime(double time);
    void setPortamentoEnabled(bool enabled);
    
    void noteOn(double frequency);
    void noteOff();
    
    double getNextSample();
    
    void setSampleRate(int rate);
    
private:
    Oscillator oscillator;
    ADSR envelope;
    Portamento portamento;
    Sequencer sequencer;
    
    int sampleRate;
    bool noteActive;
};

#endif // SYNTHESIZER_H