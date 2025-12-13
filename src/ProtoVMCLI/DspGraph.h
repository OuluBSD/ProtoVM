#ifndef _ProtoVM_DSP_GRAPH_h_
#define _ProtoVM_DSP_GRAPH_h_

#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include "SessionTypes.h"  // Include session types
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class DspNodeKind {
    Oscillator,       // audio-rate oscillator
    PanLfo,           // low-frequency oscillator for panning
    StereoPanner,     // maps mono + pan to stereo L/R
    OutputSink,       // writes stereo samples to buffers
    AnalogBlockSource, // NEW: driven by AnalogSolver
    Mixer             // NEW: mixes multiple inputs
};

enum class DspPortDirection {
    Input,
    Output
};

enum class DspPortType {
    Audio,   // per-sample audio signal
    Control  // scalar control values (e.g. freq, pan position)
};

struct DspPortId {
    Upp::String node_id;
    Upp::String port_name; // e.g. "in", "outL", "outR", "freq", "phase"
    
    bool operator==(const DspPortId& other) const {
        return node_id == other.node_id && port_name == other.port_name;
    }
};

struct DspNode {
    Upp::String id;
    DspNodeKind kind;

    // Port declarations (names and types).
    std::vector<Upp::String> input_port_names;
    std::vector<Upp::String> output_port_names;

    // Node-local parameters (e.g. base frequency).
    // Simple map-like structure; store as key/value pairs.
    std::vector<Upp::String> param_keys;
    std::vector<double> param_values;
};

struct DspConnection {
    DspPortId from;
    DspPortId to;
    
    bool operator==(const DspConnection& other) const {
        return from == other.from && to == other.to;
    }
};

struct DspGraph {
    Upp::String graph_id;

    double sample_rate_hz;
    int block_size;        // e.g. 64 or 256
    int total_samples;     // e.g. sample_rate * duration_sec

    std::vector<DspNode> nodes;
    std::vector<DspConnection> connections;

    // Optional convenience: IDs of special nodes.
    Upp::String osc_node_id;
    Upp::String pan_lfo_node_id;
    Upp::String panner_node_id;
    Upp::String output_node_id;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_DSP_GRAPH_h_