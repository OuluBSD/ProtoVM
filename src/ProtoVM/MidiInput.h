#ifndef _ProtoVM_MidiInput_h_
#define _ProtoVM_MidiInput_h_

#include "AnalogCommon.h"
#include "PolyphonyManager.h"
#include "SynthUI.h"
#include <vector>
#include <queue>
#include <functional>

// MIDI message types
enum class MidiMessageType {
    NOTE_OFF = 0x80,
    NOTE_ON = 0x90,
    POLYPHONIC_KEY_PRESSURE = 0xA0,
    CONTROL_CHANGE = 0xB0,
    PROGRAM_CHANGE = 0xC0,
    CHANNEL_PRESSURE = 0xD0,
    PITCH_BEND = 0xE0,
    SYSTEM_MESSAGE = 0xF0
};

// Structure for MIDI messages
struct MidiMessage {
    MidiMessageType type;
    int channel;      // 0-15
    int data1;        // First data byte
    int data2;        // Second data byte (if applicable)
    double timestamp; // Time of message
    
    MidiMessage(MidiMessageType t = MidiMessageType::NOTE_ON, int ch = 0, int d1 = 0, int d2 = 0, double ts = 0.0)
        : type(t), channel(ch), data1(d1), data2(d2), timestamp(ts) {}
};

class MidiInput : public AnalogNodeBase {
public:
    typedef MidiInput CLASSNAME;

    MidiInput(PolyphonyManager* synth_engine = nullptr, SynthUI* ui = nullptr);
    virtual ~MidiInput();

    virtual bool Tick() override;
    virtual String GetClassName() const override { return "MidiInput"; }

    // Process incoming MIDI messages
    void ProcessMidiMessage(const MidiMessage& msg);
    
    // Add a MIDI message to the queue
    void AddMidiMessage(const MidiMessage& msg);
    
    // Process all messages in the queue
    void ProcessMidiQueue();
    
    // Set the synth engine to control
    void SetSynthEngine(PolyphonyManager* engine);
    PolyphonyManager* GetSynthEngine() const { return synth_engine; }
    
    // Set the UI to control
    void SetUI(SynthUI* ui);
    SynthUI* GetUI() const { return ui; }
    
    // Handle specific MIDI messages
    void HandleNoteOn(int channel, int note, int velocity);
    void HandleNoteOff(int channel, int note, int velocity);
    void HandleControlChange(int channel, int controller, int value);
    void HandlePitchBend(int channel, int lsb, int msb);
    void HandleProgramChange(int channel, int program);
    
    // Get number of messages in queue
    int GetMessageQueueSize() const { return static_cast<int>(message_queue.size()); }
    
    // Set MIDI channel filter (0-15, or -1 for all channels)
    void SetChannelFilter(int channel) { channel_filter = channel; }
    int GetChannelFilter() const { return channel_filter; }
    
    // MIDI controller numbers
    enum MidiControllers {
        MODULATION_WHEEL = 1,
        VOLUME = 7,
        PAN = 10,
        EXPRESSION = 11,
        SUSTAIN_PEDAL = 64,
        PORTAMENTO = 65,
        FILTER_CUTOFF = 74,  // CC74 is often used for filter cutoff
        REVERB_AMOUNT = 91,
        TREMOLO_DEPTH = 92,
        CHORUS_DEPTH = 93,
        VIBRATO_RATE = 94,
        EFFECTS_LEVEL = 95,
        RIBBON_CONTROLLER = 96  // Often used as general-purpose controller
    };

private:
    PolyphonyManager* synth_engine;
    SynthUI* ui;
    std::queue<MidiMessage> message_queue;
    int channel_filter;  // MIDI channel to respond to (-1 for all)
    
    // Values for continuous controllers
    std::vector<int> controller_values;  // For each controller (0-127)
    
    // Pitch bend value (0-16383, center at 8192)
    int pitch_bend_value;
    
    // Currently pressed notes
    std::vector<bool> active_notes;  // For each MIDI note (0-127)
    
    // Internal processing methods
    void UpdatePitchBend();
    void UpdateControllers();
    void ApplyModulationToSynth();
};

#endif