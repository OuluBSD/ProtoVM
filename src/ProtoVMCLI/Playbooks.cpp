#include "Playbooks.h"
#include "CircuitFacade.h"
#include "HlsIrInference.h"
#include "IrOptimization.h"
#include "DiffAnalysis.h"
#include "BehavioralAnalysis.h"
#include "Transformations.h"
#include "Codegen.h"
#include "SessionStore.h"
#include "JsonIO.h"
#include <algorithm>
#include <memory>

namespace ProtoVMCLI {

// Helper function to resolve block set based on configuration
static Result<std::vector<std::string>> ResolveBlockSet(
    const PlaybookConfig& config,
    CircuitFacade& circuit_facade,
    const SessionMetadata& session,
    const std::string& session_dir
) {
    std::vector<std::string> block_ids;

    if (!config.block_ids.empty()) {
        // Use explicit list of block IDs
        block_ids = config.block_ids;
    } else if (!config.name_prefix.empty()) {
        // Use name prefix to filter blocks
        auto block_graph_result = circuit_facade.BuildBlockGraphForBranch(
            session,
            session_dir,
            session.current_branch
        );

        if (!block_graph_result.ok) {
            return Result<std::vector<std::string>>::MakeError(
                block_graph_result.error_code,
                "Failed to build block graph for branch: " + block_graph_result.error_message
            );
        }

        const auto& block_graph = block_graph_result.data;
        for (const auto& block : block_graph.blocks) {
            if (block.id.find(config.name_prefix) == 0) {  // starts with prefix
                block_ids.push_back(block.id);
            }
        }
    } else {
        return Result<std::vector<std::string>>::MakeError(
            ErrorCode::CommandParseError,
            "Either block_ids or name_prefix must be specified for system-level playbooks"
        );
    }

    return Result<std::vector<std::string>>::MakeOk(block_ids);
}

// Helper function to run OptimizeBlockAndReport for a single block in a system-level playbook
static Result<BlockPlaybookResult> RunBlockSubPlaybook_OptimizeAndReport(
    const PlaybookConfig& config,
    const std::string& block_id,
    CircuitFacade& circuit_facade,
    const SessionMetadata& session,
    const std::string& session_dir
) {
    BlockPlaybookResult result;
    result.block_id = block_id;

    // Step 1: Get initial analysis
    auto behavior_result = circuit_facade.InferBehaviorForBlockInBranch(
        session,
        session_dir,
        session.current_branch,
        block_id
    );

    if (behavior_result.ok) {
        result.initial_behavior = behavior_result.data;
        result.final_behavior = behavior_result.data; // Same initially for this playbook
    } else {
        // Log warning but continue
        result.initial_behavior = BehaviorDescriptor();
        result.final_behavior = BehaviorDescriptor();
    }

    // Step 2: Get initial IR
    auto ir_result = circuit_facade.BuildIrForBlockInBranch(
        session,
        session_dir,
        session.current_branch,
        block_id
    );

    if (ir_result.ok) {
        result.initial_ir = ir_result.data;
    } else {
        // Use default/empty IR
        result.initial_ir = IrModule();
    }

    result.final_ir = result.initial_ir; // Same for this playbook

    // Step 3: Run IR optimization
    if (!config.passes.empty()) {
        auto opt_result = circuit_facade.OptimizeBlockIrInBranch(
            session,
            session_dir,
            session.current_branch,
            block_id,
            config.passes
        );

        if (opt_result.ok) {
            result.optimization = opt_result.data;
            result.final_ir = opt_result.data.optimized;
        } else {
            // Log error but continue
            result.optimization = IrOptimizationResult();
        }
    }

    // Step 4: Propose refactoring plans (without applying)
    auto plan_result = circuit_facade.ProposeIrBasedTransformationsForBlock(
        session,
        session_dir,
        session.current_branch,
        block_id,
        config.passes
    );

    if (plan_result.ok) {
        result.proposed_plans = plan_result.data;
    } else {
        // Log error but continue
        result.proposed_plans.clear();
    }

    // Step 5: Compute diff vs baseline
    if (!config.baseline_branch.empty()) {
        // Compute behavioral diff
        auto diff_behavior_result = circuit_facade.DiffBlockBehaviorBetweenBranches(
            session,
            session_dir,
            config.baseline_branch,
            session.current_branch,
            block_id
        );

        if (diff_behavior_result.ok) {
            result.behavior_diff = diff_behavior_result.data;
        }

        // Compute IR diff
        auto diff_ir_result = circuit_facade.DiffBlockIrBetweenBranches(
            session,
            session_dir,
            config.baseline_branch,
            session.current_branch,
            block_id
        );

        if (diff_ir_result.ok) {
            result.ir_diff = diff_ir_result.data;
        }
    }

    // Step 6: Codegen for final state
    IrModule codegen_ir = config.use_optimized_ir ? result.final_ir : result.initial_ir;
    auto codegen_result = GenerateCodeForBlock(block_id, codegen_ir);
    if (codegen_result.ok) {
        result.codegen = codegen_result.data;
    } else {
        // Fallback to empty codegen
        result.codegen = CodegenModule();
    }

    // Step 7: Fill in remaining result fields
    result.applied_plan_ids.clear();  // No plans applied in this playbook
    result.new_circuit_revision = -1; // No changes applied

    return Result<BlockPlaybookResult>::MakeOk(result);
}

// Helper function to run OptimizeAndApplySafeRefactors for a single block in a system-level playbook
static Result<BlockPlaybookResult> RunBlockSubPlaybook_OptimizeAndApplySafeRefactors(
    const PlaybookConfig& config,
    const std::string& block_id,
    CircuitFacade& circuit_facade,
    const SessionMetadata& session,
    const std::string& session_dir
) {
    BlockPlaybookResult result;
    result.block_id = block_id;

    // Step 1: Get initial analysis
    auto behavior_result = circuit_facade.InferBehaviorForBlockInBranch(
        session,
        session_dir,
        session.current_branch,
        block_id
    );

    if (behavior_result.ok) {
        result.initial_behavior = behavior_result.data;
        result.final_behavior = behavior_result.data; // Will be updated after changes
    } else {
        result.initial_behavior = BehaviorDescriptor();
        result.final_behavior = BehaviorDescriptor();
    }

    // Step 2: Get initial IR
    auto ir_result = circuit_facade.BuildIrForBlockInBranch(
        session,
        session_dir,
        session.current_branch,
        block_id
    );

    if (ir_result.ok) {
        result.initial_ir = ir_result.data;
    } else {
        result.initial_ir = IrModule();
    }

    result.final_ir = result.initial_ir;

    // Step 3: Run IR optimization
    if (!config.passes.empty()) {
        auto opt_result = circuit_facade.OptimizeBlockIrInBranch(
            session,
            session_dir,
            session.current_branch,
            block_id,
            config.passes
        );

        if (opt_result.ok) {
            result.optimization = opt_result.data;
            result.final_ir = opt_result.data.optimized;
        } else {
            result.optimization = IrOptimizationResult();
        }
    }

    // Step 4: Propose refactoring plans
    auto plan_result = circuit_facade.ProposeIrBasedTransformationsForBlock(
        session,
        session_dir,
        session.current_branch,
        block_id,
        config.passes
    );

    if (plan_result.ok) {
        result.proposed_plans = plan_result.data;

        // Step 5: Behavior/IR verification & application if applicable
        if (config.apply_refactors) {
            for (const auto& plan : plan_result.data) {
                // For now, we'll apply all plans that have behavior preservation guarantees
                // In a real system, we would use more sophisticated verification logic
                bool can_apply = true;

                for (const auto& guarantee : plan.guarantees) {
                    if (guarantee == PreservationLevel::BehaviorKindPreserved ||
                        guarantee == PreservationLevel::IOContractPreserved) {
                        // These are the kinds of guarantees that suggest the refactor is safe
                        continue;
                    } else {
                        // If there are other guarantees we don't know about, be conservative
                        can_apply = false;
                        break;
                    }
                }

                if (can_apply) {
                    // Apply the transformation plan
                    auto apply_result = circuit_facade.ApplyTransformationPlan(
                        const_cast<SessionMetadata&>(session), // Need to modify session
                        session_dir,
                        session.current_branch,
                        plan,
                        config.designer_session_id  // user_id for logging
                    );

                    if (apply_result.ok) {
                        result.applied_plan_ids.push_back(plan.id);

                        // Update session with new revision
                        result.new_circuit_revision = session.circuit_revision + 1; // This is approximate

                        // Refresh block state after application
                        auto updated_behavior = circuit_facade.InferBehaviorForBlockInBranch(
                            session,
                            session_dir,
                            session.current_branch,
                            block_id
                        );

                        if (updated_behavior.ok) {
                            result.final_behavior = updated_behavior.data;
                        }

                        auto updated_ir = circuit_facade.BuildIrForBlockInBranch(
                            session,
                            session_dir,
                            session.current_branch,
                            block_id
                        );

                        if (updated_ir.ok) {
                            result.final_ir = updated_ir.data;
                        }
                    }
                }
            }
        }
    }

    // Step 6: Compute diff vs baseline
    if (!config.baseline_branch.empty()) {
        // Compute behavioral diff
        auto diff_behavior_result = circuit_facade.DiffBlockBehaviorBetweenBranches(
            session,
            session_dir,
            config.baseline_branch,
            session.current_branch,
            block_id
        );

        if (diff_behavior_result.ok) {
            result.behavior_diff = diff_behavior_result.data;
        }

        // Compute IR diff
        auto diff_ir_result = circuit_facade.DiffBlockIrBetweenBranches(
            session,
            session_dir,
            config.baseline_branch,
            session.current_branch,
            block_id
        );

        if (diff_ir_result.ok) {
            result.ir_diff = diff_ir_result.data;
        }
    }

    // Step 7: Codegen for final state
    IrModule codegen_ir = config.use_optimized_ir ? result.final_ir : result.initial_ir;
    auto codegen_result = GenerateCodeForBlock(block_id, codegen_ir);
    if (codegen_result.ok) {
        result.codegen = codegen_result.data;
    } else {
        result.codegen = CodegenModule();
    }

    return Result<BlockPlaybookResult>::MakeOk(result);
}

// Helper function to infer code generation for a block
static Result<CodegenModule> GenerateCodeForBlock(
    const std::string& block_id,
    const IrModule& ir_module,
    const std::string& flavor = "PseudoVerilog"
) {
    CodegenModule result;
    result.id = block_id;
    result.name = block_id + "_" + flavor;
    result.flavor = flavor;

    // For now, we'll generate a simple pseudo-verilog representation
    std::string code = "module " + result.name + " (\n";

    // Add inputs
    for (size_t i = 0; i < ir_module.inputs.size(); ++i) {
        const auto& input = ir_module.inputs[i];
        code += "  input [" + std::to_string(input.bit_width - 1) + ":0] " + input.name;
        if (i < ir_module.inputs.size() - 1) code += ",";
        code += "\n";
    }

    // Add outputs
    for (size_t i = 0; i < ir_module.outputs.size(); ++i) {
        const auto& output = ir_module.outputs[i];
        code += "  output [" + std::to_string(output.bit_width - 1) + ":0] " + output.name;
        if (i < ir_module.outputs.size() - 1) code += ",";
        code += "\n";
    }

    code += ");\n\n";

    // Add combinational assignments
    for (const auto& assign : ir_module.comb_assigns) {
        code += "  assign " + assign.target.name + " = ";
        if (assign.args.size() == 1) {
            code += assign.args[0].name;
        } else if (assign.args.size() == 2) {
            switch (assign.kind) {
                case IrExprKind::And:
                    code += assign.args[0].name + " & " + assign.args[1].name;
                    break;
                case IrExprKind::Or:
                    code += assign.args[0].name + " | " + assign.args[1].name;
                    break;
                case IrExprKind::Xor:
                    code += assign.args[0].name + " ^ " + assign.args[1].name;
                    break;
                case IrExprKind::Add:
                    code += assign.args[0].name + " + " + assign.args[1].name;
                    break;
                case IrExprKind::Sub:
                    code += assign.args[0].name + " - " + assign.args[1].name;
                    break;
                case IrExprKind::Eq:
                    code += assign.args[0].name + " == " + assign.args[1].name;
                    break;
                case IrExprKind::Neq:
                    code += assign.args[0].name + " != " + assign.args[1].name;
                    break;
                default:
                    code += assign.args[0].name + " /* op " + std::to_string(static_cast<int>(assign.kind)) + " */ " + assign.args[1].name;
                    break;
            }
        } else if (assign.args.size() == 3 && assign.kind == IrExprKind::Mux) {
            // Ternary operation: target = sel ? a : b
            code += assign.args[1].name + " ? " + assign.args[0].name + " : " + assign.args[2].name;
        } else {
            // For other expressions or unknown operations
            code += "/* unimplemented expr */";
        }
        code += ";\n";
    }

    code += "endmodule\n";

    result.code = code;

    return Result<CodegenModule>::MakeOk(result);
}

Result<PlaybookResult> PlaybookEngine::RunPlaybook(
    const PlaybookConfig& config,
    CoDesignerManager& designer_manager,
    SessionStore& session_store,
    const std::string& workspace_dir
) {
    // Initialize the result structure
    PlaybookResult result;
    result.kind = config.kind;
    result.config = config;
    
    // Get the designer session state
    auto session_result = designer_manager.GetSession(config.designer_session_id);
    if (!session_result.ok) {
        return Result<PlaybookResult>::MakeError(session_result.error_code, session_result.error_message);
    }
    
    CoDesignerSessionState session = session_result.data;
    result.designer_session = session;
    
    // Validate configuration based on target type
    if (config.target == "block") {
        if (config.block_id.empty()) {
            return Result<PlaybookResult>::MakeError(
                ErrorCode::CommandParseError,
                "block_id is required when target is 'block'"
            );
        }

        // Update the session's current block ID if needed
        if (session.current_block_id != config.block_id) {
            session.current_block_id = config.block_id;
            designer_manager.UpdateSession(session);
            result.designer_session = session;
        }
    } else if (config.target == "system") {
        // For system-level playbooks, we don't set a specific block ID
        // Validation happens later in the specific playbook flows
    } else {
        return Result<PlaybookResult>::MakeError(
            ErrorCode::CommandParseError,
            "target must be either 'block' or 'system'"
        );
    }
    
    // Get the circuit facade from the session store
    auto circuit_facade = std::make_shared<CircuitFacade>(session_store, workspace_dir);

    // Get session metadata for use with circuit facade methods
    auto session_metadata_result = session_store.LoadSession(session.proto_session_id);
    if (!session_metadata_result.ok) {
        return Result<PlaybookResult>::MakeError(session_metadata_result.error_code, session_metadata_result.error_message);
    }
    SessionMetadata session_metadata = session_metadata_result.data;
    std::string session_dir = workspace_dir + "/sessions/" + std::to_string(session.proto_session_id);

    // Execute the appropriate playbook flow based on kind
    switch (config.kind) {
        case PlaybookKind::OptimizeBlockAndReport: {
            // Step 1: Get initial analysis
            auto behavior_result = circuit_facade->InferBehaviorForBlockInBranch(
                session_metadata,
                session_dir,
                session.branch,
                config.block_id
            );

            if (behavior_result.ok) {
                result.initial_behavior = behavior_result.data;
                result.final_behavior = behavior_result.data; // Same initially for this playbook
            } else {
                // Log warning but continue
                result.initial_behavior = BehaviorDescriptor();
                result.final_behavior = BehaviorDescriptor();
            }

            // Step 2: Get initial IR
            auto ir_result = circuit_facade->BuildIrForBlockInBranch(
                session_metadata,
                session_dir,
                session.branch,
                config.block_id
            );

            if (ir_result.ok) {
                result.initial_ir = ir_result.data;
            } else {
                // Use default/empty IR
                result.initial_ir = IrModule();
            }

            result.final_ir = result.initial_ir; // Same for this playbook

            // Step 3: Run IR optimization
            if (!config.passes.empty()) {
                auto opt_result = circuit_facade->OptimizeBlockIrInBranch(
                    session_metadata,
                    session_dir,
                    session.branch,
                    config.block_id,
                    config.passes
                );

                if (opt_result.ok) {
                    result.optimization = opt_result.data;
                    result.final_ir = opt_result.data.optimized;
                } else {
                    // Log error but continue
                    result.optimization = IrOptimizationResult();
                }
            }

            // Step 4: Propose refactoring plans (without applying)
            auto plan_result = circuit_facade->ProposeIrBasedTransformationsForBlock(
                session_metadata,
                session_dir,
                session.branch,
                config.block_id,
                config.passes
            );

            if (plan_result.ok) {
                result.proposed_plans = plan_result.data;
            } else {
                // Log error but continue
                result.proposed_plans.clear();
            }

            // Step 5: Compute diff vs baseline
            if (!config.baseline_branch.empty()) {
                // Compute behavioral diff
                auto diff_behavior_result = circuit_facade->DiffBlockBehaviorBetweenBranches(
                    session_metadata,
                    session_dir,
                    config.baseline_branch,
                    session.branch,
                    config.block_id
                );

                if (diff_behavior_result.ok) {
                    result.behavior_diff = diff_behavior_result.data;
                }

                // Compute IR diff
                auto diff_ir_result = circuit_facade->DiffBlockIrBetweenBranches(
                    session_metadata,
                    session_dir,
                    config.baseline_branch,
                    session.branch,
                    config.block_id
                );

                if (diff_ir_result.ok) {
                    result.ir_diff = diff_ir_result.data;
                }
            }

            // Step 6: Codegen for final state
            IrModule codegen_ir = config.use_optimized_ir ? result.final_ir : result.initial_ir;
            auto codegen_result = GenerateCodeForBlock(config.block_id, codegen_ir);
            if (codegen_result.ok) {
                result.codegen = codegen_result.data;
            } else {
                // Fallback to empty codegen
                result.codegen = CodegenModule();
            }

            // Step 7: Fill in remaining result fields
            result.applied_plan_ids.clear();  // No plans applied in this playbook
            result.new_circuit_revision = -1; // No changes applied

            break;
        }

        case PlaybookKind::SystemOptimizeAndReport: {
            // System-level: run OptimizeAndReport across multiple blocks

            // Resolve the block set based on configuration
            auto block_set_result = ResolveBlockSet(config, *circuit_facade, session_metadata, session_dir);
            if (!block_set_result.ok) {
                return Result<PlaybookResult>::MakeError(block_set_result.error_code, block_set_result.error_message);
            }

            std::vector<std::string> block_ids = block_set_result.data;
            result.total_blocks = static_cast<int>(block_ids.size());
            result.blocks_with_changes = 0;
            result.total_applied_plans = 0;

            // Process each block in the resolved set
            for (const std::string& block_id : block_ids) {
                auto block_result = RunBlockSubPlaybook_OptimizeAndReport(
                    config,
                    block_id,
                    *circuit_facade,
                    session_metadata,
                    session_dir
                );

                if (block_result.ok) {
                    result.system_block_results.push_back(block_result.data);

                    // Check if this block has changes
                    bool has_changes = !block_result.data.proposed_plans.empty() ||
                                      (!block_result.data.behavior_diff.changes.empty()) ||
                                      (!block_result.data.ir_diff.changes.empty());

                    if (has_changes) {
                        result.blocks_with_changes++;
                    }
                } else {
                    // For now, we'll continue processing other blocks even if one fails
                    // In a real implementation, we might want to handle this differently
                }
            }

            break;
        }

        case PlaybookKind::SystemOptimizeAndApplySafeRefactors: {
            // System-level: run OptimizeAndApplySafeRefactors across multiple blocks

            // Resolve the block set based on configuration
            auto block_set_result = ResolveBlockSet(config, *circuit_facade, session_metadata, session_dir);
            if (!block_set_result.ok) {
                return Result<PlaybookResult>::MakeError(block_set_result.error_code, block_set_result.error_message);
            }

            std::vector<std::string> block_ids = block_set_result.data;
            result.total_blocks = static_cast<int>(block_ids.size());
            result.blocks_with_changes = 0;
            result.total_applied_plans = 0;

            // Process each block in the resolved set
            for (const std::string& block_id : block_ids) {
                auto block_result = RunBlockSubPlaybook_OptimizeAndApplySafeRefactors(
                    config,
                    block_id,
                    *circuit_facade,
                    session_metadata,
                    session_dir
                );

                if (block_result.ok) {
                    result.system_block_results.push_back(block_result.data);

                    // Check if this block has changes
                    bool has_changes = !block_result.data.applied_plan_ids.empty() ||
                                      (!block_result.data.behavior_diff.changes.empty()) ||
                                      (!block_result.data.ir_diff.changes.empty());

                    if (has_changes) {
                        result.blocks_with_changes++;
                    }

                    // Update total applied plans
                    result.total_applied_plans += static_cast<int>(block_result.data.applied_plan_ids.size());
                } else {
                    // For now, we'll continue processing other blocks even if one fails
                    // In a real implementation, we might want to handle this differently
                }
            }

            break;
        }
        
        default:
            return Result<PlaybookResult>::MakeError(
                ErrorCode::CommandParseError,
                "Unsupported playbook kind: " + std::to_string(static_cast<int>(config.kind))
            );
    }
    
    return Result<PlaybookResult>::MakeOk(result);
}

} // namespace ProtoVMCLI