#ifndef _ProtoVM_InstrumentBuilder_h_
#define _ProtoVM_InstrumentBuilder_h_

#include "InstrumentGraph.h"
#include "SessionTypes.h"  // Include session types
#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include <string>
#include <vector>

namespace ProtoVMCLI {

class InstrumentBuilder {
public:
    // Build a simple hybrid instrument configuration.
    static Result<InstrumentGraph> BuildHybridInstrument(
        const Upp::String& instrument_id,
        const InstrumentVoiceTemplate& voice_template,
        double sample_rate_hz,
        int voice_count,
        const NoteDesc& note,
        double detune_spread_cents // e.g. +/- total spread for voices
    )
    {
        InstrumentGraph graph;
        graph.instrument_id = instrument_id;
        graph.sample_rate_hz = sample_rate_hz;
        graph.voice_count = voice_count;
        graph.voice_template = voice_template;
        graph.note = note;

        for (int v = 0; v < voice_count; v++) {
            VoiceConfig cfg;
            cfg.id = Upp::String("voice") + Upp::AsString(v);
            // Spread detune symmetrically around 0: [-spread/2, +spread/2]
            cfg.detune_cents = (voice_count > 1)
                ? detune_spread_cents * ((2.0 * v) / (voice_count - 1) - 1.0)
                : 0.0;
            graph.voices.push_back(cfg);
        }

        return Result<InstrumentGraph>::MakeOk(graph);
    }
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_InstrumentBuilder_h_