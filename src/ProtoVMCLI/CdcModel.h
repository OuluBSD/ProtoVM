#pragma once

#include "PipelineModel.h"
#include <Core/Core.h>
#include "SessionTypes.h"  // For Result<T>

using namespace Upp;

enum class CdcCrossingKind {
    SingleBitSyncCandidate,
    MultiBitBundle,
    HandshakeLike,
    UnknownPattern
};

// Global-scope clock signal info (the PipelineModel variant lives in
// namespace ProtoVMCLI; CdcReport at global scope uses this one).
struct ClockSignalInfo : public Upp::Moveable<ClockSignalInfo> {
    String signal_name;
    int domain_id;
};

enum class CdcSeverity {
    Info,
    Warning,
    Error
};

struct CdcCrossingEndpoint : public Upp::Moveable<CdcCrossingEndpoint> {
    String reg_id;       // from PipelineMap::RegisterInfo::reg_id, or a special ID for non-reg endpoints
    String clock_signal; // clock signal name
    int    domain_id;    // domain id from PipelineMap
};

struct CdcCrossing : public Upp::Moveable<CdcCrossing> {
    String id;           // unique id for the crossing (e.g. "CDCC_0001")

    CdcCrossingEndpoint src;
    CdcCrossingEndpoint dst;

    CdcCrossingKind kind;

    // Simple characterization:
    bool is_single_bit;
    int  bit_width;      // -1 if unknown
    bool crosses_reset_boundary; // optional (e.g. different reset domains)
};

struct CdcIssue : public Upp::Moveable<CdcIssue> {
    String id;                // unique issue id
    CdcSeverity severity;
    String summary;           // brief human-readable description
    String detail;            // longer explanation or hint

    // link to the crossing (if applicable)
    String crossing_id;
};

struct CdcReport : public Upp::Moveable<CdcReport> {
    CdcReport() = default;
    CdcReport(const CdcReport& p)
        : Upp::Moveable<CdcReport>()
        , id(p.id)
    {
        for (int i = 0; i < p.clock_domains.GetCount(); i++) clock_domains.Add(p.clock_domains[i]);
        for (int i = 0; i < p.crossings.GetCount(); i++) crossings.Add(p.crossings[i]);
        for (int i = 0; i < p.issues.GetCount(); i++) issues.Add(p.issues[i]);
    }

    CdcReport& operator=(const CdcReport& p) {
        if (this == &p) return *this;
        id = p.id;
        clock_domains.Clear();
        for (int i = 0; i < p.clock_domains.GetCount(); i++) clock_domains.Add(p.clock_domains[i]);
        crossings.Clear();
        for (int i = 0; i < p.crossings.GetCount(); i++) crossings.Add(p.crossings[i]);
        issues.Clear();
        for (int i = 0; i < p.issues.GetCount(); i++) issues.Add(p.issues[i]);
        return *this;
    }

    String id;   // block id or subsystem id

    // Clock domains from PipelineMap for reference
    Vector<ClockSignalInfo> clock_domains;

    // All identified crossings
    Vector<CdcCrossing> crossings;

    // Issues (hazards / notes)
    Vector<CdcIssue> issues;
};