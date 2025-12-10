#include "DspGraphBuilder.h"
#include "DspGraph.h"
#include "AudioDsl.h"
#include "SessionTypes.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

Result<DspGraph> DspGraphBuilder::BuildGraphFromAudioDsl(
    const AudioDslGraph& audio_graph
) {
    DspGraph graph;
    
    // Set up basic graph properties
    graph.graph_id = "DSP_" + audio_graph.block_id;
    graph.sample_rate_hz = audio_graph.output.sample_rate_hz;
    graph.block_size = 256; // Default block size
    graph.total_samples = static_cast<int>(audio_graph.output.sample_rate_hz * audio_graph.output.duration_sec);
    
    // Create oscillator node
    DspNode osc_node;
    osc_node.id = "osc";
    osc_node.kind = DspNodeKind::Oscillator;
    osc_node.input_port_names = {}; // Oscillator has no audio inputs
    osc_node.output_port_names = {"out"};
    osc_node.param_keys = {"frequency_hz"};
    osc_node.param_values = {audio_graph.osc.frequency_hz};
    
    graph.osc_node_id = osc_node.id;
    graph.nodes.push_back(osc_node);
    
    // Create pan LFO node
    DspNode pan_lfo_node;
    pan_lfo_node.id = "pan_lfo";
    pan_lfo_node.kind = DspNodeKind::PanLfo;
    pan_lfo_node.input_port_names = {};
    pan_lfo_node.output_port_names = {"pan"};
    pan_lfo_node.param_keys = {"rate_hz"};
    pan_lfo_node.param_values = {audio_graph.pan_lfo.rate_hz};
    
    graph.pan_lfo_node_id = pan_lfo_node.id;
    graph.nodes.push_back(pan_lfo_node);
    
    // Create stereo panner node
    DspNode panner_node;
    panner_node.id = "panner";
    panner_node.kind = DspNodeKind::StereoPanner;
    panner_node.input_port_names = {"in", "pan"};
    panner_node.output_port_names = {"outL", "outR"};
    // No static params needed for stereo panner
    
    graph.panner_node_id = panner_node.id;
    graph.nodes.push_back(panner_node);
    
    // Create output sink node
    DspNode output_node;
    output_node.id = "output";
    output_node.kind = DspNodeKind::OutputSink;
    output_node.input_port_names = {"inL", "inR"};
    output_node.output_port_names = {}; // Output sink has no outputs
    // Store total_samples as a parameter for the output node
    output_node.param_keys = {"total_samples"};
    output_node.param_values = {static_cast<double>(graph.total_samples)};
    
    graph.output_node_id = output_node.id;
    graph.nodes.push_back(output_node);
    
    // Create connections
    // osc.out → panner.in
    DspConnection conn1;
    conn1.from.node_id = "osc";
    conn1.from.port_name = "out";
    conn1.to.node_id = "panner";
    conn1.to.port_name = "in";
    graph.connections.push_back(conn1);
    
    // pan_lfo.pan → panner.pan
    DspConnection conn2;
    conn2.from.node_id = "pan_lfo";
    conn2.from.port_name = "pan";
    conn2.to.node_id = "panner";
    conn2.to.port_name = "pan";
    graph.connections.push_back(conn2);
    
    // panner.outL → output.inL
    DspConnection conn3;
    conn3.from.node_id = "panner";
    conn3.from.port_name = "outL";
    conn3.to.node_id = "output";
    conn3.to.port_name = "inL";
    graph.connections.push_back(conn3);
    
    // panner.outR → output.inR
    DspConnection conn4;
    conn4.from.node_id = "panner";
    conn4.from.port_name = "outR";
    conn4.to.node_id = "output";
    conn4.to.port_name = "inR";
    graph.connections.push_back(conn4);
    
    return Result<DspGraph>::MakeOk(graph);
}

} // namespace ProtoVMCLI