#include "PipelineAnalysis.h"
#include "CircuitGraphQueries.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <map>

namespace ProtoVMCLI {

Result<PipelineMap> PipelineAnalysis::BuildPipelineMapForBlock(
    const CircuitGraph& graph,
    const TimingAnalysis* timing,
    const ScheduledModule* scheduled_ir,
    const std::string& block_id
) {
    try {
        PipelineMap pipeline_map;
        pipeline_map.id = block_id;

        // Discover clock domains
        auto clock_domains_result = DiscoverClockDomains(graph, block_id);
        if (!clock_domains_result.ok) {
            return Result<PipelineMap>::Error(clock_domains_result.error_message, clock_domains_result.error_code);
        }
        pipeline_map.clock_domains = std::move(clock_domains_result.data);

        // Discover registers and map them to domains
        auto registers_result = DiscoverRegisters(graph, pipeline_map.clock_domains, block_id);
        if (!registers_result.ok) {
            return Result<PipelineMap>::Error(registers_result.error_message, registers_result.error_code);
        }
        pipeline_map.registers = std::move(registers_result.data);

        // Discover pipeline stages
        auto stages_result = DiscoverPipelineStages(graph, scheduled_ir, pipeline_map.registers, block_id);
        if (!stages_result.ok) {
            return Result<PipelineMap>::Error(stages_result.error_message, stages_result.error_code);
        }
        pipeline_map.stages = std::move(stages_result.data);

        // Discover reg-to-reg paths
        auto paths_result = DiscoverRegToRegPaths(graph, timing, pipeline_map.registers, pipeline_map.stages, block_id);
        if (!paths_result.ok) {
            return Result<PipelineMap>::Error(paths_result.error_message, paths_result.error_code);
        }
        pipeline_map.reg_paths = std::move(paths_result.data);

        return Result<PipelineMap>::Success(std::move(pipeline_map));
    } catch (const std::exception& e) {
        return Result<PipelineMap>::Error("Failed to build pipeline map for block: " + std::string(e.what()), "PIPELINE_ANALYSIS_ERROR");
    }
}

Result<PipelineMap> PipelineAnalysis::BuildPipelineMapForSubsystem(
    const CircuitGraph& graph,
    const TimingAnalysis* timing,
    const ScheduledModule* scheduled_ir,
    const std::string& subsystem_id,
    const std::vector<std::string>& block_ids
) {
    try {
        PipelineMap pipeline_map;
        pipeline_map.id = subsystem_id;

        // For subsystem, we combine pipeline information from multiple blocks
        // First, discover clock domains across all relevant blocks
        auto clock_domains_result = DiscoverClockDomains(graph, subsystem_id);
        if (!clock_domains_result.ok) {
            return Result<PipelineMap>::Error(clock_domains_result.error_message, clock_domains_result.error_code);
        }
        pipeline_map.clock_domains = std::move(clock_domains_result.data);

        // Discover registers across all blocks
        std::vector<RegisterInfo> all_registers;
        for (const auto& block_id : block_ids) {
            auto registers_result = DiscoverRegisters(graph, pipeline_map.clock_domains, block_id);
            if (!registers_result.ok) {
                return Result<PipelineMap>::Error(registers_result.error_message, registers_result.error_code);
            }
            all_registers.insert(all_registers.end(), 
                                registers_result.data.begin(), 
                                registers_result.data.end());
        }
        pipeline_map.registers = std::move(all_registers);

        // Discover pipeline stages across all blocks
        auto stages_result = DiscoverPipelineStages(graph, scheduled_ir, pipeline_map.registers, subsystem_id);
        if (!stages_result.ok) {
            return Result<PipelineMap>::Error(stages_result.error_message, stages_result.error_code);
        }
        pipeline_map.stages = std::move(stages_result.data);

        // Discover reg-to-reg paths across all blocks
        auto paths_result = DiscoverRegToRegPaths(graph, timing, pipeline_map.registers, pipeline_map.stages, subsystem_id);
        if (!paths_result.ok) {
            return Result<PipelineMap>::Error(paths_result.error_message, paths_result.error_code);
        }
        pipeline_map.reg_paths = std::move(paths_result.data);

        return Result<PipelineMap>::Success(std::move(pipeline_map));
    } catch (const std::exception& e) {
        return Result<PipelineMap>::Error("Failed to build pipeline map for subsystem: " + std::string(e.what()), "PIPELINE_ANALYSIS_ERROR");
    }
}

Result<std::vector<ClockSignalInfo>> PipelineAnalysis::DiscoverClockDomains(
    const CircuitGraph& graph,
    const std::string& target_id
) {
    try {
        std::vector<ClockSignalInfo> clock_domains;
        std::unordered_map<std::string, int> domain_map;  // clock signal name -> domain_id
        int next_domain_id = 0;

        // For now, we'll use CircuitGraphQueries to find potential clock signals
        // This is a simplified heuristic for demonstration
        CircuitGraphQueries queries(graph);

        // Find nodes that look like clock signals (heuristic approach)
        for (const auto& node_id : graph.nodes) {
            // In a real implementation, we'd search for clock signals in the circuit
            // For now, we'll just use component names that end in "CLK" or "clk" as clock signals
            if (node_id.kind == GraphNodeKind::Net) {
                size_t last_colon = node_id.id.find_last_of(':');
                std::string net_name = (last_colon == std::string::npos) ? node_id.id : node_id.id.substr(last_colon + 1);
                
                // Look for common clock signal patterns
                if (net_name.find("CLK") != std::string::npos || 
                    net_name.find("clk") != std::string::npos ||
                    net_name.find("CLOCK") != std::string::npos) {
                    
                    // Add to domains if not already seen
                    if (domain_map.find(node_id.id) == domain_map.end()) {
                        domain_map[node_id.id] = next_domain_id;
                        
                        ClockSignalInfo info;
                        info.signal_name = node_id.id;
                        info.domain_id = next_domain_id;
                        clock_domains.push_back(info);
                        
                        next_domain_id++;
                    }
                }
            }
        }

        return Result<std::vector<ClockSignalInfo>>::Success(std::move(clock_domains));
    } catch (const std::exception& e) {
        return Result<std::vector<ClockSignalInfo>>::Error("Failed to discover clock domains: " + std::string(e.what()), "CLOCK_DOMAIN_DISCOVERY_ERROR");
    }
}

Result<std::vector<RegisterInfo>> PipelineAnalysis::DiscoverRegisters(
    const CircuitGraph& graph,
    const std::vector<ClockSignalInfo>& clock_domains,
    const std::string& target_id
) {
    try {
        std::vector<RegisterInfo> registers;
        std::unordered_map<std::string, int> clock_domain_map;  // clock signal name -> domain_id

        // Build domain mapping for quick lookup
        for (const auto& domain : clock_domains) {
            clock_domain_map[domain.signal_name] = domain.domain_id;
        }

        // This is a simplified implementation to find register-like components
        // In a real implementation, we'd look for flip-flops, latches, etc.
        for (const auto& node_id : graph.nodes) {
            std::string node_type = node_id.id;  // This will be a component ID in practice
            size_t colon_pos = node_id.id.find(':');
            std::string component_type = (colon_pos != std::string::npos) ? 
                                         node_id.id.substr(0, colon_pos) : node_id.id;

            // Look for common register component types (simplified heuristic)
            if (component_type.find("FF") != std::string::npos ||      // Flip-flop
                component_type.find("REG") != std::string::npos ||     // Register
                component_type.find("LATCH") != std::string::npos ||   // Latch
                component_type.find("DFF") != std::string::npos) {     // D Flip-flop

                // Try to determine which clock signal drives this register
                std::string clock_signal = "unknown";  // Default to unknown
                int domain_id = -1;  // Default to unknown domain

                // In a real implementation, we'd trace from the register's clock pin to its net
                // For now, we'll just create a dummy register entry
                RegisterInfo reg_info;
                reg_info.reg_id = node_id.id;  // Use node ID as reg ID
                reg_info.name = node_id.id;    // Use node ID as name for now
                reg_info.clock_signal = clock_signal;
                reg_info.domain_id = domain_id;
                reg_info.reset_signal = "";    // No reset signal by default

                registers.push_back(reg_info);
            }
        }

        // In a real implementation, we would need to connect registers to their clock signals
        // by analyzing the circuit graph for connections from clock nets to register clock pins

        return Result<std::vector<RegisterInfo>>::Success(std::move(registers));
    } catch (const std::exception& e) {
        return Result<std::vector<RegisterInfo>>::Error("Failed to discover registers: " + std::string(e.what()), "REGISTER_DISCOVERY_ERROR");
    }
}

Result<std::vector<PipelineStageInfo>> PipelineAnalysis::DiscoverPipelineStages(
    const CircuitGraph& graph,
    const ScheduledModule* scheduled_ir,
    const std::vector<RegisterInfo>& registers,
    const std::string& target_id
) {
    try {
        std::vector<PipelineStageInfo> stages;
        
        // If scheduled IR is available, use that to determine stages
        if (scheduled_ir != nullptr) {
            // Use the scheduled module to determine pipeline stages
            for (int stage_idx = 0; stage_idx < scheduled_ir->num_stages; stage_idx++) {
                PipelineStageInfo stage_info;
                stage_info.stage_index = stage_idx;
                stage_info.domain_id = 0;  // Default domain for now
                stage_info.comb_depth_estimate = 0;  // Will be calculated

                // Find registers that are scheduled in this stage
                std::unordered_set<std::string> stage_reg_ids;
                for (const auto& reg_op : scheduled_ir->reg_ops) {
                    if (reg_op.stage == stage_idx) {
                        // Add the target register to output regs for this stage
                        stage_reg_ids.insert(reg_op.reg_assign.target.name);
                        stage_info.registers_out.push_back(reg_op.reg_assign.target.name);
                    }
                }

                // Find comb ops in this stage to estimate depth
                int depth_estimate = 0;
                for (const auto& comb_op : scheduled_ir->comb_ops) {
                    if (comb_op.stage == stage_idx) {
                        // Simplified depth estimation - count operations
                        depth_estimate++;
                    }
                }
                stage_info.comb_depth_estimate = depth_estimate;

                stages.push_back(stage_info);
            }
        } else {
            // If no scheduled IR is available, use circuit graph analysis to estimate stages
            // This is a simplified heuristic for now
            if (!registers.empty()) {
                PipelineStageInfo stage_info;
                stage_info.stage_index = 0;
                stage_info.domain_id = 0;
                stage_info.comb_depth_estimate = 1;

                for (const auto& reg : registers) {
                    stage_info.registers_out.push_back(reg.reg_id);
                }

                stages.push_back(stage_info);
            }
        }

        return Result<std::vector<PipelineStageInfo>>::Success(std::move(stages));
    } catch (const std::exception& e) {
        return Result<std::vector<PipelineStageInfo>>::Error("Failed to discover pipeline stages: " + std::string(e.what()), "PIPELINE_STAGE_DISCOVERY_ERROR");
    }
}

Result<std::vector<RegToRegPathInfo>> PipelineAnalysis::DiscoverRegToRegPaths(
    const CircuitGraph& graph,
    const TimingAnalysis* timing,
    const std::vector<RegisterInfo>& registers,
    const std::vector<PipelineStageInfo>& stages,
    const std::string& target_id
) {
    try {
        std::vector<RegToRegPathInfo> paths;

        // Map register IDs to their stage indices for quick lookup
        std::unordered_map<std::string, int> reg_to_stage;
        for (size_t i = 0; i < stages.size(); i++) {
            for (const auto& reg_id : stages[i].registers_out) {
                reg_to_stage[reg_id] = static_cast<int>(i);
            }
        }

        // Create all possible register-to-register paths
        for (size_t i = 0; i < registers.size(); i++) {
            for (size_t j = 0; j < registers.size(); j++) {
                if (i != j) {  // Don't create self-paths
                    RegToRegPathInfo path_info;
                    path_info.src_reg_id = registers[i].reg_id;
                    path_info.dst_reg_id = registers[j].reg_id;

                    // Determine if they are in the same domain
                    bool same_domain = (registers[i].domain_id == registers[j].domain_id) && 
                                      (registers[i].domain_id != -1);

                    path_info.domain_id = same_domain ? registers[i].domain_id : -1;
                    path_info.crosses_clock_domain = !same_domain;

                    // Determine stage span (difference in stage indices)
                    int src_stage = reg_to_stage.count(path_info.src_reg_id) ? 
                                   reg_to_stage[path_info.src_reg_id] : -1;
                    int dst_stage = reg_to_stage.count(path_info.dst_reg_id) ? 
                                   reg_to_stage[path_info.dst_reg_id] : -1;

                    path_info.stage_span = (src_stage != -1 && dst_stage != -1) ? 
                                          (dst_stage - src_stage) : 0;

                    // Default depth estimate (will be calculated more accurately in real implementation)
                    path_info.comb_depth_estimate = 1;

                    paths.push_back(path_info);
                }
            }
        }

        return Result<std::vector<RegToRegPathInfo>>::Success(std::move(paths));
    } catch (const std::exception& e) {
        return Result<std::vector<RegToRegPathInfo>>::Error("Failed to discover reg-to-reg paths: " + std::string(e.what()), "REG_TO_REG_PATH_DISCOVERY_ERROR");
    }
}

} // namespace ProtoVMCLI