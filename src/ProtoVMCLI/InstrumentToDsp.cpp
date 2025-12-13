#include "InstrumentToDsp.h"
#include "DspGraphBuilder.h"
#include "DspGraph.h"
#include <cmath>  // For standard math functions
#include <algorithm> // For std::min/max

namespace ProtoVMCLI {

Result<DspGraph> InstrumentToDsp::BuildDspGraphForInstrument(
    const InstrumentGraph& instrument,
    const CircuitFacade& facade,
    const SessionMetadata& session,
    const std::string& session_dir,
    const std::string& branch_name) {
    
    // Create the DSP graph
    DspGraph graph;
    graph.graph_id = Upp::String().Cat() << "INSTR_" << instrument.instrument_id << "_DSP";
    graph.sample_rate_hz = instrument.sample_rate_hz;
    graph.total_samples = static_cast<int>(instrument.sample_rate_hz * instrument.note.duration_sec);
    graph.block_size = 64;  // Default block size
    
    // Add a mixer node to combine all voices
    DspNode mixer_node;
    mixer_node.id = "mixer";
    mixer_node.kind = DspNodeKind::Mixer;
    
    // Calculate inputs based on voice count - each voice contributes L and R
    for (int i = 0; i < instrument.voice_count; i++) {
        mixer_node.input_port_names.push_back(Upp::String().Cat() << "inL" << i);
        mixer_node.input_port_names.push_back(Upp::String().Cat() << "inR" << i);
    }
    mixer_node.output_port_names.push_back("outL");
    mixer_node.output_port_names.push_back("outR");
    
    graph.nodes.push_back(mixer_node);
    
    // Process each voice
    for (int i = 0; i < instrument.voice_count; i++) {
        const VoiceConfig& voice = instrument.voices[i];
        
        // Calculate the detuned frequency
        double detuned_freq = ApplyDetune(instrument.note.base_freq_hz, voice.detune_cents);
        
        DspNode source_node;
        Upp::String source_output_port = "out";
        
        if (voice.use_analog_source && !instrument.voice_template.analog_block_id.IsEmpty()) {
            // Use analog source
            source_node.id = Upp::String().Cat() << "analog_source_" << i;
            source_node.kind = DspNodeKind::AnalogBlockSource;
            
            // Extract analog model for this voice
            auto analog_result = facade.ExtractAnalogModelForBlockInBranch(
                session, 
                session_dir, 
                branch_name,
                instrument.voice_template.analog_block_id
            );
            
            if (!analog_result.ok) {
                return Result<DspGraph>::MakeError(
                    analog_result.error_code,
                    "Failed to extract analog model for voice " + std::to_string(i) + ": " + analog_result.error_message
                );
            }
            
            // Store the analog model ID as a parameter
            source_node.param_keys.push_back("analog_model_id");
            source_node.param_values.push_back(std::stod(analog_result.data.id));
        } else {
            // Use digital oscillator
            source_node.id = Upp::String().Cat() << "osc_" << i;
            source_node.kind = DspNodeKind::Oscillator;
            
            // Add frequency parameter
            source_node.param_keys.push_back("frequency_hz");
            source_node.param_values.push_back(detuned_freq);
        }
        
        // Add output port for the source
        source_node.output_port_names.push_back("out");
        
        graph.nodes.push_back(source_node);
        
        // Add pan LFO for this voice
        DspNode pan_lfo_node;
        pan_lfo_node.id = Upp::String().Cat() << "pan_lfo_" << i;
        pan_lfo_node.kind = DspNodeKind::PanLfo;
        
        // Add rate parameter
        pan_lfo_node.param_keys.push_back("rate_hz");
        pan_lfo_node.param_values.push_back(instrument.voice_template.pan_lfo_hz);
        
        // Add output port
        pan_lfo_node.output_port_names.push_back("out");
        
        graph.nodes.push_back(pan_lfo_node);
        
        // Add stereo panner for this voice
        DspNode panner_node;
        panner_node.id = Upp::String().Cat() << "panner_" << i;
        panner_node.kind = DspNodeKind::StereoPanner;
        
        // Add input ports
        panner_node.input_port_names.push_back("audio_in");
        panner_node.input_port_names.push_back("pan_ctrl");
        
        // Add output ports
        panner_node.output_port_names.push_back("outL");
        panner_node.output_port_names.push_back("outR");
        
        graph.nodes.push_back(panner_node);
        
        // Create connections for this voice
        DspConnection conn1, conn2, conn3, conn4;
        
        // Source out -> panner audio_in
        conn1.from.node_id = source_node.id;
        conn1.from.port_name = "out";
        conn1.to.node_id = panner_node.id;
        conn1.to.port_name = "audio_in";
        graph.connections.push_back(conn1);
        
        // Pan LFO out -> panner pan_ctrl
        conn2.from.node_id = pan_lfo_node.id;
        conn2.from.port_name = "out";
        conn2.to.node_id = panner_node.id;
        conn2.to.port_name = "pan_ctrl";
        graph.connections.push_back(conn2);
        
        // Panner outL -> mixer inL<i>
        conn3.from.node_id = panner_node.id;
        conn3.from.port_name = "outL";
        conn3.to.node_id = mixer_node.id;
        conn3.to.port_name = Upp::String().Cat() << "inL" << i;
        graph.connections.push_back(conn3);
        
        // Panner outR -> mixer inR<i>
        conn4.from.node_id = panner_node.id;
        conn4.from.port_name = "outR";
        conn4.to.node_id = mixer_node.id;
        conn4.to.port_name = Upp::String().Cat() << "inR" << i;
        graph.connections.push_back(conn4);
    }
    
    // Add output sink to receive the mixed signals
    DspNode output_node;
    output_node.id = "output_sink";
    output_node.kind = DspNodeKind::OutputSink;
    
    // Add input ports
    output_node.input_port_names.push_back("inL");
    output_node.input_port_names.push_back("inR");
    
    graph.nodes.push_back(output_node);
    
    // Connect mixer outputs to output sink
    DspConnection out_conn1, out_conn2;
    
    // Mixer outL -> output sink inL
    out_conn1.from.node_id = mixer_node.id;
    out_conn1.from.port_name = "outL";
    out_conn1.to.node_id = output_node.id;
    out_conn1.to.port_name = "inL";
    graph.connections.push_back(out_conn1);
    
    // Mixer outR -> output sink inR
    out_conn2.from.node_id = mixer_node.id;
    out_conn2.from.port_name = "outR";
    out_conn2.to.node_id = output_node.id;
    out_conn2.to.port_name = "inR";
    graph.connections.push_back(out_conn2);
    
    // Set special node IDs for runtime tracking
    graph.output_node_id = "output_sink";
    
    return Result<DspGraph>::MakeOk(graph);
}

} // namespace ProtoVMCLI