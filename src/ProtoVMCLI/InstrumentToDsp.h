#ifndef _ProtoVM_InstrumentToDsp_h_
#define _ProtoVM_InstrumentToDsp_h_

#include "InstrumentGraph.h"
#include "DspGraph.h"
#include "SessionTypes.h"  // Include session types
#include "CircuitFacade.h" // For CircuitFacade
#include <ProtoVM/ProtoVM.h>  // Include U++ types
#include <string>
#include <vector>

namespace ProtoVMCLI {

class InstrumentToDsp {
public:
    static Result<DspGraph> BuildDspGraphForInstrument(
        const InstrumentGraph& instrument,
        const CircuitFacade& facade,        // to fetch AnalogBlockModel etc.
        const SessionMetadata& session,
        const std::string& session_dir,
        const std::string& branch_name
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_InstrumentToDsp_h_