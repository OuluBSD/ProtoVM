#ifndef _ProtoVM_GlobalPipelineAnalysis_h_
#define _ProtoVM_GlobalPipelineAnalysis_h_

#include "GlobalPipeline.h"
#include "PipelineModel.h"
#include "CircuitGraph.h"
#include "TimingAnalysis.h"
#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

class GlobalPipelineAnalysis {
public:
    static Result<GlobalPipelineMap> BuildGlobalPipelineMapForSubsystem(
        const String& subsystem_id,
        const Vector<String>& block_ids,
        const Vector<PipelineMap>& per_block_pipelines,
        const CircuitGraph& graph,
        const TimingAnalysis* timing // optional but recommended
    );

private:
    // Helper methods for building the global pipeline map
    static Result<std::vector<GlobalPipelinePath>> FindGlobalPaths(
        const String& subsystem_id,
        const Vector<String>& block_ids,
        const Vector<PipelineMap>& per_block_pipelines,
        const CircuitGraph& graph
    );

    static Result<std::vector<GlobalPipelineStage>> BuildGlobalStages(
        const String& subsystem_id,
        const Vector<String>& block_ids,
        const Vector<PipelineMap>& per_block_pipelines,
        const std::vector<GlobalPipelinePath>& paths
    );

    static void UpdateMetrics(GlobalPipelineMap& global_map);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_GlobalPipelineAnalysis_h_