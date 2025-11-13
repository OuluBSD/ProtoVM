#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <vector>
#include <random>

class Sequencer {
public:
    Sequencer();
    
    void setBPM(int bpm);
    void setNumNotes(int notes);
    void setOctaveRange(int min, int max);
    
    void start();
    void stop();
    
    bool isRunning() const { return running; }
    
    // Get the next note frequency
    double getNextNote();
    
    // Convert MIDI note number to frequency
    static double midiToFreq(int note);
    
private:
    void updateNotes();
    int bpm;
    int numNotes;
    int minOctave;
    int maxOctave;
    bool running;
    
    std::vector<double> noteFrequencies;
    int currentNote;
    
    std::mt19937 rng;
    std::uniform_int_distribution<int> noteDist;
    
    int samplesPerBeat;
    int sampleCounter;
    
    static const int sampleRate = 44100;
};

#endif // SEQUENCER_H