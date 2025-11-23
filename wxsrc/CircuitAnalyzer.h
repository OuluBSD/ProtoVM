#ifndef CIRCUITANALYZER_H
#define CIRCUITANALYZER_H

#include <vector>
#include <string>
#include "CircuitCanvas.h"
#include "CircuitAnalysis.h"  // Include the main analysis definitions

// Forward declaration
class CircuitCanvas;

// Concrete implementation of the CircuitAnalyzer
class ConcreteCircuitAnalyzer : public ::CircuitAnalyzer {
public:
    ConcreteCircuitAnalyzer(CircuitCanvas* canvas);
    
    // Override the base class methods with specific implementations
    AnalysisResult AnalyzeCircuit() override;

    // Additional methods specific to this implementation
    AnalysisResult ValidateCircuitForSimulation();
    AnalysisResult CheckForCombinatorialLoops();
    AnalysisResult CheckForFloatingInputs();
    AnalysisResult PerformTimingAnalysis();
    AnalysisResult AnalyzeCircuitComplexity();

private:
    // Private implementation details
    std::map<Component*, std::vector<Wire*>> m_componentConnections;  // Component to its connected wires
    void BuildConnectionMap();  // Build the connection map for faster analysis

    // Helper methods
    bool DetectCombinatorialLoops();
    std::vector<Component*> FindCombinatorialPath(Component* start, Component* end);
    double CalculatePropagationDelay(Wire* wire);
};

#endif // CIRCUITANALYZER_H