#ifndef _ProtoVM_PipelineAnalysis_h_
#define _ProtoVM_PipelineAnalysis_h_

#include "PipelineModel.h"
#include "CircuitGraph.h"
#include "TimingAnalysis.h"
#include "ScheduledIr.h"
#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

class PipelineAnalysis {
public:
    // Build a pipeline map for a single block.
    static Result<PipelineMap> BuildPipelineMapForBlock(
        const CircuitGraph& graph,
        const TimingAnalysis* timing,         // optional
        const ScheduledModule* scheduled_ir,  // optional
        const std::string& block_id
    );

    // Optional: Build a pipeline map for a subsystem (multi-block).
    static Result<PipelineMap> BuildPipelineMapForSubsystem(
        const CircuitGraph& graph,
        const TimingAnalysis* timing,
        const ScheduledModule* scheduled_ir,  // may be omitted or approximate
        const std::string& subsystem_id,
        const std::vector<std::string>& block_ids
    );
    
private:
    // Helper methods
    static Result<std::vector<ClockSignalInfo>> DiscoverClockDomains(
        const CircuitGraph& graph,
        const std::string& target_id
    );
    
    static Result<std::vector<RegisterInfo>> DiscoverRegisters(
        const CircuitGraph& graph,
        const std::vector<ClockSignalInfo>& clock_domains,
        const std::string& target_id
    );
    
    static Result<std::vector<PipelineStageInfo>> DiscoverPipelineStages(
        const CircuitGraph& graph,
        const ScheduledModule* scheduled_ir,
        const std::vector<RegisterInfo>& registers,
        const std::string& target_id
    );
    
    static Result<std::vector<RegToRegPathInfo>> DiscoverRegToRegPaths(
        const CircuitGraph& graph,
        const TimingAnalysis* timing,
        const std::vector<RegisterInfo>& registers,
        const std::vector<PipelineStageInfo>& stages,
        const std::string& target_id
    );
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_PipelineAnalysis_h_