#include "GlobalPipelineAnalysis.h"
#include "CircuitGraphQueries.h"
#include <algorithm>
#include <map>
#include <set>
#include <queue>

namespace ProtoVMCLI {

Result<GlobalPipelineMap> GlobalPipelineAnalysis::BuildGlobalPipelineMapForSubsystem(
    const String& subsystem_id,
    const Vector<String>& block_ids,
    const Vector<PipelineMap>& per_block_pipelines,
    const CircuitGraph& graph,
    const TimingAnalysis* timing
) {
    GlobalPipelineMap global_map;
    global_map.subsystem_id = subsystem_id;
    global_map.block_ids <<= block_ids;

    // Collect all clock domains from the per-block pipelines
    std::set<std::string> unique_domains;
    for (const auto& pipeline : per_block_pipelines) {
        for (const auto& domain : pipeline.clock_domains) {
            unique_domains.insert(domain.signal_name);
            // Only add if not already present to avoid duplicates
            bool found = false;
            for (const auto& existing_domain : global_map.clock_domains) {
                if (existing_domain.signal_name == domain.signal_name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                global_map.clock_domains.Add(domain);
            }
        }
    }

    // Find all global paths
    auto paths_result = FindGlobalPaths(subsystem_id, block_ids, per_block_pipelines, graph);
    if (!paths_result.ok) {
        return Result<GlobalPipelineMap>::MakeError(paths_result.error_code, paths_result.error_message);
    }
    global_map.paths <<= paths_result.data;

    // Build global stages based on paths and per-block pipelines
    auto stages_result = BuildGlobalStages(subsystem_id, block_ids, per_block_pipelines, global_map.paths);
    if (!stages_result.ok) {
        return Result<GlobalPipelineMap>::MakeError(stages_result.error_code, stages_result.error_message);
    }
    global_map.stages <<= stages_result.data;

    // Update global metrics
    UpdateMetrics(global_map);

    return Result<GlobalPipelineMap>::MakeOk(std::move(global_map));
}

Result<Vector<GlobalPipelinePath>> GlobalPipelineAnalysis::FindGlobalPaths(
    const String& subsystem_id,
    const Vector<String>& block_ids,
    const Vector<PipelineMap>& per_block_pipelines,
    const CircuitGraph& graph
) {
    Vector<GlobalPipelinePath> paths;

    // Find all registers in the subsystem blocks
    std::set<String> subsystem_registers;
    for (const auto& pipeline : per_block_pipelines) {
        for (const auto& reg : pipeline.registers) {
            subsystem_registers.insert(reg.reg_id);
        }
    }

    // Find connections between registers across blocks
    // We'll look for paths that span multiple blocks
    std::set<String> processed_paths; // To avoid duplicate paths
    
    for (const auto& block_id : block_ids) {
        // Find the pipeline for this block
        const PipelineMap* current_pipeline = nullptr;
        for (const auto& pipeline : per_block_pipelines) {
            if (pipeline.id == block_id.ToStd()) {
                current_pipeline = &pipeline;
                break;
            }
        }
        
        if (!current_pipeline) continue;

        // For each register in this block's pipeline, try to find paths to other blocks
        for (const auto& reg : current_pipeline->registers) {
            // CircuitGraph exposes no direct "outputs" query; resolve the two-hop
            // signal-flow fan-out (register -> net/pin -> register) from the edges.
            for (const auto& edge : graph.edges) {
                if (edge.kind != GraphEdgeKind::SignalFlow ||
                    edge.from.kind != GraphNodeKind::Component ||
                    edge.from.id != reg.reg_id)
                    continue;

                const GraphNodeId& mid = edge.to;
                for (const auto& edge2 : graph.edges) {
                    if (edge2.from.kind != mid.kind || edge2.from.id != mid.id)
                        continue;
                    if (edge2.to.kind != GraphNodeKind::Component)
                        continue;

                // Check if the output goes to a register in another block
                const std::string& output_reg = edge2.to.id;
                if (subsystem_registers.count(Upp::String(output_reg))) {
                    // Found a path between registers across blocks
                    GlobalPipelinePath path;
                    path.path_id = subsystem_id + "_path_000"; // TODO: Generate unique ID
                    path.reg_ids.Add(reg.reg_id);
                    path.reg_ids.Add(output_reg);
                    
                    // Determine which blocks this path spans
                    String src_block = "";
                    String dst_block = "";
                    
                    for (const auto& pipeline : per_block_pipelines) {
                        for (const auto& reg_info : pipeline.registers) {
                            if (reg_info.reg_id == reg.reg_id) {
                                src_block = pipeline.id;
                            }
                            if (reg_info.reg_id == output_reg) {
                                dst_block = pipeline.id;
                            }
                        }
                    }
                    
                    path.block_ids.Add(src_block);
                    path.block_ids.Add(dst_block);
                    
                    // Set domain info based on the source register
                    for (const auto& reg_info : current_pipeline->registers) {
                        if (reg_info.reg_id == reg.reg_id) {
                            path.domain_id = reg_info.domain_id;
                            break;
                        }
                    }
                    
                    path.total_stages = 1; // Single hop
                    path.total_comb_depth_estimate = 5; // Placeholder - would come from timing analysis
                    path.segment_depths.Add(5); // Placeholder
                    
                    paths.Add(path);
                }
                }
            }
        }
    }

    // More sophisticated path finding would require:
    // - Tracing through the circuit graph to find longer paths
    // - Identifying paths that span multiple stages across multiple blocks
    // - Finding paths that start from subsystem inputs and end at subsystem outputs

    return Result<Vector<GlobalPipelinePath>>::MakeOk(std::move(paths));
}

Result<Vector<GlobalPipelineStage>> GlobalPipelineAnalysis::BuildGlobalStages(
    const String& subsystem_id,
    const Vector<String>& block_ids,
    const Vector<PipelineMap>& per_block_pipelines,
    const Vector<GlobalPipelinePath>& paths
) {
    Vector<GlobalPipelineStage> stages;
    
    // Map register IDs to their global stage index based on per-block pipeline stage indices
    std::map<String, int> reg_to_stage;
    std::map<int, std::set<String>> stage_to_regs;  // stage index -> set of registers
    std::map<int, std::set<String>> stage_to_blocks; // stage index -> set of blocks
    
    // Aggregate stage information from all blocks
    for (size_t block_idx = 0; block_idx < per_block_pipelines.size(); ++block_idx) {
        const auto& pipeline = per_block_pipelines[block_idx];
        const String& block_id = block_ids[block_idx];
        
        for (const auto& stage : pipeline.stages) {
            // Create a global stage index if it doesn't exist yet
            int global_stage_idx = stage.stage_index;
            
            // Add registers from this stage to the global stage
            for (const auto& reg_id : stage.registers_in) {
                reg_to_stage[reg_id] = global_stage_idx;
                stage_to_regs[global_stage_idx].insert(reg_id);
                stage_to_blocks[global_stage_idx].insert(pipeline.id);
            }
            
            for (const auto& reg_id : stage.registers_out) {
                reg_to_stage[reg_id] = global_stage_idx;
                stage_to_regs[global_stage_idx].insert(reg_id);
                stage_to_blocks[global_stage_idx].insert(pipeline.id);
            }
        }
    }
    
    // Create global stages from the aggregated information
    for (const auto& stage_pair : stage_to_regs) {
        int stage_idx = stage_pair.first;
        const auto& reg_set = stage_pair.second;
        
        GlobalPipelineStage global_stage;
        global_stage.stage_index = stage_idx;
        global_stage.domain_id = 0; // This would need to be determined more accurately
        
        // Add registers to this stage
        for (const auto& reg_id : reg_set) {
            global_stage.reg_ids.Add(reg_id);
        }
        
        // Add blocks to this stage
        for (const auto& block_id : stage_to_blocks[stage_idx]) {
            global_stage.block_ids.Add(block_id);
        }
        
        // Calculate depth estimates
        global_stage.max_comb_depth_estimate = 0; // Would be calculated based on actual paths
        global_stage.avg_comb_depth_estimate = 0; // Would be calculated based on actual paths
        
        stages.Add(global_stage);
    }
    
    // Sort stages by index to ensure consistent ordering
    std::sort(stages.begin(), stages.end(), [](const GlobalPipelineStage& a, const GlobalPipelineStage& b) {
        return a.stage_index < b.stage_index;
    });
    
    return Result<Vector<GlobalPipelineStage>>::MakeOk(std::move(stages));
}

void GlobalPipelineAnalysis::UpdateMetrics(GlobalPipelineMap& global_map) {
    // Calculate max total depth across all stages
    for (const auto& stage : global_map.stages) {
        if (stage.max_comb_depth_estimate > global_map.max_total_depth) {
            global_map.max_total_depth = stage.max_comb_depth_estimate;
        }
    }
    
    // Calculate max stages
    for (const auto& stage : global_map.stages) {
        if (stage.stage_index > global_map.max_stages) {
            global_map.max_stages = stage.stage_index;
        }
    }
    if (!global_map.stages.IsEmpty()) {
        global_map.max_stages += 1; // Convert from max index to count
    }
}

} // namespace ProtoVMCLI