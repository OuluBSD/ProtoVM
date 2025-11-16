#include "PolyphonyManager.h"
#include <algorithm>

PolyphonyManager::PolyphonyManager(int max_voices)
    : max_voices(max_voices)
    , polyphonic_output(0.0)
    , allocation_mode(POLYPHONIC)
    , stealing_mode(OLDEST_FIRST)
{
    // Initialize voices
    voices.resize(max_voices);
    for (int i = 0; i < max_voices; i++) {
        voices[i] = new Voice();
        voices[i]->path = new AudioSignalPath(SignalPathType::VINTAGE_MONO_SYNTH);
    }
}

PolyphonyManager::~PolyphonyManager() {
    // Clean up voices and their signal paths
    for (auto* voice : voices) {
        delete voice;
    }
    voices.clear();
}

bool PolyphonyManager::Tick() {
    polyphonic_output = 0.0;
    
    // Process based on allocation mode
    if (allocation_mode == MONOPHONIC || allocation_mode == LEGATO) {
        ProcessMonophonicMode();
        return true;
    }
    
    // Process each active voice
    for (auto* voice : voices) {
        if (voice->active) {
            // Update voice age
            voice->age += 1.0/44100.0; // Assuming 44.1kHz sample rate
            
            // Process the voice's signal path
            ProcessVoice(voice);
            
            // Add to polyphonic output
            polyphonic_output += voice->path->GetOutput() * voice->velocity;
        } else {
            // Reset age for inactive voices
            voice->age = 0.0;
        }
    }
    
    // Normalize output to prevent clipping with many voices
    polyphonic_output /= sqrt(static_cast<double>(max_voices));
    
    return true;
}

void PolyphonyManager::NoteOn(int note_number, double velocity) {
    // Handle based on allocation mode
    if (allocation_mode == MONOPHONIC || allocation_mode == LEGATO) {
        // For monophonic modes, add to active notes
        auto it = std::find(active_notes.begin(), active_notes.end(), note_number);
        if (it == active_notes.end()) {
            active_notes.push_back(note_number);
        }
        return;
    }
    
    // Find a free voice
    int free_voice_idx = FindFreeVoice();
    
    if (free_voice_idx >= 0) {
        // Found a free voice, use it
        voices[free_voice_idx]->StartNote(note_number, velocity);
        
        // Set the frequency in the VCO of this voice's signal path
        if (voices[free_voice_idx]->path) {
            AnalogNodeBase* vco = voices[free_voice_idx]->path->GetComponent(0); // VCO is first component
            if (vco) {
                // This would require casting to VCO to set the frequency
                // For now, we'll just ensure the voice knows its frequency
            }
        }
    } else {
        // No free voices, handle voice stealing
        HandleVoiceStealing();
        
        // Try again to find a free voice after stealing
        free_voice_idx = FindFreeVoice();
        if (free_voice_idx >= 0) {
            voices[free_voice_idx]->StartNote(note_number, velocity);
        }
    }
}

void PolyphonyManager::NoteOff(int note_number) {
    if (allocation_mode == MONOPHONIC || allocation_mode == LEGATO) {
        // For monophonic modes, remove from active notes
        auto it = std::find(active_notes.begin(), active_notes.end(), note_number);
        if (it != active_notes.end()) {
            active_notes.erase(it);
            
            // If there are still active notes, continue the top one
            if (!active_notes.empty()) {
                int top_note = allocation_mode == LEGATO ? 
                    active_notes.back() :  // For legato, use last played
                    *std::max_element(active_notes.begin(), active_notes.end());  // For mono, use highest
            
                // Find a voice playing the old note to retarget
                for (auto* voice : voices) {
                    if (voice->note_number == note_number && voice->active) {
                        voice->StartNote(top_note, voice->velocity);
                        break;
                    }
                }
            } else {
                // No more active notes, stop the voice
                for (auto* voice : voices) {
                    if (voice->note_number == note_number) {
                        voice->StopNote();
                    }
                }
            }
        }
        return;
    }
    
    // Find the voice playing this note
    int voice_idx = FindVoiceByNote(note_number);
    if (voice_idx >= 0) {
        // Trigger the release of this voice
        // This would involve triggering the ADSR's release phase
        if (voices[voice_idx]->path) {
            // Get the ADSR component and trigger note off
            AnalogNodeBase* adsr = voices[voice_idx]->path->GetComponent(4); // ADSR is 5th component
            if (adsr) {
                // This would require actual ADSR interface to trigger release
            }
        }
        
        // For now, just mark the voice as inactive
        voices[voice_idx]->StopNote();
    }
}

void PolyphonyManager::AllNotesOff() {
    for (auto* voice : voices) {
        voice->StopNote();
    }
    
    active_notes.clear();
}

int PolyphonyManager::GetActiveVoicesCount() const {
    int count = 0;
    for (const auto* voice : voices) {
        if (voice->active) {
            count++;
        }
    }
    return count;
}

void PolyphonyManager::SetMaxVoices(int max_voices) {
    if (max_voices == this->max_voices) {
        return; // No change
    }
    
    if (max_voices > this->max_voices) {
        // Growing the voice pool
        int old_size = this->max_voices;
        voices.resize(max_voices);
        for (int i = old_size; i < max_voices; i++) {
            voices[i] = new Voice();
            voices[i]->path = new AudioSignalPath(SignalPathType::VINTAGE_MONO_SYNTH);
        }
    } else {
        // Shrinking the voice pool - need to handle active voices
        // Stop any active voices that will be removed
        for (int i = max_voices; i < this->max_voices; i++) {
            if (voices[i] && voices[i]->active) {
                voices[i]->StopNote();
            }
        }
        // Remove the excess voices
        for (int i = max_voices; i < this->max_voices; i++) {
            delete voices[i];
            voices[i] = nullptr;
        }
        voices.resize(max_voices);
    }
    
    this->max_voices = max_voices;
}

Voice* PolyphonyManager::GetVoice(int index) {
    if (index >= 0 && index < static_cast<int>(voices.size())) {
        return voices[index];
    }
    return nullptr;
}

void PolyphonyManager::SetVoiceAllocationMode(VoiceAllocationMode mode) {
    this->allocation_mode = mode;
    
    // Clear active notes when changing modes
    active_notes.clear();
}

void PolyphonyManager::SetVoiceStealingMode(VoiceStealingMode mode) {
    this->stealing_mode = mode;
}

int PolyphonyManager::FindFreeVoice() {
    for (int i = 0; i < static_cast<int>(voices.size()); i++) {
        if (!voices[i]->active) {
            return i;
        }
    }
    return -1; // No free voices
}

int PolyphonyManager::FindVoiceByNote(int note_number) {
    for (int i = 0; i < static_cast<int>(voices.size()); i++) {
        if (voices[i]->active && voices[i]->note_number == note_number) {
            return i;
        }
    }
    return -1; // Note not found
}

void PolyphonyManager::ProcessVoice(Voice* voice) {
    if (!voice->path) return;
    
    // Update the VCO frequency in this voice's signal path
    AnalogNodeBase* vco = voice->path->GetComponent(0);
    if (vco) {
        // This would require casting to VCO to set frequency
        // For now, we'll just tick the signal path
    }
    
    // Process the signal path
    voice->path->Tick();
}

void PolyphonyManager::HandleVoiceStealing() {
    switch (stealing_mode) {
        case OLDEST_FIRST: {
            // Find the oldest active voice
            int oldest_idx = -1;
            double oldest_age = -1.0;
            for (int i = 0; i < static_cast<int>(voices.size()); i++) {
                if (voices[i]->active && voices[i]->age > oldest_age) {
                    oldest_age = voices[i]->age;
                    oldest_idx = i;
                }
            }
            
            if (oldest_idx >= 0) {
                voices[oldest_idx]->StopNote();
            }
            break;
        }
        
        case QUIETEST_FIRST: {
            // Find the quietest active voice (based on velocity or current level)
            int quietest_idx = -1;
            double quietest_level = 1.0e10; // Large number
            for (int i = 0; i < static_cast<int>(voices.size()); i++) {
                if (voices[i]->active && voices[i]->velocity < quietest_level) {
                    quietest_level = voices[i]->velocity;
                    quietest_idx = i;
                }
            }
            
            if (quietest_idx >= 0) {
                voices[quietest_idx]->StopNote();
            }
            break;
        }
        
        case LAST_PLAYED: {
            // Find the most recently active voice
            int newest_idx = -1;
            double newest_age = 1.0e10; // Large number
            for (int i = 0; i < static_cast<int>(voices.size()); i++) {
                if (voices[i]->active && voices[i]->age < newest_age) {
                    newest_age = voices[i]->age;
                    newest_idx = i;
                }
            }
            
            if (newest_idx >= 0) {
                voices[newest_idx]->StopNote();
            }
            break;
        }
    }
}

void PolyphonyManager::ProcessMonophonicMode() {
    if (active_notes.empty()) {
        polyphonic_output = 0.0;
        return;
    }
    
    // Determine which note to play based on mode
    int note_to_play = 0;
    if (allocation_mode == MONOPHONIC) {
        // Play the highest note
        note_to_play = *std::max_element(active_notes.begin(), active_notes.end());
    } else if (allocation_mode == LEGATO) {
        // Play the most recently played note
        note_to_play = active_notes.back();
    }
    
    // Find a voice to use for this note
    Voice* voice = nullptr;
    for (auto* v : voices) {
        if (!v->active || v->note_number == note_to_play) {
            voice = v;
            break;
        }
    }
    
    if (!voice) {
        // If no voice is available, try to steal one
        for (auto* v : voices) {
            if (v->active) {
                voice = v;
                break;
            }
        }
    }
    
    if (voice) {
        if (!voice->active) {
            // Start new note
            voice->StartNote(note_to_play, 0.8); // Default velocity
        } else if (voice->note_number != note_to_play) {
            // Change to new note
            voice->StartNote(note_to_play, voice->velocity);
        }
        
        // Process this voice
        ProcessVoice(voice);
        polyphonic_output = voice->path->GetOutput() * voice->velocity;
    }
}