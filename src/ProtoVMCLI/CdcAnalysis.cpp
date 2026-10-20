#include "CdcAnalysis.h"

using namespace Upp;
using namespace ProtoVMCLI;

// Resolve a numeric domain id for a clock signal name (handler-layer shim:
// the CdcCrossing endpoint carries the numeric id, the pipeline layer the name).
static int CdcCrossingDomainNum(const std::string& clock_signal) {
    return -1;
}


Result<CdcReport> CdcAnalysis::BuildCdcReportForBlock(
    const PipelineMap& pipeline,
    const CircuitGraph& graph,
    const TimingAnalysis* timing /*= nullptr*/
) {
    CdcReport report;
    report.id = pipeline.id; // Use pipeline ID as the report ID
    
    // Add clock domains from pipeline
    for (const auto& domain : pipeline.clock_domains) {
        ::ClockSignalInfo clock_info;   // global variant (CdcModel.h)
        clock_info.signal_name = Upp::String(domain.signal_name.c_str());
        clock_info.domain_id = domain.domain_id;
        report.clock_domains.Add(clock_info);
    }
    
    // Find register-to-register paths that cross clock domains
    int crossing_counter = 0;
    for (const auto& path : pipeline.reg_paths) {
        if (path.crosses_clock_domain) {
            CdcCrossing crossing;
            crossing.id = Upp::String("CDCC_") + Upp::Format("%04d", crossing_counter++);
            
            // Source endpoint
            crossing.src.reg_id = path.src_reg_id;
            crossing.src.clock_signal = path.src_clock_domain;
            crossing.src.domain_id = CdcCrossingDomainNum(path.src_clock_domain);
            
            // Destination endpoint
            crossing.dst.reg_id = path.dst_reg_id;
            crossing.dst.clock_signal = path.dst_clock_domain;
            crossing.dst.domain_id = CdcCrossingDomainNum(path.dst_clock_domain);
            
            // Determine bit width (simplified - in practice this might require more analysis)
            crossing.is_single_bit = path.width == 1;
            crossing.bit_width = path.width;
            crossing.crosses_reset_boundary = false; // Simplified for now
            
            // Classify the crossing
            crossing.kind = ClassifyCrossing(crossing, graph);
            
            // Add the crossing to the report
            report.crossings.Add(crossing);
            
            // Create an issue for this crossing
            CdcIssue issue;
            issue.id = Upp::String("CDCISS_") + Upp::Format("%04d", report.issues.GetCount());
            issue.severity = DetermineSeverity(crossing.kind);
            issue.summary = GenerateSummary(crossing.kind, crossing.src, crossing.dst, crossing.bit_width);
            issue.detail = GenerateDetail(crossing.kind, crossing.src, crossing.dst, crossing.bit_width);
            issue.crossing_id = crossing.id;
            
            report.issues.Add(issue);
        }
    }
    
    return report;
}

Result<CdcReport> CdcAnalysis::BuildCdcReportForSubsystem(
    const PipelineMap& pipeline,
    const CircuitGraph& graph,
    const TimingAnalysis* timing /*= nullptr*/
) {
    // For subsystems, we can aggregate the same information as for blocks
    // This is a simplified implementation - in practice, this might aggregate across multiple blocks
    return BuildCdcReportForBlock(pipeline, graph, timing);
}

CdcCrossingKind CdcAnalysis::ClassifyCrossing(const CdcCrossing& crossing, 
                                             const CircuitGraph& graph) {
    // Classification heuristics:
    // 1. Single-bit signals that don't have obvious bundling are SingleBitSyncCandidate
    // 2. Multi-bit buses are MultiBitBundle
    // 3. Special patterns like handshake signals might be HandshakeLike
    // 4. Everything else is UnknownPattern
    
    if (crossing.is_single_bit && crossing.bit_width == 1) {
        // Check if it's part of a bundle (if there are other related signals crossing together)
        // For simplicity, assuming single bit crossing without other related crossings
        return CdcCrossingKind::SingleBitSyncCandidate;
    } else if (crossing.bit_width > 1) {
        // Multi-bit signal crossing
        return CdcCrossingKind::MultiBitBundle;
    }
    
    // Check for handshake-like patterns (simplified check)
    // Look for signals with names like *valid/*ready, *req/*ack, etc.
    if (crossing.src.reg_id.Find("valid") >= 0 && crossing.dst.reg_id.Find("ready") >= 0) {
        return CdcCrossingKind::HandshakeLike;
    } else if (crossing.src.reg_id.Find("req") >= 0 && crossing.dst.reg_id.Find("ack") >= 0) {
        return CdcCrossingKind::HandshakeLike;
    } else if (crossing.src.reg_id.Find("request") >= 0 && crossing.dst.reg_id.Find("acknowledge") >= 0) {
        return CdcCrossingKind::HandshakeLike;
    }
    
    // Default to unknown pattern
    return CdcCrossingKind::UnknownPattern;
}

CdcSeverity CdcAnalysis::DetermineSeverity(CdcCrossingKind kind) {
    switch (kind) {
        case CdcCrossingKind::SingleBitSyncCandidate:
            return CdcSeverity::Warning;  // Safe if proper synchronizer exists
        case CdcCrossingKind::MultiBitBundle:
            return CdcSeverity::Error;    // Multi-bit CDC without verification is risky
        case CdcCrossingKind::HandshakeLike:
            return CdcSeverity::Info;     // Handshake protocols can be safe if designed properly
        case CdcCrossingKind::UnknownPattern:
        default:
            return CdcSeverity::Warning;  // Unknown patterns should be reviewed
    }
}

std::string CdcAnalysis::GenerateSummary(CdcCrossingKind kind, 
                                  const CdcCrossingEndpoint& src, 
                                  const CdcCrossingEndpoint& dst,
                                  int bit_width) {
    switch (kind) {
        case CdcCrossingKind::SingleBitSyncCandidate:
            return "Single-bit CDC from " + src.clock_signal.ToStd() + " to " + dst.clock_signal.ToStd() + ".";
        case CdcCrossingKind::MultiBitBundle:
            return std::to_string(bit_width) + "-bit CDC bundle from " + src.clock_signal.ToStd() + " to " + dst.clock_signal.ToStd() + ".";
        case CdcCrossingKind::HandshakeLike:
            return "Handshake-like CDC from " + src.clock_signal.ToStd() + " to " + dst.clock_signal.ToStd() + ".";
        case CdcCrossingKind::UnknownPattern:
        default:
            return "Unknown CDC pattern from " + src.clock_signal.ToStd() + " to " + dst.clock_signal.ToStd() + ".";
    }
}

std::string CdcAnalysis::GenerateDetail(CdcCrossingKind kind, 
                                 const CdcCrossingEndpoint& src, 
                                 const CdcCrossingEndpoint& dst,
                                 int bit_width) {
    switch (kind) {
        case CdcCrossingKind::SingleBitSyncCandidate:
            return "Single-bit control signal crossing clock domains. This is typically safe with a 2-flop synchronizer.";
        case CdcCrossingKind::MultiBitBundle:
            if (bit_width > 0) {
                return std::string("Multi-bit (" + std::to_string(bit_width) + " bits) register crossing clock domains without recognized safe structure. Consider using async FIFO, Gray code encoding, or other multi-bit CDC techniques.");
            } else {
                return std::string("Multi-bit register crossing clock domains without recognized safe structure. Consider using async FIFO, Gray code encoding, or other multi-bit CDC techniques.");
            }
        case CdcCrossingKind::HandshakeLike:
            return std::string("Signal pair resembles ready/valid or request/ack pattern. Verify that the handshake protocol is correctly designed for clock domain crossing.");
        case CdcCrossingKind::UnknownPattern:
        default:
            return std::string("Signal crossing clock domains with an unrecognized pattern. Review this crossing to ensure proper synchronizer implementation.");
    }
}
