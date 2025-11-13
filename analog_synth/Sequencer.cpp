#include "Sequencer.h"
#include <cmath>

Sequencer::Sequencer() : 
    bpm(120), 
    numNotes(8), 
    minOctave(3), 
    maxOctave(6),
    running(false),
    currentNote(0),
    rng(std::random_device{}()),
    sampleCounter(0) {
    
    // Calculate samples per beat based on BPM
    double beatsPerSecond = bpm / 60.0;
    samplesPerBeat = static_cast<int>(sampleRate / beatsPerSecond);
    
    // Set up random distribution for notes
    int minNote = minOctave * 12 + 12;  // C in the min octave
    int maxNote = maxOctave * 12 + 11; // B in the max octave
    noteDist = std::uniform_int_distribution<int>(minNote, maxNote);
    
    // Generate initial random notes
    updateNotes();
}

void Sequencer::setBPM(int newBPM) {
    bpm = newBPM;
    double beatsPerSecond = bpm / 60.0;
    samplesPerBeat = static_cast<int>(sampleRate / beatsPerSecond);
}

void Sequencer::setNumNotes(int notes) {
    numNotes = notes;
    updateNotes();
}

void Sequencer::setOctaveRange(int min, int max) {
    minOctave = min;
    maxOctave = max;
    int minNote = minOctave * 12 + 12;
    int maxNote = maxOctave * 12 + 11;
    noteDist = std::uniform_int_distribution<int>(minNote, maxNote);
    updateNotes();
}

void Sequencer::start() {
    running = true;
    sampleCounter = 0;
    currentNote = 0;
}

void Sequencer::stop() {
    running = false;
}

double Sequencer::getNextNote() {
    if (!running) {
        return 0.0; // Silent when not running
    }
    
    // Check if it's time for the next note
    sampleCounter++;
    if (sampleCounter >= samplesPerBeat) {
        currentNote = (currentNote + 1) % numNotes;
        sampleCounter = 0;
    }
    
    return noteFrequencies[currentNote];
}

double Sequencer::midiToFreq(int note) {
    // Convert MIDI note number to frequency
    // A4 (MIDI note 69) = 440 Hz
    return 440.0 * pow(2.0, (note - 69) / 12.0);
}

void Sequencer::updateNotes() {
    noteFrequencies.resize(numNotes);
    int minNote = minOctave * 12 + 12;
    int maxNote = maxOctave * 12 + 11;
    
    for (int i = 0; i < numNotes; i++) {
        int note = noteDist(rng);
        noteFrequencies[i] = midiToFreq(note);
    }
}