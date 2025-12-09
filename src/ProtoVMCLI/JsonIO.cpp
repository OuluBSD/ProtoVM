#include "JsonIO.h"
#include "Transformations.h"
#include "DiffAnalysis.h"
#include "ScheduledIr.h"
#include "Scheduling.h"
#include "CdcModel.h"
#include <iostream>
#include <sstream>

namespace ProtoVMCLI {

Upp::String JsonIO::SuccessResponse(const std::string& command, const Upp::ValueMap& data) {
    Upp::ValueMap response;
    response.Add("ok", true);
    response.Add("command", Upp::String(command.c_str()));
    response.Add("error_code", Upp::Value());
    response.Add("error", Upp::Value());
    if (!data.IsEmpty()) {
        response.Add("data", data);
    } else {
        response.Add("data", Upp::Value());
    }
    return ValueMapToJson(response);
}

Upp::String JsonIO::ErrorResponse(const std::string& command, const std::string& error_msg, const std::string& error_code) {
    Upp::ValueMap response;
    response.Add("ok", false);
    response.Add("command", Upp::String(command.c_str()));
    if (!error_code.empty()) {
        response.Add("error_code", Upp::String(error_code.c_str()));
    } else {
        response.Add("error_code", Upp::Value());
    }
    response.Add("error", Upp::String(error_msg.c_str()));
    response.Add("data", Upp::Value());
    return ValueMapToJson(response);
}

template<typename T>
Upp::String JsonIO::FromResult(const std::string& command, const Result<T>& result,
                               std::function<Upp::ValueMap(const T&)> converter) {
    if (result.ok) {
        Upp::ValueMap data;
        if (converter) {
            data = converter(result.data);
        } else {
            // For simple types, just return an empty object or add a value field
            data.Add("value", Upp::Value(result.data));
        }
        return SuccessResponse(command, data);
    } else {
        std::string code_str = ErrorCodeToString(result.error_code);
        return ErrorResponse(command, result.error_message, code_str);
    }
}

std::string JsonIO::ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::None:
            return "NONE";
        case ErrorCode::WorkspaceNotFound:
            return "WORKSPACE_NOT_FOUND";
        case ErrorCode::InvalidWorkspace:
            return "INVALID_WORKSPACE";
        case ErrorCode::WorkspaceCorrupt:
            return "WORKSPACE_CORRUPT";
        case ErrorCode::SessionNotFound:
            return "SESSION_NOT_FOUND";
        case ErrorCode::SessionCorrupt:
            return "SESSION_CORRUPT";
        case ErrorCode::SessionDeleted:
            return "SESSION_DELETED";
        case ErrorCode::SessionIdConflict:
            return "SESSION_ID_CONFLICT";
        case ErrorCode::CircuitFileNotFound:
            return "CIRCUIT_FILE_NOT_FOUND";
        case ErrorCode::CircuitFileUnreadable:
            return "CIRCUIT_FILE_UNREADABLE";
        case ErrorCode::StorageIoError:
            return "STORAGE_IO_ERROR";
        case ErrorCode::StorageSchemaMismatch:
            return "STORAGE_SCHEMA_MISMATCH";
        case ErrorCode::CommandParseError:
            return "COMMAND_PARSE_ERROR";
        case ErrorCode::InternalError:
            return "INTERNAL_ERROR";
        default:
            return "UNKNOWN_ERROR";
    }
}

ErrorCode JsonIO::StringToErrorCode(const std::string& s) {
    if (s == "NONE") return ErrorCode::None;
    if (s == "WORKSPACE_NOT_FOUND") return ErrorCode::WorkspaceNotFound;
    if (s == "INVALID_WORKSPACE") return ErrorCode::InvalidWorkspace;
    if (s == "WORKSPACE_CORRUPT") return ErrorCode::WorkspaceCorrupt;
    if (s == "SESSION_NOT_FOUND") return ErrorCode::SessionNotFound;
    if (s == "SESSION_CORRUPT") return ErrorCode::SessionCorrupt;
    if (s == "SESSION_DELETED") return ErrorCode::SessionDeleted;
    if (s == "SESSION_ID_CONFLICT") return ErrorCode::SessionIdConflict;
    if (s == "CIRCUIT_FILE_NOT_FOUND") return ErrorCode::CircuitFileNotFound;
    if (s == "CIRCUIT_FILE_UNREADABLE") return ErrorCode::CircuitFileUnreadable;
    if (s == "STORAGE_IO_ERROR") return ErrorCode::StorageIoError;
    if (s == "STORAGE_SCHEMA_MISMATCH") return ErrorCode::StorageSchemaMismatch;
    if (s == "COMMAND_PARSE_ERROR") return ErrorCode::CommandParseError;
    if (s == "INTERNAL_ERROR") return ErrorCode::InternalError;
    return ErrorCode::InternalError;  // default for unknown error codes
}

Upp::ValueMap JsonIO::ParseArgs(int argc, char** argv) {
    Upp::ValueMap args;

    for (int i = 1; i < argc; i++) {
        Upp::String arg = argv[i];

        if (arg.StartsWith("--")) {
            Upp::String key = arg.Mid(2);
            Upp::String value = "";

            if (i + 1 < argc && argv[i + 1][0] != '-') {
                value = argv[++i];
            }

            args.Add(key, value);
        }
        else if (arg[0] == '-') {
            // Short form options
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                Upp::String key = arg.Mid(1);
                Upp::String value = argv[++i];
                args.Add(key, value);
            }
        }
        else {
            // Positional argument - handle command hierarchy
            if (!args.IsKey("command")) {
                args.Add("command", arg);
            } else if (args.Get("command", Upp::String("")) == "debug") {
                // Handle debug subcommands
                if (!args.IsKey("subcommand")) {
                    args.Add("subcommand", arg);
                } else if (args.Get("subcommand", Upp::String("")) == "process" ||
                          args.Get("subcommand", Upp::String("")) == "websocket" ||
                          args.Get("subcommand", Upp::String("")) == "poll") {
                    if (!args.IsKey("action")) {
                        args.Add("action", arg);
                    }
                }
            } else {
                // Store additional positional arguments if needed in the future
                args.Add("extra_" + Upp::AsString(i), arg);
            }
        }
    }

    return args;
}

Upp::String JsonIO::Serialize(const Upp::ValueMap& obj) {
    return ValueMapToJson(obj);
}

Upp::ValueMap JsonIO::Deserialize(const Upp::String& str) {
    // This is a simplified implementation that would need a full JSON parser in real use
    Upp::ValueMap empty_map;
    return empty_map; // Return empty for now
}

Upp::String JsonIO::ValueMapToJson(const Upp::ValueMap& vm) {
    Upp::String result = "{";
    bool first = true;
    
    for (int i = 0; i < vm.GetCount(); i++) {
        if (!first) {
            result += ",";
        }
        result += "\"" + vm.GetKey(i) + "\":" + ValueToJson(vm[i]);
        first = false;
    }
    
    result += "}";
    return result;
}

Upp::String JsonIO::ValueArrayToJson(const Upp::ValueArray& va) {
    Upp::String result = "[";
    bool first = true;
    
    for (int i = 0; i < va.GetCount(); i++) {
        if (!first) {
            result += ",";
        }
        result += ValueToJson(va[i]);
        first = false;
    }
    
    result += "]";
    return result;
}

Upp::String JsonIO::ValueToJson(const Upp::Value& val) {
    if (val.Is<int>()) {
        return Upp::AsString(val.Get<int>());
    } else if (val.Is<bool>()) {
        return val.Get<bool>() ? Upp::String("true") : Upp::String("false");
    } else if (val.Is<double>()) {
        return Upp::AsString(val.Get<double>());
    } else if (val.Is<Upp::String>()) {
        Upp::String str = val.Get<Upp::String>();
        // Escape quotes and other special characters
        str = Upp::String().Cat() << "\"" << str.Replace("\"", "\\\"") << "\"";
        return str;
    } else if (val.Is<Upp::ValueMap>()) {
        return ValueMapToJson(val.Get<Upp::ValueMap>());
    } else if (val.Is<Upp::ValueArray>()) {
        return ValueArrayToJson(val.Get<Upp::ValueArray>());
    } else {
        // Default to string representation
        return Upp::String().Cat() << "\"" << Upp::AsString(val) << "\"";
    }
}

Upp::Value JsonIO::DiagnosticSeverityToJson(DiagnosticSeverity severity) {
    return Upp::String(DiagnosticSeverityToString(severity).c_str());
}

Upp::Value JsonIO::DiagnosticKindToJson(DiagnosticKind kind) {
    return Upp::String(DiagnosticKindToString(kind).c_str());
}

Upp::ValueMap JsonIO::CircuitDiagnosticLocationToValueMap(const CircuitDiagnosticLocation& location) {
    Upp::ValueMap location_map;
    if (!location.component_id.empty()) {
        location_map.Add("component_id", Upp::String(location.component_id.c_str()));
    } else {
        location_map.Add("component_id", Upp::Value());
    }
    if (!location.wire_id.empty()) {
        location_map.Add("wire_id", Upp::String(location.wire_id.c_str()));
    } else {
        location_map.Add("wire_id", Upp::Value());
    }
    if (!location.pin_name.empty()) {
        location_map.Add("pin_name", Upp::String(location.pin_name.c_str()));
    } else {
        location_map.Add("pin_name", Upp::Value());
    }
    return location_map;
}

Upp::ValueMap JsonIO::CircuitDiagnosticToValueMap(const CircuitDiagnostic& diagnostic) {
    Upp::ValueMap diagnostic_map;
    diagnostic_map.Add("severity", DiagnosticSeverityToJson(diagnostic.severity));
    diagnostic_map.Add("kind", DiagnosticKindToJson(diagnostic.kind));
    diagnostic_map.Add("location", CircuitDiagnosticLocationToValueMap(diagnostic.location));
    diagnostic_map.Add("message", Upp::String(diagnostic.message.c_str()));
    if (!diagnostic.suggested_fix.empty()) {
        diagnostic_map.Add("suggested_fix", Upp::String(diagnostic.suggested_fix.c_str()));
    } else {
        diagnostic_map.Add("suggested_fix", Upp::Value());
    }
    return diagnostic_map;
}

Upp::ValueArray JsonIO::CircuitDiagnosticsToValueArray(const std::vector<CircuitDiagnostic>& diagnostics) {
    Upp::ValueArray diagnostics_array;
    for (const auto& diagnostic : diagnostics) {
        diagnostics_array.Add(CircuitDiagnosticToValueMap(diagnostic));
    }
    return diagnostics_array;
}

Upp::ValueMap JsonIO::TimingNodeIdToValueMap(const TimingNodeId& node_id) {
    Upp::ValueMap node_map;
    node_map.Add("id", Upp::String(node_id.id.c_str()));
    return node_map;
}

Upp::ValueMap JsonIO::TimingPathPointToValueMap(const TimingPathPoint& point) {
    Upp::ValueMap point_map;
    point_map.Add("node", TimingNodeIdToValueMap(point.node));
    point_map.Add("depth", point.depth);
    return point_map;
}

Upp::ValueMap JsonIO::TimingPathToValueMap(const TimingPath& path) {
    Upp::ValueMap path_map;
    Upp::ValueArray points_array;
    for (const auto& point : path.points) {
        points_array.Add(TimingPathPointToValueMap(point));
    }
    path_map.Add("points", points_array);
    path_map.Add("total_depth", path.total_depth);
    return path_map;
}

Upp::ValueMap JsonIO::TimingSummaryToValueMap(const TimingSummary& summary) {
    Upp::ValueMap summary_map;
    summary_map.Add("max_depth", summary.max_depth);
    summary_map.Add("path_count", summary.path_count);
    return summary_map;
}

Upp::ValueArray JsonIO::TimingPathsToValueArray(const std::vector<TimingPath>& paths) {
    Upp::ValueArray paths_array;
    for (const auto& path : paths) {
        paths_array.Add(TimingPathToValueMap(path));
    }
    return paths_array;
}

Upp::ValueMap JsonIO::HazardCandidateToValueMap(const HazardCandidate& hazard) {
    Upp::ValueMap hazard_map;
    Upp::ValueArray sources_array;
    for (const auto& source : hazard.sources) {
        sources_array.Add(TimingNodeIdToValueMap(source));
    }
    hazard_map.Add("sources", sources_array);

    Upp::ValueArray reconvergent_array;
    for (const auto& reconvergent : hazard.reconvergent_points) {
        reconvergent_array.Add(TimingNodeIdToValueMap(reconvergent));
    }
    hazard_map.Add("reconvergent_points", reconvergent_array);

    hazard_map.Add("description", Upp::String(hazard.description.c_str()));
    return hazard_map;
}

Upp::ValueArray JsonIO::HazardCandidatesToValueArray(const std::vector<HazardCandidate>& hazards) {
    Upp::ValueArray hazards_array;
    for (const auto& hazard : hazards) {
        hazards_array.Add(HazardCandidateToValueMap(hazard));
    }
    return hazards_array;
}

Upp::ValueArray JsonIO::TimingNodeIdsToValueArray(const std::vector<TimingNodeId>& nodes) {
    Upp::ValueArray nodes_array;
    for (const auto& node : nodes) {
        nodes_array.Add(TimingNodeIdToValueMap(node));
    }
    return nodes_array;
}

Upp::ValueArray JsonIO::TimingLoopsToValueArray(const std::vector<std::vector<TimingNodeId>>& loops) {
    Upp::ValueArray loops_array;
    for (const auto& loop : loops) {
        loops_array.Add(TimingNodeIdsToValueArray(loop));
    }
    return loops_array;
}

Upp::ValueMap JsonIO::FunctionalNodeIdToValueMap(const FunctionalNodeId& node_id) {
    Upp::ValueMap node_map;
    node_map.Add("id", Upp::String(node_id.id.c_str()));
    node_map.Add("kind", Upp::String(node_id.kind.c_str()));
    return node_map;
}

Upp::ValueMap JsonIO::ConeNodeToValueMap(const ConeNode& cone_node) {
    Upp::ValueMap cone_map;
    cone_map.Add("node", FunctionalNodeIdToValueMap(cone_node.node));
    cone_map.Add("depth", cone_node.depth);
    return cone_map;
}

Upp::ValueMap JsonIO::FunctionalConeToValueMap(const FunctionalCone& cone) {
    Upp::ValueMap cone_map;
    cone_map.Add("root", FunctionalNodeIdToValueMap(cone.root));

    Upp::ValueArray nodes_array;
    for (const auto& node : cone.nodes) {
        nodes_array.Add(ConeNodeToValueMap(node));
    }
    cone_map.Add("nodes", nodes_array);

    return cone_map;
}

Upp::ValueMap JsonIO::DependencySummaryToValueMap(const DependencySummary& summary) {
    Upp::ValueMap summary_map;
    summary_map.Add("root", FunctionalNodeIdToValueMap(summary.root));
    summary_map.Add("upstream_count", summary.upstream_count);
    summary_map.Add("downstream_count", summary.downstream_count);
    return summary_map;
}

Upp::ValueArray JsonIO::ConeNodesToValueArray(const std::vector<ConeNode>& cone_nodes) {
    Upp::ValueArray nodes_array;
    for (const auto& node : cone_nodes) {
        nodes_array.Add(ConeNodeToValueMap(node));
    }
    return nodes_array;
}

Upp::ValueArray JsonIO::FunctionalConesToValueArray(const std::vector<FunctionalCone>& cones) {
    Upp::ValueArray cones_array;
    for (const auto& cone : cones) {
        cones_array.Add(FunctionalConeToValueMap(cone));
    }
    return cones_array;
}

Upp::ValueArray JsonIO::DependencySummariesToValueArray(const std::vector<DependencySummary>& summaries) {
    Upp::ValueArray summaries_array;
    for (const auto& summary : summaries) {
        summaries_array.Add(DependencySummaryToValueMap(summary));
    }
    return summaries_array;
}

Upp::Value JsonIO::BlockKindToJson(BlockKind kind) {
    std::string kind_str = "GenericComb";
    switch (kind) {
        case BlockKind::GenericComb: kind_str = "GenericComb"; break;
        case BlockKind::Adder: kind_str = "Adder"; break;
        case BlockKind::Comparator: kind_str = "Comparator"; break;
        case BlockKind::Mux: kind_str = "Mux"; break;
        case BlockKind::Decoder: kind_str = "Decoder"; break;
        case BlockKind::Encoder: kind_str = "Encoder"; break;
        case BlockKind::Register: kind_str = "Register"; break;
        case BlockKind::Counter: kind_str = "Counter"; break;
        case BlockKind::Latch: kind_str = "Latch"; break;
    }
    return Upp::String(kind_str.c_str());
}

Upp::ValueMap JsonIO::BlockPortToValueMap(const BlockPort& port) {
    Upp::ValueMap port_map;
    port_map.Add("name", Upp::String(port.name.c_str()));
    port_map.Add("direction", Upp::String(port.direction.c_str()));

    Upp::ValueArray pins_array;
    for (const auto& pin_id : port.pins) {
        pins_array.Add(Upp::String(pin_id.c_str()));
    }
    port_map.Add("pins", pins_array);

    return port_map;
}

Upp::ValueMap JsonIO::BlockInstanceToValueMap(const BlockInstance& block) {
    Upp::ValueMap block_map;
    block_map.Add("id", Upp::String(block.id.c_str()));
    block_map.Add("kind", BlockKindToJson(block.kind));

    Upp::ValueArray components_array;
    for (const auto& comp_id : block.components) {
        components_array.Add(Upp::String(comp_id.c_str()));
    }
    block_map.Add("components", components_array);

    Upp::ValueArray nets_array;
    for (const auto& net_id : block.nets) {
        nets_array.Add(Upp::String(net_id.c_str()));
    }
    block_map.Add("nets", nets_array);

    Upp::ValueArray ports_array;
    for (const auto& port : block.ports) {
        ports_array.Add(BlockPortToValueMap(port));
    }
    block_map.Add("ports", ports_array);

    return block_map;
}

Upp::ValueMap JsonIO::BlockGraphToValueMap(const BlockGraph& block_graph) {
    Upp::ValueMap graph_map;

    graph_map.Add("blocks", BlockInstancesToValueArray(block_graph.blocks));

    // For now, add empty edges array (we can enhance this later)
    Upp::ValueArray edges_array;
    graph_map.Add("edges", edges_array);

    return graph_map;
}

Upp::ValueArray JsonIO::BlockInstancesToValueArray(const std::vector<BlockInstance>& blocks) {
    Upp::ValueArray array;
    for (const auto& block : blocks) {
        array.Add(BlockInstanceToValueMap(block));
    }
    return array;
}

Upp::Value JsonIO::BehaviorKindToJson(BehaviorKind kind) {
    std::string kind_str = "Unknown";
    switch (kind) {
        case BehaviorKind::Unknown: kind_str = "Unknown"; break;
        case BehaviorKind::CombinationalLogic: kind_str = "CombinationalLogic"; break;
        case BehaviorKind::Adder: kind_str = "Adder"; break;
        case BehaviorKind::Subtractor: kind_str = "Subtractor"; break;
        case BehaviorKind::Comparator: kind_str = "Comparator"; break;
        case BehaviorKind::EqualityComparator: kind_str = "EqualityComparator"; break;
        case BehaviorKind::InequalityComparator: kind_str = "InequalityComparator"; break;
        case BehaviorKind::Mux: kind_str = "Mux"; break;
        case BehaviorKind::Decoder: kind_str = "Decoder"; break;
        case BehaviorKind::Encoder: kind_str = "Encoder"; break;
        case BehaviorKind::Register: kind_str = "Register"; break;
        case BehaviorKind::Counter: kind_str = "Counter"; break;
        case BehaviorKind::StateMachine: kind_str = "StateMachine"; break;
    }
    return Upp::String(kind_str.c_str());
}

Upp::ValueMap JsonIO::BehaviorPortRoleToValueMap(const BehaviorPortRole& port_role) {
    Upp::ValueMap port_map;
    port_map.Add("port_name", Upp::String(port_role.port_name.c_str()));
    port_map.Add("role", Upp::String(port_role.role.c_str()));
    return port_map;
}

Upp::ValueMap JsonIO::BehaviorDescriptorToValueMap(const BehaviorDescriptor& descriptor) {
    Upp::ValueMap desc_map;
    desc_map.Add("subject_id", Upp::String(descriptor.subject_id.c_str()));
    desc_map.Add("subject_kind", Upp::String(descriptor.subject_kind.c_str()));
    desc_map.Add("behavior_kind", BehaviorKindToJson(descriptor.behavior_kind));
    desc_map.Add("bit_width", descriptor.bit_width);
    desc_map.Add("description", Upp::String(descriptor.description.c_str()));

    Upp::ValueArray ports_array;
    for (const auto& port : descriptor.ports) {
        ports_array.Add(BehaviorPortRoleToValueMap(port));
    }
    desc_map.Add("ports", ports_array);

    return desc_map;
}

Upp::ValueArray JsonIO::BehaviorPortRolesToValueArray(const std::vector<BehaviorPortRole>& port_roles) {
    Upp::ValueArray array;
    for (const auto& port_role : port_roles) {
        array.Add(BehaviorPortRoleToValueMap(port_role));
    }
    return array;
}

Upp::Value JsonIO::TransformationKindToJson(TransformationKind kind) {
    std::string kind_str = "Unknown";
    switch (kind) {
        case TransformationKind::Unknown: kind_str = "Unknown"; break;
        case TransformationKind::SimplifyDoubleInversion: kind_str = "SimplifyDoubleInversion"; break;
        case TransformationKind::SimplifyRedundantGate: kind_str = "SimplifyRedundantGate"; break;
        case TransformationKind::ReplaceWithKnownBlock: kind_str = "ReplaceWithKnownBlock"; break;
        case TransformationKind::RewireFanoutTree: kind_str = "RewireFanoutTree"; break;
        case TransformationKind::MergeEquivalentBlocks: kind_str = "MergeEquivalentBlocks"; break;
    }
    return Upp::String(kind_str.c_str());
}

Upp::Value JsonIO::PreservationLevelToJson(PreservationLevel level) {
    std::string level_str = "BehaviorKindPreserved";
    switch (level) {
        case PreservationLevel::BehaviorKindPreserved: level_str = "BehaviorKindPreserved"; break;
        case PreservationLevel::IOContractPreserved: level_str = "IOContractPreserved"; break;
        case PreservationLevel::DependencyPatternPreserved: level_str = "DependencyPatternPreserved"; break;
    }
    return Upp::String(level_str.c_str());
}

Upp::ValueMap JsonIO::TransformationTargetToValueMap(const TransformationTarget& target) {
    Upp::ValueMap target_map;
    target_map.Add("subject_id", Upp::String(target.subject_id.c_str()));
    target_map.Add("subject_kind", Upp::String(target.subject_kind.c_str()));
    return target_map;
}

Upp::ValueMap JsonIO::TransformationStepToValueMap(const TransformationStep& step) {
    Upp::ValueMap step_map;
    step_map.Add("description", Upp::String(step.description.c_str()));
    return step_map;
}

Upp::ValueMap JsonIO::TransformationPlanToValueMap(const TransformationPlan& plan) {
    Upp::ValueMap plan_map;
    plan_map.Add("id", Upp::String(plan.id.c_str()));
    plan_map.Add("kind", TransformationKindToJson(plan.kind));
    plan_map.Add("target", TransformationTargetToValueMap(plan.target));
    plan_map.Add("guarantees", PreservationLevelsToValueArray(plan.guarantees));
    plan_map.Add("steps", TransformationStepsToValueArray(plan.steps));
    return plan_map;
}

Upp::ValueArray JsonIO::PreservationLevelsToValueArray(const Vector<PreservationLevel>& levels) {
    Upp::ValueArray array;
    for (const auto& level : levels) {
        array.Add(PreservationLevelToJson(level));
    }
    return array;
}

Upp::ValueArray JsonIO::TransformationStepsToValueArray(const Vector<TransformationStep>& steps) {
    Upp::ValueArray array;
    for (const auto& step : steps) {
        array.Add(TransformationStepToValueMap(step));
    }
    return array;
}

Upp::ValueArray JsonIO::TransformationPlansToValueArray(const Vector<TransformationPlan>& plans) {
    Upp::ValueArray array;
    for (const auto& plan : plans) {
        array.Add(TransformationPlanToValueMap(plan));
    }
    return array;
}

Upp::Value JsonIO::IrExprKindToJson(IrExprKind kind) {
    std::string kind_str = "Value";
    switch (kind) {
        case IrExprKind::Value: kind_str = "Value"; break;
        case IrExprKind::Not: kind_str = "Not"; break;
        case IrExprKind::And: kind_str = "And"; break;
        case IrExprKind::Or: kind_str = "Or"; break;
        case IrExprKind::Xor: kind_str = "Xor"; break;
        case IrExprKind::Add: kind_str = "Add"; break;
        case IrExprKind::Sub: kind_str = "Sub"; break;
        case IrExprKind::Mux: kind_str = "Mux"; break;
        case IrExprKind::Eq: kind_str = "Eq"; break;
        case IrExprKind::Neq: kind_str = "Neq"; break;
    }
    return Upp::String(kind_str.c_str());
}

Upp::ValueMap JsonIO::IrValueToValueMap(const IrValue& value) {
    Upp::ValueMap value_map;
    value_map.Add("name", Upp::String(value.name.c_str()));
    value_map.Add("bit_width", value.bit_width);
    value_map.Add("is_literal", value.is_literal);
    if (value.is_literal) {
        value_map.Add("literal", static_cast<int>(value.literal));
    } else {
        value_map.Add("literal", Upp::Value()); // null if not a literal
    }
    return value_map;
}

Upp::ValueMap JsonIO::IrExprToValueMap(const IrExpr& expr) {
    Upp::ValueMap expr_map;
    expr_map.Add("kind", IrExprKindToJson(expr.kind));
    expr_map.Add("target", IrValueToValueMap(expr.target));

    Upp::ValueArray args_array;
    for (const auto& arg : expr.args) {
        args_array.Add(IrValueToValueMap(arg));
    }
    expr_map.Add("args", args_array);

    return expr_map;
}

Upp::ValueMap JsonIO::IrRegAssignToValueMap(const IrRegAssign& reg_assign) {
    Upp::ValueMap assign_map;
    assign_map.Add("target", IrValueToValueMap(reg_assign.target));
    assign_map.Add("expr", IrExprToValueMap(reg_assign.expr));
    assign_map.Add("clock", Upp::String(reg_assign.clock.c_str()));
    assign_map.Add("reset", Upp::String(reg_assign.reset.c_str()));
    return assign_map;
}

Upp::ValueMap JsonIO::IrModuleToValueMap(const IrModule& module) {
    Upp::ValueMap module_map;
    module_map.Add("id", Upp::String(module.id.c_str()));

    // Add inputs
    Upp::ValueArray inputs_array;
    for (const auto& input : module.inputs) {
        inputs_array.Add(IrValueToValueMap(input));
    }
    module_map.Add("inputs", inputs_array);

    // Add outputs
    Upp::ValueArray outputs_array;
    for (const auto& output : module.outputs) {
        outputs_array.Add(IrValueToValueMap(output));
    }
    module_map.Add("outputs", outputs_array);

    // Add combinational assignments
    Upp::ValueArray comb_assigns_array;
    for (const auto& assign : module.comb_assigns) {
        comb_assigns_array.Add(IrExprToValueMap(assign));
    }
    module_map.Add("comb_assigns", comb_assigns_array);

    // Add register assignments
    Upp::ValueArray reg_assigns_array;
    for (const auto& assign : module.reg_assigns) {
        reg_assigns_array.Add(IrRegAssignToValueMap(assign));
    }
    module_map.Add("reg_assigns", reg_assigns_array);

    return module_map;
}

Upp::ValueArray JsonIO::IrValuesToValueArray(const std::vector<IrValue>& values) {
    Upp::ValueArray array;
    for (const auto& value : values) {
        array.Add(IrValueToValueMap(value));
    }
    return array;
}

Upp::ValueArray JsonIO::IrExprsToValueArray(const std::vector<IrExpr>& exprs) {
    Upp::ValueArray array;
    for (const auto& expr : exprs) {
        array.Add(IrExprToValueMap(expr));
    }
    return array;
}

Upp::ValueArray JsonIO::IrRegAssignsToValueArray(const std::vector<IrRegAssign>& reg_assigns) {
    Upp::ValueArray array;
    for (const auto& assign : reg_assigns) {
        array.Add(IrRegAssignToValueMap(assign));
    }
    return array;
}

Upp::Value JsonIO::BehaviorChangeKindToJson(BehaviorChangeKind kind) {
    switch (kind) {
        case BehaviorChangeKind::None:
            return Upp::String("None");
        case BehaviorChangeKind::BehaviorKindChanged:
            return Upp::String("BehaviorKindChanged");
        case BehaviorChangeKind::BitWidthChanged:
            return Upp::String("BitWidthChanged");
        case BehaviorChangeKind::PortsChanged:
            return Upp::String("PortsChanged");
        case BehaviorChangeKind::DescriptionChanged:
            return Upp::String("DescriptionChanged");
        case BehaviorChangeKind::MultipleChanges:
            return Upp::String("MultipleChanges");
        default:
            return Upp::String("Unknown");
    }
}

Upp::Value JsonIO::IrChangeKindToJson(IrChangeKind kind) {
    switch (kind) {
        case IrChangeKind::None:
            return Upp::String("None");
        case IrChangeKind::InterfaceChanged:
            return Upp::String("InterfaceChanged");
        case IrChangeKind::CombLogicChanged:
            return Upp::String("CombLogicChanged");
        case IrChangeKind::RegLogicChanged:
            return Upp::String("RegLogicChanged");
        case IrChangeKind::MultipleChanges:
            return Upp::String("MultipleChanges");
        default:
            return Upp::String("Unknown");
    }
}

Upp::ValueMap JsonIO::PortChangeToValueMap(const PortChange& port_change) {
    Upp::ValueMap map;
    map.Add("port_name", Upp::String(port_change.port_name.c_str()));
    map.Add("before_role", Upp::String(port_change.before_role.c_str()));
    map.Add("after_role", Upp::String(port_change.after_role.c_str()));
    map.Add("before_width", port_change.before_width);
    map.Add("after_width", port_change.after_width);
    return map;
}

Upp::ValueMap JsonIO::BehaviorDiffToValueMap(const BehaviorDiff& behavior_diff) {
    Upp::ValueMap map;
    map.Add("subject_id", Upp::String(behavior_diff.subject_id.c_str()));
    map.Add("subject_kind", Upp::String(behavior_diff.subject_kind.c_str()));
    map.Add("change_kind", BehaviorChangeKindToJson(behavior_diff.change_kind));
    map.Add("before_behavior", BehaviorDescriptorToValueMap(behavior_diff.before_behavior));
    map.Add("after_behavior", BehaviorDescriptorToValueMap(behavior_diff.after_behavior));
    map.Add("port_changes", PortChangesToValueArray(behavior_diff.port_changes));
    return map;
}

Upp::ValueMap JsonIO::IrExprChangeToValueMap(const IrExprChange& expr_change) {
    Upp::ValueMap map;
    map.Add("target_name", Upp::String(expr_change.target_name.c_str()));
    map.Add("before_expr_repr", Upp::String(expr_change.before_expr_repr.c_str()));
    map.Add("after_expr_repr", Upp::String(expr_change.after_expr_repr.c_str()));
    return map;
}

Upp::ValueMap JsonIO::IrRegChangeToValueMap(const IrRegChange& reg_change) {
    Upp::ValueMap map;
    map.Add("target_name", Upp::String(reg_change.target_name.c_str()));
    map.Add("before_expr_repr", Upp::String(reg_change.before_expr_repr.c_str()));
    map.Add("after_expr_repr", Upp::String(reg_change.after_expr_repr.c_str()));
    return map;
}

Upp::ValueMap JsonIO::IrInterfaceChangeToValueMap(const IrInterfaceChange& iface_change) {
    Upp::ValueMap map;
    map.Add("added_inputs", IrValuesToValueArraySimple(iface_change.added_inputs));
    map.Add("removed_inputs", IrValuesToValueArraySimple(iface_change.removed_inputs));
    map.Add("added_outputs", IrValuesToValueArraySimple(iface_change.added_outputs));
    map.Add("removed_outputs", IrValuesToValueArraySimple(iface_change.removed_outputs));
    return map;
}

Upp::ValueMap JsonIO::IrDiffToValueMap(const IrDiff& ir_diff) {
    Upp::ValueMap map;
    map.Add("module_id", Upp::String(ir_diff.module_id.c_str()));
    map.Add("change_kind", IrChangeKindToJson(ir_diff.change_kind));
    map.Add("iface_changes", IrInterfaceChangeToValueMap(ir_diff.iface_changes));
    map.Add("comb_changes", IrExprChangesToValueArray(ir_diff.comb_changes));
    map.Add("reg_changes", IrRegChangesToValueArray(ir_diff.reg_changes));
    return map;
}

Upp::ValueArray JsonIO::PortChangesToValueArray(const std::vector<PortChange>& port_changes) {
    Upp::ValueArray array;
    for (const auto& change : port_changes) {
        array.Add(PortChangeToValueMap(change));
    }
    return array;
}

Upp::ValueArray JsonIO::IrExprChangesToValueArray(const std::vector<IrExprChange>& expr_changes) {
    Upp::ValueArray array;
    for (const auto& change : expr_changes) {
        array.Add(IrExprChangeToValueMap(change));
    }
    return array;
}

Upp::ValueArray JsonIO::IrRegChangesToValueArray(const std::vector<IrRegChange>& reg_changes) {
    Upp::ValueArray array;
    for (const auto& change : reg_changes) {
        array.Add(IrRegChangeToValueMap(change));
    }
    return array;
}

Upp::ValueArray JsonIO::IrValuesToValueArraySimple(const std::vector<IrValue>& values) {
    Upp::ValueArray array;
    for (const auto& value : values) {
        array.Add(IrValueToValueMap(value));
    }
    return array;
}

Upp::Value JsonIO::IrOptPassKindToJson(IrOptPassKind kind) {
    switch (kind) {
        case IrOptPassKind::SimplifyAlgebraic: return Upp::String("SimplifyAlgebraic");
        case IrOptPassKind::FoldConstants: return Upp::String("FoldConstants");
        case IrOptPassKind::SimplifyMux: return Upp::String("SimplifyMux");
        case IrOptPassKind::EliminateTrivialLogic: return Upp::String("EliminateTrivialLogic");
        default: return Upp::String("Unknown");
    }
}

Upp::ValueMap JsonIO::IrOptChangeSummaryToValueMap(const IrOptChangeSummary& summary) {
    Upp::ValueMap map;
    map.Add("pass_kind", IrOptPassKindToJson(summary.pass_kind));
    map.Add("expr_changes", summary.expr_changes);
    map.Add("reg_changes", summary.reg_changes);
    map.Add("behavior_preserved", summary.behavior_preserved);
    return map;
}

Upp::ValueArray JsonIO::IrOptChangeSummariesToValueArray(const std::vector<IrOptChangeSummary>& summaries) {
    Upp::ValueArray array;
    for (const auto& summary : summaries) {
        array.Add(IrOptChangeSummaryToValueMap(summary));
    }
    return array;
}

Upp::ValueMap JsonIO::IrOptimizationResultToValueMap(const IrOptimizationResult& result) {
    Upp::ValueMap map;
    map.Add("original", IrModuleToValueMap(result.original));
    map.Add("optimized", IrModuleToValueMap(result.optimized));
    map.Add("summaries", IrOptChangeSummariesToValueArray(result.summaries));
    return map;
}

Upp::ValueMap JsonIO::CoDesignerSessionStateToValueMap(const CoDesignerSessionState& session) {
    Upp::ValueMap session_map;
    session_map.Add("designer_session_id", Upp::String(session.designer_session_id.c_str()));
    session_map.Add("proto_session_id", session.proto_session_id);
    session_map.Add("branch", Upp::String(session.branch.c_str()));
    session_map.Add("current_block_id", Upp::String(session.current_block_id.c_str()));
    session_map.Add("current_node_id", Upp::String(session.current_node_id.c_str()));
    session_map.Add("current_node_kind", Upp::String(session.current_node_kind.c_str()));
    session_map.Add("use_optimized_ir", session.use_optimized_ir);
    return session_map;
}

Upp::Value JsonIO::PlaybookKindToJson(PlaybookKind kind) {
    std::string kind_str = "OptimizeBlockAndReport";
    switch (kind) {
        case PlaybookKind::OptimizeBlockAndReport: kind_str = "OptimizeBlockAndReport"; break;
        case PlaybookKind::OptimizeAndApplySafeRefactors: kind_str = "OptimizeAndApplySafeRefactors"; break;
        case PlaybookKind::SystemOptimizeAndReport: kind_str = "SystemOptimizeAndReport"; break;
        case PlaybookKind::SystemOptimizeAndApplySafeRefactors: kind_str = "SystemOptimizeAndApplySafeRefactors"; break;
    }
    return Upp::String(kind_str.c_str());
}

Upp::ValueMap JsonIO::PlaybookConfigToValueMap(const PlaybookConfig& config) {
    Upp::ValueMap config_map;
    config_map.Add("kind", PlaybookKindToJson(config.kind));
    config_map.Add("designer_session_id", Upp::String(config.designer_session_id.c_str()));
    config_map.Add("target", Upp::String(config.target.c_str()));
    config_map.Add("block_id", Upp::String(config.block_id.c_str()));

    // Add system-level parameters
    Upp::ValueArray block_ids_array;
    for (const auto& block_id : config.block_ids) {
        block_ids_array.Add(Upp::String(block_id.c_str()));
    }
    config_map.Add("block_ids", block_ids_array);
    config_map.Add("name_prefix", Upp::String(config.name_prefix.c_str()));

    config_map.Add("baseline_branch", Upp::String(config.baseline_branch.c_str()));

    // Add passes array
    Upp::ValueArray passes_array;
    for (const auto& pass : config.passes) {
        passes_array.Add(IrOptPassKindToJson(pass));
    }
    config_map.Add("passes", passes_array);

    config_map.Add("use_optimized_ir", config.use_optimized_ir);
    config_map.Add("apply_refactors", config.apply_refactors);
    return config_map;
}

Upp::ValueMap JsonIO::CodegenModuleToValueMap(const CodegenModule& module) {
    Upp::ValueMap module_map;
    module_map.Add("id", Upp::String(module.id.c_str()));
    module_map.Add("name", Upp::String(module.name.c_str()));
    module_map.Add("flavor", Upp::String(module.flavor.c_str()));
    module_map.Add("code", Upp::String(module.code.c_str()));
    return module_map;
}

Upp::ValueMap JsonIO::BlockPlaybookResultToValueMap(const BlockPlaybookResult& result) {
    Upp::ValueMap result_map;
    result_map.Add("block_id", Upp::String(result.block_id.c_str()));

    // Add behavior descriptors
    result_map.Add("initial_behavior", BehaviorDescriptorToValueMap(result.initial_behavior));
    result_map.Add("final_behavior", BehaviorDescriptorToValueMap(result.final_behavior));

    // Add IR modules
    result_map.Add("initial_ir", IrModuleToValueMap(result.initial_ir));
    result_map.Add("final_ir", IrModuleToValueMap(result.final_ir));

    // Add optimization result
    result_map.Add("optimization", IrOptimizationResultToValueMap(result.optimization));

    // Add transformation plans
    result_map.Add("proposed_plans", TransformationPlansToValueArray(result.proposed_plans));

    // Add applied plan IDs
    result_map.Add("applied_plan_ids", StringVectorToValueArray(result.applied_plan_ids));

    // Add revision
    result_map.Add("new_circuit_revision", result.new_circuit_revision);

    // Add diffs
    result_map.Add("behavior_diff", BehaviorDiffToValueMap(result.behavior_diff));
    result_map.Add("ir_diff", IrDiffToValueMap(result.ir_diff));

    // Add codegen
    result_map.Add("codegen", CodegenModuleToValueMap(result.codegen));

    return result_map;
}

Upp::ValueMap JsonIO::PlaybookResultToValueMap(const PlaybookResult& result) {
    Upp::ValueMap result_map;
    result_map.Add("kind", PlaybookKindToJson(result.kind));
    result_map.Add("config", PlaybookConfigToValueMap(result.config));
    result_map.Add("designer_session", CoDesignerSessionStateToValueMap(result.designer_session));

    // Add behavior descriptors
    result_map.Add("initial_behavior", BehaviorDescriptorToValueMap(result.initial_behavior));
    result_map.Add("final_behavior", BehaviorDescriptorToValueMap(result.final_behavior));

    // Add IR modules
    result_map.Add("initial_ir", IrModuleToValueMap(result.initial_ir));
    result_map.Add("final_ir", IrModuleToValueMap(result.final_ir));

    // Add optimization result
    result_map.Add("optimization", IrOptimizationResultToValueMap(result.optimization));

    // Add transformation plans
    result_map.Add("proposed_plans", TransformationPlansToValueArray(result.proposed_plans));

    // Add applied plan IDs
    result_map.Add("applied_plan_ids", StringVectorToValueArray(result.applied_plan_ids));

    // Add revision
    result_map.Add("new_circuit_revision", result.new_circuit_revision);

    // Add diffs
    result_map.Add("behavior_diff", BehaviorDiffToValueMap(result.behavior_diff));
    result_map.Add("ir_diff", IrDiffToValueMap(result.ir_diff));

    // Add codegen
    result_map.Add("codegen", CodegenModuleToValueMap(result.codegen));

    // Add system-level results
    Upp::ValueArray system_block_results_array;
    for (const auto& block_result : result.system_block_results) {
        system_block_results_array.Add(BlockPlaybookResultToValueMap(block_result));
    }
    result_map.Add("system_block_results", system_block_results_array);

    // Add system-level metrics
    result_map.Add("total_blocks", result.total_blocks);
    result_map.Add("blocks_with_changes", result.blocks_with_changes);
    result_map.Add("total_applied_plans", result.total_applied_plans);

    return result_map;
}

Upp::ValueArray JsonIO::TransformationPlansToValueArray(const std::vector<TransformationPlan>& plans) {
    Upp::ValueArray array;
    for (const auto& plan : plans) {
        array.Add(TransformationPlanToValueMap(plan));
    }
    return array;
}

Upp::ValueArray JsonIO::StringVectorToValueArray(const std::vector<std::string>& strings) {
    Upp::ValueArray array;
    for (const auto& str : strings) {
        array.Add(Upp::String(str.c_str()));
    }
    return array;
}

Upp::Value JsonIO::SchedulingStrategyToJson(SchedulingStrategy strategy) {
    std::string strategy_str = "SingleStage";
    switch (strategy) {
        case SchedulingStrategy::SingleStage:
            strategy_str = "SingleStage";
            break;
        case SchedulingStrategy::DepthBalancedStages:
            strategy_str = "DepthBalancedStages";
            break;
        case SchedulingStrategy::FixedStageCount:
            strategy_str = "FixedStageCount";
            break;
    }
    return Upp::String(strategy_str.c_str());
}

Upp::ValueMap JsonIO::SchedulingConfigToValueMap(const SchedulingConfig& config) {
    Upp::ValueMap config_map;
    config_map.Add("strategy", SchedulingStrategyToJson(config.strategy));
    config_map.Add("requested_stages", config.requested_stages);
    return config_map;
}

Upp::ValueMap JsonIO::ScheduledExprToValueMap(const ScheduledExpr& scheduled_expr) {
    Upp::ValueMap scheduled_expr_map;
    scheduled_expr_map.Add("stage", scheduled_expr.stage);
    scheduled_expr_map.Add("expr", IrExprToValueMap(scheduled_expr.expr));
    return scheduled_expr_map;
}

Upp::ValueMap JsonIO::ScheduledRegAssignToValueMap(const ScheduledRegAssign& scheduled_reg_assign) {
    Upp::ValueMap scheduled_reg_assign_map;
    scheduled_reg_assign_map.Add("stage", scheduled_reg_assign.stage);
    scheduled_reg_assign_map.Add("reg_assign", IrRegAssignToValueMap(scheduled_reg_assign.reg_assign));
    return scheduled_reg_assign_map;
}

Upp::ValueMap JsonIO::ScheduledModuleToValueMap(const ScheduledModule& scheduled_module) {
    Upp::ValueMap scheduled_module_map;
    scheduled_module_map.Add("id", Upp::String(scheduled_module.id.c_str()));
    scheduled_module_map.Add("num_stages", scheduled_module.num_stages);

    // Add inputs
    scheduled_module_map.Add("inputs", IrValuesToValueArray(scheduled_module.inputs));

    // Add outputs
    scheduled_module_map.Add("outputs", IrValuesToValueArray(scheduled_module.outputs));

    // Add scheduled combinational operations
    scheduled_module_map.Add("comb_ops", ScheduledExprsToValueArray(scheduled_module.comb_ops));

    // Add scheduled register operations
    scheduled_module_map.Add("reg_ops", ScheduledRegAssignsToValueArray(scheduled_module.reg_ops));

    return scheduled_module_map;
}

Upp::ValueArray JsonIO::ScheduledExprsToValueArray(const std::vector<ScheduledExpr>& scheduled_exprs) {
    Upp::ValueArray array;
    for (const auto& scheduled_expr : scheduled_exprs) {
        array.Add(ScheduledExprToValueMap(scheduled_expr));
    }
    return array;
}

Upp::ValueArray JsonIO::ScheduledRegAssignsToValueArray(const std::vector<ScheduledRegAssign>& scheduled_reg_assigns) {
    Upp::ValueArray array;
    for (const auto& scheduled_reg_assign : scheduled_reg_assigns) {
        array.Add(ScheduledRegAssignToValueMap(scheduled_reg_assign));
    }
    return array;
}

Upp::ValueMap JsonIO::ClockSignalInfoToValueMap(const ClockSignalInfo& clock_signal) {
    Upp::ValueMap signal_map;
    signal_map.Add("signal_name", Upp::String(clock_signal.signal_name.c_str()));
    signal_map.Add("domain_id", clock_signal.domain_id);
    return signal_map;
}

Upp::ValueMap JsonIO::RegisterInfoToValueMap(const RegisterInfo& register_info) {
    Upp::ValueMap reg_map;
    reg_map.Add("reg_id", Upp::String(register_info.reg_id.c_str()));
    reg_map.Add("name", Upp::String(register_info.name.c_str()));
    reg_map.Add("clock_signal", Upp::String(register_info.clock_signal.c_str()));
    reg_map.Add("domain_id", register_info.domain_id);
    reg_map.Add("reset_signal", Upp::String(register_info.reset_signal.c_str()));
    return reg_map;
}

Upp::ValueMap JsonIO::PipelineStageInfoToValueMap(const PipelineStageInfo& stage_info) {
    Upp::ValueMap stage_map;
    stage_map.Add("stage_index", stage_info.stage_index);
    stage_map.Add("domain_id", stage_info.domain_id);
    stage_map.Add("comb_depth_estimate", stage_info.comb_depth_estimate);

    // Add register lists
    Upp::ValueArray registers_in_array;
    for (const auto& reg_id : stage_info.registers_in) {
        registers_in_array.Add(Upp::String(reg_id.c_str()));
    }
    stage_map.Add("registers_in", registers_in_array);

    Upp::ValueArray registers_out_array;
    for (const auto& reg_id : stage_info.registers_out) {
        registers_out_array.Add(Upp::String(reg_id.c_str()));
    }
    stage_map.Add("registers_out", registers_out_array);

    return stage_map;
}

Upp::ValueMap JsonIO::RegToRegPathInfoToValueMap(const RegToRegPathInfo& path_info) {
    Upp::ValueMap path_map;
    path_map.Add("src_reg_id", Upp::String(path_info.src_reg_id.c_str()));
    path_map.Add("dst_reg_id", Upp::String(path_info.dst_reg_id.c_str()));
    path_map.Add("domain_id", path_info.domain_id);
    path_map.Add("comb_depth_estimate", path_info.comb_depth_estimate);
    path_map.Add("stage_span", path_info.stage_span);
    path_map.Add("crosses_clock_domain", path_info.crosses_clock_domain);
    return path_map;
}

Upp::ValueMap JsonIO::PipelineMapToValueMap(const PipelineMap& pipeline_map) {
    Upp::ValueMap map;
    map.Add("id", Upp::String(pipeline_map.id.c_str()));

    // Add clock domains
    map.Add("clock_domains", ClockSignalInfosToValueArray(pipeline_map.clock_domains));

    // Add registers
    map.Add("registers", RegisterInfosToValueArray(pipeline_map.registers));

    // Add pipeline stages
    map.Add("stages", PipelineStageInfosToValueArray(pipeline_map.stages));

    // Add register-to-register paths
    map.Add("reg_paths", RegToRegPathInfosToValueArray(pipeline_map.reg_paths));

    return map;
}

Upp::ValueArray JsonIO::ClockSignalInfosToValueArray(const std::vector<ClockSignalInfo>& clock_signals) {
    Upp::ValueArray array;
    for (const auto& signal : clock_signals) {
        array.Add(ClockSignalInfoToValueMap(signal));
    }
    return array;
}

Upp::ValueArray JsonIO::RegisterInfosToValueArray(const std::vector<RegisterInfo>& registers) {
    Upp::ValueArray array;
    for (const auto& reg : registers) {
        array.Add(RegisterInfoToValueMap(reg));
    }
    return array;
}

Upp::ValueArray JsonIO::PipelineStageInfosToValueArray(const std::vector<PipelineStageInfo>& stages) {
    Upp::ValueArray array;
    for (const auto& stage : stages) {
        array.Add(PipelineStageInfoToValueMap(stage));
    }
    return array;
}

Upp::ValueArray JsonIO::RegToRegPathInfosToValueArray(const std::vector<RegToRegPathInfo>& paths) {
    Upp::ValueArray array;
    for (const auto& path : paths) {
        array.Add(RegToRegPathInfoToValueMap(path));
    }
    return array;
}

// CDC model serialization implementations

Upp::Value JsonIO::CdcCrossingKindToJson(CdcCrossingKind kind) {
    switch (kind) {
        case CdcCrossingKind::SingleBitSyncCandidate:
            return Upp::String("SingleBitSyncCandidate");
        case CdcCrossingKind::MultiBitBundle:
            return Upp::String("MultiBitBundle");
        case CdcCrossingKind::HandshakeLike:
            return Upp::String("HandshakeLike");
        case CdcCrossingKind::UnknownPattern:
            return Upp::String("UnknownPattern");
        default:
            return Upp::String("Unknown");
    }
}

Upp::Value JsonIO::CdcSeverityToJson(CdcSeverity severity) {
    switch (severity) {
        case CdcSeverity::Info:
            return Upp::String("Info");
        case CdcSeverity::Warning:
            return Upp::String("Warning");
        case CdcSeverity::Error:
            return Upp::String("Error");
        default:
            return Upp::String("Unknown");
    }
}

Upp::ValueMap JsonIO::CdcCrossingEndpointToValueMap(const CdcCrossingEndpoint& endpoint) {
    Upp::ValueMap map;
    map.Add("reg_id", Upp::String(endpoint.reg_id.c_str()));
    map.Add("clock_signal", Upp::String(endpoint.clock_signal.c_str()));
    map.Add("domain_id", endpoint.domain_id);
    return map;
}

Upp::ValueMap JsonIO::CdcCrossingToValueMap(const CdcCrossing& crossing) {
    Upp::ValueMap map;
    map.Add("id", Upp::String(crossing.id.c_str()));
    map.Add("src", CdcCrossingEndpointToValueMap(crossing.src));
    map.Add("dst", CdcCrossingEndpointToValueMap(crossing.dst));
    map.Add("kind", CdcCrossingKindToJson(crossing.kind));
    map.Add("is_single_bit", crossing.is_single_bit);
    map.Add("bit_width", crossing.bit_width);
    map.Add("crosses_reset_boundary", crossing.crosses_reset_boundary);
    return map;
}

Upp::ValueMap JsonIO::CdcIssueToValueMap(const CdcIssue& issue) {
    Upp::ValueMap map;
    map.Add("id", Upp::String(issue.id.c_str()));
    map.Add("severity", CdcSeverityToJson(issue.severity));
    map.Add("summary", Upp::String(issue.summary.c_str()));
    map.Add("detail", Upp::String(issue.detail.c_str()));
    map.Add("crossing_id", Upp::String(issue.crossing_id.c_str()));
    return map;
}

Upp::ValueMap JsonIO::CdcReportToValueMap(const CdcReport& report) {
    Upp::ValueMap map;
    map.Add("id", Upp::String(report.id.c_str()));

    // Convert clock domains
    Upp::ValueArray clock_domains_array;
    for (const auto& clock_domain : report.clock_domains) {
        Upp::ValueMap clock_map;
        clock_map.Add("signal_name", Upp::String(clock_domain.signal_name.c_str()));
        clock_map.Add("domain_id", clock_domain.domain_id);
        clock_domains_array.Add(clock_map);
    }
    map.Add("clock_domains", clock_domains_array);

    // Convert crossings
    map.Add("crossings", CdcCrossingsToValueArray(report.crossings));

    // Convert issues
    map.Add("issues", CdcIssuesToValueArray(report.issues));

    return map;
}

Upp::ValueArray JsonIO::CdcCrossingsToValueArray(const Vector<CdcCrossing>& crossings) {
    Upp::ValueArray array;
    for (const auto& crossing : crossings) {
        array.Add(CdcCrossingToValueMap(crossing));
    }
    return array;
}

Upp::ValueArray JsonIO::CdcIssuesToValueArray(const Vector<CdcIssue>& issues) {
    Upp::ValueArray array;
    for (const auto& issue : issues) {
        array.Add(CdcIssueToValueMap(issue));
    }
    return array;
}

} // namespace ProtoVMCLI