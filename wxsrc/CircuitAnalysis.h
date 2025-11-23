#ifndef CIRCUIT_ANALYSIS_H
#define CIRCUIT_ANALYSIS_H


#include <vector>
#include <string>
#include <map>

// Forward declarations
class CircuitCanvas;
class Component;
class Wire;
class CircuitAnalyzer;

// Represents a path in the circuit for analysis
struct CircuitPath {
    std::vector<Component*> components;
    std::vector<Wire*> wires;
    int length;  // Number of components in the path
    bool isLoop; // Whether this path forms a loop (feedback)
    
    CircuitPath() : length(0), isLoop(false) {}
};

// Represents circuit analysis results
struct AnalysisResult {
    int totalComponents;
    int totalWires;
    int inputCount;
    int outputCount;
    int loopsDetected;  // Number of feedback loops
    std::vector<CircuitPath> paths;  // Critical paths in the circuit
    double estimatedPropagationDelay; // Estimated max propagation delay
    std::string analysisSummary;
};

// Circuit analyzer class for advanced analysis
class CircuitAnalyzer {
public:
    CircuitAnalyzer(CircuitCanvas* canvas);
    virtual ~CircuitAnalyzer() = default;
    
    // Perform comprehensive analysis of the circuit
    virtual AnalysisResult AnalyzeCircuit();
    
    // Detect feedback loops (critical for sequential circuits)
    std::vector<CircuitPath> DetectFeedbackLoops();
    
    // Find critical paths (paths with maximum delay)
    std::vector<CircuitPath> FindCriticalPaths();
    
    // Calculate circuit complexity metrics
    int CalculateCircuitDepth();  // Maximum path length from input to output
    
    // Identify input and output components
    std::vector<Component*> GetInputComponents();
    std::vector<Component*> GetOutputComponents();
    
    // Timing analysis
    double EstimateMaxPropagationDelay();
    
protected:
    CircuitCanvas* m_canvas;
    
    // Helper methods
    std::vector<Component*> FindConnectedComponents(Component* startComp, bool forwardOnly = true);
    bool IsPathLoop(const CircuitPath& path);
    int CalculatePathLength(const CircuitPath& path);
    
private:
    // Private implementation details
    std::map<Component*, std::vector<Wire*>> m_componentConnections;  // Component to its connected wires
    void BuildConnectionMap();  // Build the connection map for faster analysis
};

#endif // CIRCUIT_ANALYSIS_H