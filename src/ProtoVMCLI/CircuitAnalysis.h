#ifndef _ProtoVM_CircuitAnalysis_h_
#define _ProtoVM_CircuitAnalysis_h_

#include "CircuitData.h"
#include "CircuitDiagnostics.h"
#include "SessionTypes.h"  // For Result<T>
#include <vector>

namespace ProtoVMCLI {

class CircuitAnalysis {
public:
    Result<std::vector<CircuitDiagnostic>> AnalyzeCircuit(const CircuitData& circuit);
    
private:
    // Helper methods for specific analysis checks
    void CheckFloatingNets(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics);
    void CheckShortCircuits(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics);
    void CheckUnconnectedPins(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics);
    void CheckFanout(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics);
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CircuitAnalysis_h_