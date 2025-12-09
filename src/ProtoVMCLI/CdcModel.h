#pragma once

#include <Core/Core.h>
#include <plugin/Result.h>

using namespace Upp;

enum class CdcCrossingKind {
    SingleBitSyncCandidate,
    MultiBitBundle,
    HandshakeLike,
    UnknownPattern
};

enum class CdcSeverity {
    Info,
    Warning,
    Error
};

struct CdcCrossingEndpoint {
    String reg_id;       // from PipelineMap::RegisterInfo::reg_id, or a special ID for non-reg endpoints
    String clock_signal; // clock signal name
    int    domain_id;    // domain id from PipelineMap
};

struct CdcCrossing {
    String id;           // unique id for the crossing (e.g. "CDCC_0001")

    CdcCrossingEndpoint src;
    CdcCrossingEndpoint dst;

    CdcCrossingKind kind;

    // Simple characterization:
    bool is_single_bit;
    int  bit_width;      // -1 if unknown
    bool crosses_reset_boundary; // optional (e.g. different reset domains)
};

struct CdcIssue {
    String id;                // unique issue id
    CdcSeverity severity;
    String summary;           // brief human-readable description
    String detail;            // longer explanation or hint

    // link to the crossing (if applicable)
    String crossing_id;
};

struct ClockSignalInfo {
    String signal_name;
    int domain_id;
};

struct CdcReport {
    String id;   // block id or subsystem id

    // Clock domains from PipelineMap for reference
    Vector<ClockSignalInfo> clock_domains;

    // All identified crossings
    Vector<CdcCrossing> crossings;

    // Issues (hazards / notes)
    Vector<CdcIssue> issues;
};