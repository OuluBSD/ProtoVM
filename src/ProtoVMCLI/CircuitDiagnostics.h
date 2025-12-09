#ifndef _ProtoVM_CircuitDiagnostics_h_
#define _ProtoVM_CircuitDiagnostics_h_

#include "CircuitData.h"
#include "SessionTypes.h"  // For Result<T>
#include <string>
#include <vector>

namespace ProtoVMCLI {

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error
};

enum class DiagnosticKind {
    FloatingNet,
    ShortCircuit,
    MultipleDrivers,
    UnconnectedPin,
    InvalidFanout,
    ClockDomainConflict,
    GenericIssue
    // extendable in the future
};

struct CircuitDiagnosticLocation {
    std::string component_id;  // optional, may be empty
    std::string wire_id;       // optional
    std::string pin_name;      // optional
};

struct CircuitDiagnostic {
    DiagnosticSeverity severity;
    DiagnosticKind kind;
    CircuitDiagnosticLocation location;
    std::string message;           // human-readable
    std::string suggested_fix;     // optional, may be empty
};

// Convert DiagnosticSeverity to string for JSON serialization
inline std::string DiagnosticSeverityToString(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Info:    return "info";
        case DiagnosticSeverity::Warning: return "warning";
        case DiagnosticSeverity::Error:   return "error";
        default: return "unknown";
    }
}

// Convert string to DiagnosticSeverity
inline DiagnosticSeverity StringToDiagnosticSeverity(const std::string& str) {
    if (str == "info") return DiagnosticSeverity::Info;
    if (str == "warning") return DiagnosticSeverity::Warning;
    if (str == "error") return DiagnosticSeverity::Error;
    return DiagnosticSeverity::Warning; // default
}

// Convert DiagnosticKind to string for JSON serialization
inline std::string DiagnosticKindToString(DiagnosticKind kind) {
    switch (kind) {
        case DiagnosticKind::FloatingNet:       return "FloatingNet";
        case DiagnosticKind::ShortCircuit:      return "ShortCircuit";
        case DiagnosticKind::MultipleDrivers:   return "MultipleDrivers";
        case DiagnosticKind::UnconnectedPin:    return "UnconnectedPin";
        case DiagnosticKind::InvalidFanout:     return "InvalidFanout";
        case DiagnosticKind::ClockDomainConflict: return "ClockDomainConflict";
        case DiagnosticKind::GenericIssue:      return "GenericIssue";
        default: return "Unknown";
    }
}

// Convert string to DiagnosticKind
inline DiagnosticKind StringToDiagnosticKind(const std::string& str) {
    if (str == "FloatingNet") return DiagnosticKind::FloatingNet;
    if (str == "ShortCircuit") return DiagnosticKind::ShortCircuit;
    if (str == "MultipleDrivers") return DiagnosticKind::MultipleDrivers;
    if (str == "UnconnectedPin") return DiagnosticKind::UnconnectedPin;
    if (str == "InvalidFanout") return DiagnosticKind::InvalidFanout;
    if (str == "ClockDomainConflict") return DiagnosticKind::ClockDomainConflict;
    if (str == "GenericIssue") return DiagnosticKind::GenericIssue;
    return DiagnosticKind::GenericIssue; // default
}

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitDiagnostics_h_