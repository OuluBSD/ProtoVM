#ifndef _ProtoVM_GlobalPipeline_h_
#define _ProtoVM_GlobalPipeline_h_

#include "SessionTypes.h"  // For Result<T>
#include "ProtoVM.h"       // For Upp types
#include "PipelineModel.h" // For PipelineMap, ClockSignalInfo, etc.
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Represents a logical end-to-end path within a subsystem.
struct GlobalPipelinePath {
    String path_id;

    // Sequence of registers encountered along the path.
    Vector<String> reg_ids;         // in traversal order

    // Blocks involved along the path (may repeat).
    Vector<String> block_ids;

    // Stage / domain metadata.
    int domain_id;
    int total_stages;               // number of register-to-register hops
    int total_comb_depth_estimate;  // sum or max of depth segments (implementation choice)

    // Optional: per-segment depths.
    Vector<int> segment_depths;     // depth between consecutive regs
};

// Represents a global "stage band" across multiple blocks.
struct GlobalPipelineStage {
    int stage_index;                // 0..N-1 along some reference path
    int domain_id;

    // Registers that conceptually belong to this stage, across blocks.
    Vector<String> reg_ids;

    // Blocks that have logic contributing to this stage.
    Vector<String> block_ids;

    int max_comb_depth_estimate;    // worst-case combinational depth in this stage
    int avg_comb_depth_estimate;    // optional approximate average depth
};

// Represents a global pipeline model for a subsystem.
struct GlobalPipelineMap {
    String subsystem_id;
    Vector<String> block_ids;

    // Domain(s) involved in this subsystem.
    Vector<ClockSignalInfo> clock_domains;

    // Per-domain global stages (you can keep a flat vector with domain_id tags).
    Vector<GlobalPipelineStage> stages;

    // End-to-end paths of interest (e.g. from subsystem inputs to outputs).
    Vector<GlobalPipelinePath> paths;

    // Optional summary metrics:
    int max_total_depth = -1;
    int max_stages = -1;
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_GlobalPipeline_h_