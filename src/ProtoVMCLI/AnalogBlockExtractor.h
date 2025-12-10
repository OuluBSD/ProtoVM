#ifndef _ProtoVM_AnalogBlockExtractor_h_
#define _ProtoVM_AnalogBlockExtractor_h_

#include "AnalogModel.h"
#include "CircuitGraph.h"  // Include CircuitGraph for the extractor
#include "SessionTypes.h"  // Include session types
#include <string>

namespace ProtoVMCLI {

class AnalogBlockExtractor {
public:
    // Build a simplified analog model from an analog-oriented circuit block.
    static Result<AnalogBlockModel> ExtractAnalogModelForBlock(
        const Upp::String& block_id,
        const CircuitGraph& graph
        // Optionally more context (e.g. analog component metadata) if available
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_AnalogBlockExtractor_h_