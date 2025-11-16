#include "Synthesizer.h"
#include "ModulationMatrix.h"

Synthesizer::Synthesizer() :
    sampleRate(44100),
    noteActive(false),
    currentNote(60.0), // Default to middle C
    currentVelocity(1.0) {}

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

// LFO control methods
void Synthesizer::setLFO1Rate(double rate) {
    lfo1.setRate(rate);
}

void Synthesizer::setLFO1Depth(double depth) {
    lfo1.setDepth(depth);
}

void Synthesizer::setLFO1Waveform(Waveform wf) {
    lfo1.setWaveform(wf);
}

void Synthesizer::setLFO2Rate(double rate) {
    lfo2.setRate(rate);
}

void Synthesizer::setLFO2Depth(double depth) {
    lfo2.setDepth(depth);
}

void Synthesizer::setLFO2Waveform(Waveform wf) {
    lfo2.setWaveform(wf);
}

// Filter control methods
void Synthesizer::setFilterCutoff(double cutoff) {
    filter.setCutoff(cutoff);
}

void Synthesizer::setFilterResonance(double resonance) {
    filter.setResonance(resonance);
}

void Synthesizer::setFilterType(int type) {
    filter.setType(type);
}

// Modulation matrix methods
void Synthesizer::addModulationConnection(ModulationSource source, ModulationDestination destination, double amount) {
    modulationMatrix.addConnection(source, destination, amount);
}

void Synthesizer::removeModulationConnection(ModulationSource source, ModulationDestination destination) {
    modulationMatrix.removeConnection(source, destination);
}

void Synthesizer::clearAllModulationConnections() {
    modulationMatrix.clearAllConnections();
}

void Synthesizer::noteOn(double frequency, double velocity) {
    currentNote = frequency; // Store the note for modulation purposes
    currentVelocity = velocity;
    
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
    // Process LFOs to get their current values
    double lfo1Value = lfo1.getNextSample();
    double lfo2Value = lfo2.getNextSample();
    
    // Process envelope to get its current value
    double envValue = envelope.getNextSample();
    
    // Set current values in the modulation matrix
    modulationMatrix.setCurrentLFO1Value(lfo1Value);
    modulationMatrix.setCurrentLFO2Value(lfo2Value);
    modulationMatrix.setCurrentADSR1Value(envValue); // Using the main envelope as ADSR1
    modulationMatrix.setCurrentADSR2Value(0.0); // Placeholder for second envelope
    modulationMatrix.setCurrentVelocity(currentVelocity * 127.0); // Convert to MIDI velocity range
    modulationMatrix.setCurrentNote(currentNote);
    modulationMatrix.setCurrentMidiCC(0.0); // Placeholder - would come from MIDI CC input
    modulationMatrix.setCurrentEnvelopeFollower(0.0); // Placeholder for envelope follower
    modulationMatrix.setCurrentNoise(0.0); // Placeholder for noise source
    
    // Process all modulations
    modulationMatrix.processModulation();
    
    // Apply modulations to various parameters
    // Modulate oscillator frequency
    double baseFreq = portamento.getNextFrequency();
    double freqMod = modulationMatrix.getModulationValue(ModulationDestination::OSC_FREQUENCY);
    oscillator.setFrequency(baseFreq * (1.0 + freqMod));
    
    // Modulate waveform if applicable (this would be an advanced feature)
    // For now, we'll just let the waveform stay constant
    
    // Modulate filter cutoff
    double baseCutoff = 0.5; // Default cutoff
    double cutoffMod = modulationMatrix.getModulationValue(ModulationDestination::FILTER_CUTOFF);
    filter.setCutoff(baseCutoff + cutoffMod);
    
    // Modulate filter resonance
    double baseResonance = 0.5; // Default resonance
    double resMod = modulationMatrix.getModulationValue(ModulationDestination::FILTER_RESONANCE);
    filter.setResonance(baseResonance + resMod);
    
    // Get next sample from oscillator
    double oscSample = oscillator.getNextSample();
    
    // Apply filter
    double filteredSample = filter.processSample(oscSample);
    
    // Apply envelope
    double output = filteredSample * envValue;
    
    return output;
}

void Synthesizer::setSampleRate(int rate) {
    sampleRate = rate;
    filter.setSampleRate(rate);
}

PresetData Synthesizer::getCurrentPreset(const std::string& name, const std::string& description) const {
    PresetData preset;
    preset.name = name;
    preset.description = description;
    
    // Get oscillator settings
    preset.waveform = oscillator.getWaveform();
    
    // Get ADSR settings
    preset.attack = envelope.getAttack();
    preset.decay = envelope.getDecay();
    preset.sustain = envelope.getSustain();
    preset.release = envelope.getRelease();
    
    // Get portamento settings
    preset.portamentoTime = portamento.getTime();
    preset.portamentoEnabled = portamento.isEnabled();
    
    // Get LFO settings
    preset.lfo1Rate = lfo1.getRate();
    preset.lfo1Depth = lfo1.getDepth();
    preset.lfo1Waveform = lfo1.getWaveform();
    preset.lfo2Rate = lfo2.getRate();
    preset.lfo2Depth = lfo2.getDepth();
    preset.lfo2Waveform = lfo2.getWaveform();
    
    // Get filter settings
    preset.filterCutoff = filter.getCutoff();
    preset.filterResonance = filter.getResonance();
    preset.filterType = filter.getType();
    
    // Get modulation connections from the modulation matrix
    for (const auto& conn : modulationMatrix.getAllConnections()) {
        preset.modulationConnections.push_back(std::make_tuple(conn.source, conn.destination, conn.amount));
    }
    
    return preset;
}

void Synthesizer::loadPreset(const PresetData& preset) {
    // Apply oscillator settings
    setWaveform(preset.waveform);
    
    // Apply ADSR settings
    setADSRParams(preset.attack, preset.decay, preset.sustain, preset.release);
    
    // Apply portamento settings
    setPortamentoTime(preset.portamentoTime);
    setPortamentoEnabled(preset.portamentoEnabled);
    
    // Apply LFO settings
    setLFO1Rate(preset.lfo1Rate);
    setLFO1Depth(preset.lfo1Depth);
    setLFO1Waveform(preset.lfo1Waveform);
    setLFO2Rate(preset.lfo2Rate);
    setLFO2Depth(preset.lfo2Depth);
    setLFO2Waveform(preset.lfo2Waveform);
    
    // Apply filter settings
    setFilterCutoff(preset.filterCutoff);
    setFilterResonance(preset.filterResonance);
    setFilterType(preset.filterType);
    
    // Clear and rebuild modulation connections
    clearAllModulationConnections();
    for (const auto& conn : preset.modulationConnections) {
        addModulationConnection(std::get<0>(conn), std::get<1>(conn), std::get<2>(conn));
    }
}