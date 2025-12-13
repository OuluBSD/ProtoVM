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
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_InstrumentBuilder_h_