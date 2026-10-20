#ifndef _ProtoVM_GlobalPipeline_h_
#define _ProtoVM_GlobalPipeline_h_

#include "SessionTypes.h"  // For Result<T>
#include <ProtoVM/ProtoVM.h>       // For Upp types
#include "PipelineModel.h" // For PipelineMap, ClockSignalInfo, etc.
#include <string>
#include <vector>

namespace ProtoVMCLI {

// Represents a logical end-to-end path within a subsystem.
struct GlobalPipelinePath : public Upp::Moveable<GlobalPipelinePath> {
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

    GlobalPipelinePath() : domain_id(0), total_stages(0), total_comb_depth_estimate(0) {}
    GlobalPipelinePath(const GlobalPipelinePath& other) {
        path_id = other.path_id;
        reg_ids <<= other.reg_ids;
        block_ids <<= other.block_ids;
        domain_id = other.domain_id;
        total_stages = other.total_stages;
        total_comb_depth_estimate = other.total_comb_depth_estimate;
        segment_depths <<= other.segment_depths;
    }
    GlobalPipelinePath& operator=(const GlobalPipelinePath& other) {
        path_id = other.path_id;
        reg_ids <<= other.reg_ids;
        block_ids <<= other.block_ids;
        domain_id = other.domain_id;
        total_stages = other.total_stages;
        total_comb_depth_estimate = other.total_comb_depth_estimate;
        segment_depths <<= other.segment_depths;
        return *this;
    }
};

// Represents a global "stage band" across multiple blocks.
struct GlobalPipelineStage : public Upp::Moveable<GlobalPipelineStage> {
    int stage_index;                // 0..N-1 along some reference path
    int domain_id;

    // Registers that conceptually belong to this stage, across blocks.
    Vector<String> reg_ids;

    // Blocks that have logic contributing to this stage.
    Vector<String> block_ids;

    int max_comb_depth_estimate;    // worst-case combinational depth in this stage
    int avg_comb_depth_estimate;    // optional approximate average depth

    GlobalPipelineStage() : stage_index(0), domain_id(0), max_comb_depth_estimate(0), avg_comb_depth_estimate(0) {}
    GlobalPipelineStage(const GlobalPipelineStage& other)
        : stage_index(other.stage_index)
        , domain_id(other.domain_id)
        , max_comb_depth_estimate(other.max_comb_depth_estimate)
        , avg_comb_depth_estimate(other.avg_comb_depth_estimate) {
        reg_ids <<= other.reg_ids;
        block_ids <<= other.block_ids;
    }
    GlobalPipelineStage& operator=(const GlobalPipelineStage& other) {
        stage_index = other.stage_index;
        domain_id = other.domain_id;
        reg_ids <<= other.reg_ids;
        block_ids <<= other.block_ids;
        max_comb_depth_estimate = other.max_comb_depth_estimate;
        avg_comb_depth_estimate = other.avg_comb_depth_estimate;
        return *this;
    }
};

// Represents a global pipeline model for a subsystem.
struct GlobalPipelineMap : public Upp::Moveable<GlobalPipelineMap> {
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

    GlobalPipelineMap() {}
    GlobalPipelineMap(const GlobalPipelineMap& other)
        : subsystem_id(other.subsystem_id)
        , max_total_depth(other.max_total_depth)
        , max_stages(other.max_stages) {
        block_ids <<= other.block_ids;
        for (const auto& domain : other.clock_domains)
            clock_domains.Add(domain);
        for (const auto& stage : other.stages)
            stages.Add(stage);
        for (const auto& path : other.paths)
            paths.Add(path);
    }
    GlobalPipelineMap& operator=(const GlobalPipelineMap& other) {
        subsystem_id = other.subsystem_id;
        block_ids <<= other.block_ids;
        clock_domains <<= other.clock_domains;
        stages <<= other.stages;
        paths <<= other.paths;
        max_total_depth = other.max_total_depth;
        max_stages = other.max_stages;
        return *this;
    }
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_GlobalPipeline_h_