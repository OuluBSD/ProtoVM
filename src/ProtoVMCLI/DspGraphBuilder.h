#ifndef _ProtoVM_DSP_GRAPH_BUILDER_h_
#define _ProtoVM_DSP_GRAPH_BUILDER_h_

#include "DspGraph.h"
#include "AudioDsl.h"
#include "SessionTypes.h"  // For the Result template
#include <string>

namespace ProtoVMCLI {

class DspGraphBuilder {
public:
    static Result<DspGraph> BuildGraphFromAudioDsl(
        const AudioDslGraph& audio_graph
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_DSP_GRAPH_BUILDER_h_