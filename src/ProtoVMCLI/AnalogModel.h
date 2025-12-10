#ifndef _ProtoVM_AnalogModel_h_
#define _ProtoVM_AnalogModel_h_

#include "ProtoVM.h"  // Include U++ types
#include "SessionTypes.h"  // Include session types
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class AnalogBlockKind {
    RcOscillator,
    SimpleFilter,
    TransistorStage,
    Unknown
};

// Types of internal states we track.
enum class AnalogStateKind {
    Voltage,
    Current
};

struct AnalogStateVar {
    Upp::String name;          // e.g. "v_cap", "v_out"
    AnalogStateKind kind;
    double value;         // current state
};

struct AnalogParam {
    Upp::String name;          // e.g. "R", "C", "gain", "bias"
    double value;
};

struct AnalogBlockModel {
    Upp::String id;               // e.g. "ANALOG_OSC1"
    Upp::String block_id;         // underlying ProtoVM block/circuit id

    AnalogBlockKind kind;

    // State variables (e.g. capacitor voltages).
    std::vector<AnalogStateVar> state;

    // Parameters (R, C, effective gain, etc.).
    std::vector<AnalogParam> params;

    // Output node index or mapping; simple single-output for now.
    Upp::String output_state_name; // which state variable is treated as audio output

    // Optional: natural frequency estimate, useful for debugging or tuning.
    double estimated_freq_hz = -1.0;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_AnalogModel_h_