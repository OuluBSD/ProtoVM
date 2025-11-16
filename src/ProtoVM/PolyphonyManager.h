#ifndef _ProtoVM_PolyphonyManager_h_
#define _ProtoVM_PolyphonyManager_h_

#include "AnalogCommon.h"
#include "AudioSignalPath.h"
#include <vector>

// Structure to represent a single voice in the polyphonic synthesizer
struct Voice {
    int note_number;            // MIDI note number (0-127)
    double frequency;           // Frequency of the note
    bool active;                // Whether this voice is currently active
    double velocity;            // Note velocity (0.0-1.0)
    double age;                 // How long the note has been playing
    AudioSignalPath* path;      // Signal path for this voice
    
    Voice() : note_number(-1), frequency(0.0), active(false), velocity(0.0), age(0.0), path(nullptr) {}
    ~Voice() { if (path) delete path; }
    
    // Helper to calculate frequency from MIDI note number
    static double NoteToFrequency(int note) {
        return 440.0 * pow(2.0, (note - 69) / 12.0);
    }
    
    // Start a new note
    void StartNote(int note, double vel) {
        note_number = note;
        frequency = NoteToFrequency(note);
        active = true;
        velocity = vel;
        age = 0.0;
    }
    
    // Stop a note
    void StopNote() {
        active = false;
    }
};

class PolyphonyManager : public AnalogNodeBase {
public:
    typedef PolyphonyManager CLASSNAME;

    PolyphonyManager(int max_voices = 16);
    virtual ~PolyphonyManager();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "PolyphonyManager"; }

    // Note handling
    void NoteOn(int note_number, double velocity = 1.0);
    void NoteOff(int note_number);
    void AllNotesOff();
    
    // Get the number of currently active voices
    int GetActiveVoicesCount() const;
    
    // Get the maximum number of voices
    int GetMaxVoices() const { return max_voices; }
    
    // Set the maximum number of voices
    void SetMaxVoices(int max_voices);
    
    // Get the polyphonic output
    double GetOutput() const { return polyphonic_output; }
    
    // Get voice by index (for direct control)
    Voice* GetVoice(int index);
    
    // Set voice allocation mode (polyphonic, monophonic, legato, etc.)
    enum VoiceAllocationMode {
        POLYPHONIC,     // Multiple notes simultaneously
        MONOPHONIC,     // Single note at a time (latest or highest priority)
        LEGATO,         // Smooth transitions between notes
        MULTI_TIMBRAL   // Different timbres on different channels
    };
    
    void SetVoiceAllocationMode(VoiceAllocationMode mode);
    VoiceAllocationMode GetVoiceAllocationMode() const { return allocation_mode; }
    
    // Set voice stealing mode (when max voices exceeded)
    enum VoiceStealingMode {
        OLDEST_FIRST,   // Steal the oldest playing note
        QUIETEST_FIRST, // Steal the quietest note
        LAST_PLAYED     // Steal the most recently played note
    };
    
    void SetVoiceStealingMode(VoiceStealingMode mode);
    VoiceStealingMode GetVoiceStealingMode() const { return stealing_mode; }

private:
    std::vector<Voice*> voices;
    int max_voices;
    double polyphonic_output;
    
    VoiceAllocationMode allocation_mode;
    VoiceStealingMode stealing_mode;
    
    // Internal methods
    int FindFreeVoice();
    int FindVoiceByNote(int note_number);
    void ProcessVoice(Voice* voice);
    void HandleVoiceStealing();
    void ProcessMonophonicMode();
    
    // For monophonic modes
    std::vector<int> active_notes;  // For tracking what notes are currently pressed
};

#endif