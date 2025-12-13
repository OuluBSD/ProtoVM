#ifndef _ProtoVM_InstrumentGraph_h_
#define _ProtoVM_InstrumentGraph_h_

#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include "SessionTypes.h"  // Include session types
#include "DspGraph.h"      // Include DSP graph structures
#include "AnalogModel.h"   // Include analog model structures
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct NoteDesc {
    double base_freq_hz;    // e.g. 440.0 (A4)
    double velocity;        // 0..1 for now (simple amplitude scaling)
    double duration_sec;    // e.g. 3.0
};

struct VoiceConfig {
    Upp::String id;              // e.g. "voice0", "voice1"
    double detune_cents;         // -50..+50 etc., optional
    bool use_analog_source;      // if true, voice source is AnalogBlockModel; else digital oscillator
};

struct InstrumentVoiceTemplate {
    Upp::String id;                      // e.g. "main_voice"
    Upp::String analog_block_id;         // optional: underlying analog circuit block ID
    Upp::String digital_block_id;        // optional: digital oscillator block ID (for Codegen/DSP osc)

    // Simple routing flags; we keep this phase minimal.
    bool has_pan_lfo = true;
    double pan_lfo_hz = 0.25;
    bool has_filter = false;        // reserved for future

    // Per-voice DSP parameters can be here or derived later.
};

struct InstrumentGraph {
    Upp::String instrument_id;           // e.g. "HYBRID_OSC_1"

    double sample_rate_hz;          // e.g. 48000.0
    int voice_count;                // e.g. 4

    InstrumentVoiceTemplate voice_template;
    std::vector<VoiceConfig> voices;

    NoteDesc note;

    // Optional: high-level mode
    bool use_analog_primary = true; // analog vs digital main source
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_InstrumentGraph_h_