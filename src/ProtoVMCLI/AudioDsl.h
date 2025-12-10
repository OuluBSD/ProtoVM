#ifndef _ProtoVM_AudioDsl_h_
#define _ProtoVM_AudioDsl_h_

#include <string>

namespace ProtoVMCLI {

struct AudioDslOscillator {
    std::string id;              // e.g. "osc1"
    double frequency_hz;         // e.g. ~440.0
};

struct AudioDslPanLfo {
    std::string id;              // e.g. "pan_lfo1"
    double rate_hz;              // e.g. 0.25 (one full L->R->L cycle in 4s)
};

struct AudioDslOutputConfig {
    double sample_rate_hz;       // e.g. 48000.0
    double duration_sec;         // e.g. 3.0
};

struct AudioDslGraph {
    std::string block_id;        // underlying block this maps to (oscillator-like)
    AudioDslOscillator osc;
    AudioDslPanLfo pan_lfo;
    AudioDslOutputConfig output;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_AudioDsl_h_