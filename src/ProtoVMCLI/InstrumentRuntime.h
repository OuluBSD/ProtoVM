#ifndef _ProtoVM_InstrumentRuntime_h_
#define _ProtoVM_InstrumentRuntime_h_

#include "InstrumentGraph.h"
#include "SessionTypes.h"  // Include session types
#include "CircuitFacade.h" // For CircuitFacade
#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include <vector>

namespace ProtoVMCLI {

class InstrumentRuntime {
public:
    static Result<void> RenderInstrument(
        const InstrumentGraph& instrument,
        CircuitFacade& facade,
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name,
        std::vector<float>& out_left,
        std::vector<float>& out_right
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_InstrumentRuntime_h_