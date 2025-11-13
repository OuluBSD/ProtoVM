#include "Synthesizer.h"

Synthesizer::Synthesizer() : 
    sampleRate(44100), 
    noteActive(false) {}

void Synthesizer::setWaveform(Waveform wf) {
    oscillator.setWaveform(wf);
}

void Synthesizer::setADSRParams(double attack, double decay, double sustain, double release) {
    envelope.setAttack(attack);
    envelope.setDecay(decay);
    envelope.setSustain(sustain);
    envelope.setRelease(release);
}

void Synthesizer::setPortamentoTime(double time) {
    portamento.setTime(time);
}

void Synthesizer::setPortamentoEnabled(bool enabled) {
    portamento.setEnabled(enabled);
}

void Synthesizer::noteOn(double frequency) {
    if (noteActive) {
        // If a note is already playing, use portamento to glide to new note
        portamento.setTargetFrequency(frequency);
    } else {
        // If no note is playing, start immediately
        portamento.setCurrentFrequency(frequency);
        portamento.setTargetFrequency(frequency);
        envelope.noteOn();
        noteActive = true;
    }
}

void Synthesizer::noteOff() {
    envelope.noteOff();
    noteActive = false;
}

double Synthesizer::getNextSample() {
    // Get frequency from portamento (handles gliding between notes)
    double freq = portamento.getNextFrequency();
    oscillator.setFrequency(freq);
    
    // Get next sample from oscillator
    double oscSample = oscillator.getNextSample();
    
    // Apply envelope to the sample
    double envValue = envelope.getNextSample();
    double output = oscSample * envValue;
    
    return output;
}

void Synthesizer::setSampleRate(int rate) {
    sampleRate = rate;
}