#include "MidiInput.h"

MidiInput::MidiInput(PolyphonyManager* synth_engine, SynthUI* ui)
    : synth_engine(synth_engine)
    , ui(ui)
    , channel_filter(-1)  // Respond to all channels by default
    , pitch_bend_value(8192)  // Center value for pitch bend (0-16383)
{
    // Initialize controller values to default positions
    controller_values.resize(128, 0);
    for (int i = 0; i < 128; i++) {
        if (i == VOLUME) controller_values[i] = 100;  // Default volume to 100/127
        else if (i == PAN) controller_values[i] = 64;  // Center pan
        else if (i == EXPRESSION) controller_values[i] = 127;  // Full expression
    }
    
    // Initialize active notes array
    active_notes.resize(128, false);
}

MidiInput::~MidiInput() {
    // Cleanup handled by stack variables
}

bool MidiInput::Tick() {
    // Process all MIDI messages in the queue
    ProcessMidiQueue();
    
    // Apply continuous updates based on controller and pitch bend values
    ApplyModulationToSynth();
    
    return true;
}

void MidiInput::ProcessMidiMessage(const MidiMessage& msg) {
    // Check if message is for our filtered channel
    if (channel_filter >= 0 && channel_filter != msg.channel) {
        return; // Ignore messages not on filtered channel
    }
    
    switch (msg.type) {
        case MidiMessageType::NOTE_ON:
            HandleNoteOn(msg.channel, msg.data1, msg.data2);
            break;
            
        case MidiMessageType::NOTE_OFF:
            HandleNoteOff(msg.channel, msg.data1, msg.data2);
            break;
            
        case MidiMessageType::CONTROL_CHANGE:
            HandleControlChange(msg.channel, msg.data1, msg.data2);
            break;
            
        case MidiMessageType::PITCH_BEND:
            HandlePitchBend(msg.channel, msg.data1, msg.data2);
            break;
            
        case MidiMessageType::PROGRAM_CHANGE:
            HandleProgramChange(msg.channel, msg.data1);
            break;
            
        default:
            // Handle other MIDI message types if needed
            break;
    }
}

void MidiInput::AddMidiMessage(const MidiMessage& msg) {
    message_queue.push(msg);
}

void MidiInput::ProcessMidiQueue() {
    // Process all messages in the queue
    while (!message_queue.empty()) {
        MidiMessage msg = message_queue.front();
        message_queue.pop();
        
        ProcessMidiMessage(msg);
    }
}

void MidiInput::SetSynthEngine(PolyphonyManager* engine) {
    synth_engine = engine;
}

void MidiInput::SetUI(SynthUI* ui_control) {
    ui = ui_control;
}

void MidiInput::HandleNoteOn(int channel, int note, int velocity) {
    // Convert to 0.0-1.0 velocity range
    double normalized_velocity = static_cast<double>(velocity) / 127.0;
    
    if (synth_engine) {
        synth_engine->NoteOn(note, normalized_velocity);
    }
    
    // Mark note as active
    if (note >= 0 && note < 128) {
        active_notes[note] = true;
    }
}

void MidiInput::HandleNoteOff(int channel, int note, int velocity) {
    if (synth_engine) {
        synth_engine->NoteOff(note);
    }
    
    // Mark note as inactive
    if (note >= 0 && note < 128) {
        active_notes[note] = false;
    }
}

void MidiInput::HandleControlChange(int channel, int controller, int value) {
    // Update controller value
    if (controller >= 0 && controller < 128) {
        controller_values[controller] = value;
    }
    
    // Map common controllers to synthesizer parameters
    if (ui) {
        switch (controller) {
            case MODULATION_WHEEL:
                // Map to vibrato depth or filter modulation
                ui->SetParameterByName("Vibrato Depth", static_cast<double>(value) / 127.0);
                ui->SetParameterByName("Filter Mod Wheel", static_cast<double>(value) / 127.0 * 0.5);
                break;
                
            case VOLUME:
                // Map to overall volume
                ui->SetParameterByName("Amplifier Level", static_cast<double>(value) / 127.0 * 0.8);
                break;
                
            case FILTER_CUTOFF:
                // Map to filter cutoff
                ui->SetParameterByName("Filter Cutoff", 20.0 + (20000.0 - 20.0) * (static_cast<double>(value) / 127.0));
                break;
                
            case SUSTAIN_PEDAL:
                // Could be used for sustaining notes beyond note-off
                break;
                
            case PORTAMENTO:
                // Map to portamento time
                ui->SetParameterByName("Portamento Time", static_cast<double>(value) / 127.0);
                break;
                
            case REVERB_AMOUNT:
                // Map to reverb amount in an effects processor
                if (ui->GetParameterByName("Reverb Amount") != -1) {
                    ui->SetParameterByName("Reverb Amount", static_cast<double>(value) / 127.0);
                }
                break;
                
            case CHORUS_DEPTH:
                // Map to chorus depth in an effects processor
                if (ui->GetParameterByName("Chorus Depth") != -1) {
                    ui->SetParameterByName("Chorus Depth", static_cast<double>(value) / 127.0);
                }
                break;
                
            default:
                // Other controllers could be mapped to specific parameters
                break;
        }
    }
}

void MidiInput::HandlePitchBend(int channel, int lsb, int msb) {
    // Combine LSB and MSB to get full 14-bit value
    pitch_bend_value = (msb << 7) | lsb;  // Range 0-16383, center at 8192
    
    // Update synth if needed
    UpdatePitchBend();
}

void MidiInput::HandleProgramChange(int channel, int program) {
    // Could implement preset changing here
    // For now, just ignore program changes
}

void MidiInput::UpdatePitchBend() {
    if (!synth_engine || !ui) return;
    
    // Convert pitch bend value (0-16383) to semitones (-2 to +2 typically)
    double normalized_bend = static_cast<double>(pitch_bend_value - 8192) / 8192.0; // -1 to 1
    double semitones_bend = normalized_bend * 2.0; // +/- 2 semitones
    
    // Apply pitch bend to currently playing voices
    // This would require updating the VCO frequency for each active voice
    for (int i = 0; i < synth_engine->GetMaxVoices(); i++) {
        Voice* voice = synth_engine->GetVoice(i);
        if (voice && voice->active) {
            // Calculate bent frequency
            double base_freq = voice->frequency;
            double bent_freq = base_freq * pow(2.0, semitones_bend / 12.0);
            
            // In a real implementation, we'd update the VCO in this voice's signal path
            // For now, we'll just use the UI to represent the frequency change
            ui->SetParameterByName("VCO1 Frequency", bent_freq);
        }
    }
}

void MidiInput::UpdateControllers() {
    // Apply all controller values to the synth parameters
    // This method could be called periodically to sync controller changes
}

void MidiInput::ApplyModulationToSynth() {
    // Apply continuous modulation from controllers and pitch bend
    // This is called every tick to ensure smooth updates
    
    UpdatePitchBend();
    
    // Apply modulation wheel effect (CC1)
    double mod_wheel_value = static_cast<double>(controller_values[MODULATION_WHEEL]) / 127.0;
    
    if (ui) {
        // Apply to vibrato depth and filter modulation
        ui->SetParameterByName("Vibrato Depth", mod_wheel_value * 0.5);
        
        // Apply to filter modulation amount
        double filter_mod = mod_wheel_value * 0.3; // 30% of full modulation
        // This would update the filter cutoff modulation in a real implementation
    }
    
    // Apply other continuous controllers as needed
    // Pan, volume, expression, etc. could be continuously updated here
}