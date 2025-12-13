#include "InstrumentBuilder.h"
#include "InstrumentGraph.h"
#include <cmath>  // For standard math functions
#include <algorithm> // For std::min/max

namespace ProtoVMCLI {

Result<InstrumentGraph> InstrumentBuilder::BuildHybridInstrument(
    const Upp::String& instrument_id,
    const InstrumentVoiceTemplate& voice_template,
    double sample_rate_hz,
    int voice_count,
    const NoteDesc& note,
    double detune_spread_cents) {
    
    // Validate inputs
    if (voice_count <= 0) {
        return Result<InstrumentGraph>::MakeError(
            ErrorCode::InvalidInput,
            "Voice count must be greater than 0"
        );
    }
    
    if (sample_rate_hz <= 0) {
        return Result<InstrumentGraph>::MakeError(
            ErrorCode::InvalidInput,
            "Sample rate must be greater than 0"
        );
    }
    
    if (note.duration_sec <= 0) {
        return Result<InstrumentGraph>::MakeError(
            ErrorCode::InvalidInput,
            "Duration must be greater than 0"
        );
    }
    
    if (note.base_freq_hz <= 0) {
        return Result<InstrumentGraph>::MakeError(
            ErrorCode::InvalidInput,
            "Base frequency must be greater than 0"
        );
    }
    
    // Build the instrument graph
    InstrumentGraph instrument;
    instrument.instrument_id = instrument_id;
    instrument.sample_rate_hz = sample_rate_hz;
    instrument.voice_count = voice_count;
    instrument.voice_template = voice_template;
    instrument.note = note;
    
    // Set analog primary based on template
    instrument.use_analog_primary = !voice_template.analog_block_id.IsEmpty();
    
    // Generate voice configurations
    std::vector<VoiceConfig> voices;
    for (int i = 0; i < voice_count; i++) {
        VoiceConfig voice;
        voice.id = Upp::String().Cat() << "voice" << i;
        
        // Calculate detune distribution
        // Distribute detune linearly across the spread
        // For voice i out of voice_count voices, we want to distribute detunes
        // For example, with 4 voices and 10 cent spread: -5, -1.67, 1.67, 5
        if (voice_count == 1) {
            voice.detune_cents = 0.0;
        } else {
            // Calculate the step size (-spread/2 to +spread/2)
            double step = detune_spread_cents / (voice_count - 1);
            voice.detune_cents = -detune_spread_cents / 2.0 + i * step;
        }
        
        // Determine if this voice uses analog or digital source
        // If analog is primary and we have an analog block ID, use analog
        // Otherwise use digital
        voice.use_analog_source = instrument.use_analog_primary &&
                                  !voice_template.analog_block_id.IsEmpty();
        
        voices.push_back(voice);
    }
    
    instrument.voices = voices;
    
    return Result<InstrumentGraph>::MakeOk(instrument);
}

} // namespace ProtoVMCLI