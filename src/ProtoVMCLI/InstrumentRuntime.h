#ifndef _ProtoVM_InstrumentRuntime_h_
#define _ProtoVM_InstrumentRuntime_h_

#include "InstrumentGraph.h"
#include "SessionTypes.h"  // Include session types
#include "CircuitFacade.h" // For CircuitFacade
#include <ProtoVM/ProtoVM.h>
#include <cmath>  // Include U++ types
#include <vector>

namespace ProtoVMCLI {

class InstrumentRuntime {
public:
    // Renders the instrument offline: per-voice sine synthesis at the note
    // frequency with the voice detune applied, amplitude scaled by velocity.
    // (The CircuitFacade/session parameters are carried for a future
    // engine-rendered path and intentionally unused here.)
    static Result<void> RenderInstrument(
        const InstrumentGraph& instrument,
        CircuitFacade& facade,
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        std::vector<float>& out_left,
        std::vector<float>& out_right
    )
    {
        const double sample_rate = instrument.sample_rate_hz;
        const size_t frames = static_cast<size_t>(sample_rate * instrument.note.duration_sec);
        out_left.assign(frames, 0.0f);
        out_right.assign(frames, 0.0f);

        if (instrument.voices.empty())
            return Result<void>::MakeOk();

        for (const auto& voice : instrument.voices) {
            const double freq = instrument.note.base_freq_hz
                * std::pow(2.0, voice.detune_cents / 1200.0);
            const float amp = static_cast<float>(instrument.note.velocity)
                / static_cast<float>(instrument.voices.size());
            for (size_t i = 0; i < frames; i++) {
                const double t = static_cast<double>(i) / sample_rate;
                out_left[i] += amp * static_cast<float>(std::sin(2.0 * M_PI * freq * t));
                out_right[i] += amp * static_cast<float>(
                    std::sin(2.0 * M_PI * freq * t + (voice.detune_cents / 1200.0) * 0.5));
            }
        }

        return Result<void>::MakeOk();
    }
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_InstrumentRuntime_h_