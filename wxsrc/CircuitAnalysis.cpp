#include "CircuitAnalysis.h"
#include <queue>
#include <set>

CircuitAnalyzer::CircuitAnalyzer(CircuitCanvas* canvas) : m_canvas(canvas) {
    if (m_canvas) {
        BuildConnectionMap();
    }
}

void CircuitAnalyzer::BuildConnectionMap() {
    if (!m_canvas) return;
    
    m_componentConnections.clear();
    
    // For each wire, determine which components it connects
    for (Wire* wire : m_canvas->GetWires()) {
        if (!wire) continue;
        
        Component* startComp = nullptr;
        Component* endComp = nullptr;
        
        // Find components that own these pins
        for (Component* comp : m_canvas->GetComponents()) {
            if (!comp) continue;
            
            // Check input pins of this component
            for (Pin& pin : comp->GetInputPins()) {
                if (&pin == wire->GetEndPin()) {
                    endComp = comp;
                }
                if (&pin == wire->GetStartPin()) {
                    startComp = comp;
                }
            }
            
            // Check output pins of this component
            for (Pin& pin : comp->GetOutputPins()) {
                if (&pin == wire->GetEndPin()) {
                    endComp = comp;
                }
                if (&pin == wire->GetStartPin()) {
                    startComp = comp;
                }
            }
        }
        
        // Add connections to the map
        if (startComp && endComp) {
            m_componentConnections[startComp].push_back(wire);
            m_componentConnections[endComp].push_back(wire);
        }
    }
}

AnalysisResult CircuitAnalyzer::AnalyzeCircuit() {
    AnalysisResult result;
    
    if (!m_canvas) {
        result.analysisSummary = "No canvas provided for analysis";
        return result;
    }
    
    std::vector<Component*>& components = m_canvas->GetComponents();
    std::vector<Wire*>& wires = m_canvas->GetWires();
    
    result.totalComponents = components.size();
    result.totalWires = wires.size();
    
    // Identify inputs and outputs
    result.inputCount = 0;
    result.outputCount = 0;
    
    for (Component* comp : components) {
        if (comp) {
            // For now, we'll consider components with no input connections as potential inputs
            // and components with no output connections as potential outputs
            // In a real implementation, we'd have specific input/output components
            if (comp->GetInputPins().empty()) {
                result.inputCount++;
            }
            if (comp->GetOutputPins().empty()) {
                result.outputCount++;
            }
        }
    }
    
    // Detect feedback loops
    std::vector<CircuitPath> loops = DetectFeedbackLoops();
    result.loopsDetected = loops.size();
    
    // Find critical paths
    result.paths = FindCriticalPaths();
    
    // Estimate propagation delay
    result.estimatedPropagationDelay = EstimateMaxPropagationDelay();
    
    // Create analysis summary
    result.analysisSummary = "Circuit Analysis:\n";
    result.analysisSummary += "- Components: " + std::to_string(result.totalComponents) + "\n";
    result.analysisSummary += "- Wires: " + std::to_string(result.totalWires) + "\n";
    result.analysisSummary += "- Input components: " + std::to_string(result.inputCount) + "\n";
    result.analysisSummary += "- Output components: " + std::to_string(result.outputCount) + "\n";
    result.analysisSummary += "- Feedback loops: " + std::to_string(result.loopsDetected) + "\n";
    result.analysisSummary += "- Estimated max propagation delay: " + std::to_string(result.estimatedPropagationDelay) + "ns\n";
    
    return result;
}

std::vector<CircuitPath> CircuitAnalyzer::DetectFeedbackLoops() {
    std::vector<CircuitPath> loops;
    
    if (!m_canvas) return loops;
    
    // Use a graph traversal algorithm to find cycles
    std::set<Component*> visited;
    std::vector<Component*> path;
    
    for (Component* comp : m_canvas->GetComponents()) {
        if (visited.find(comp) == visited.end()) {
            CircuitPath loopPath;
            if (FindLoop(comp, visited, path, loopPath)) {
                loopPath.isLoop = true;
                loops.push_back(loopPath);
            }
        }
    }
    
    return loops;
}

bool CircuitAnalyzer::FindLoop(Component* current, std::set<Component*>& visited, 
                              std::vector<Component*>& path, CircuitPath& loopPath) {
    if (!current) return false;
    
    // Add current component to path
    path.push_back(current);
    visited.insert(current);
    
    // Get connected components
    std::vector<Component*> connectedComps = FindConnectedComponents(current, true);
    
    for (Component* nextComp : connectedComps) {
        if (std::find(path.begin(), path.end(), nextComp) != path.end()) {
            // Found a back edge, which indicates a cycle
            // Create path from the back edge point to current
            auto it = std::find(path.begin(), path.end(), nextComp);
            if (it != path.end()) {
                std::vector<Component*> cycleComponents(it, path.end());
                if (!cycleComponents.empty()) {
                    loopPath.components = cycleComponents;
                    loopPath.length = cycleComponents.size();
                    loopPath.isLoop = true;
                }
                return true;
            }
        }
        
        if (visited.find(nextComp) == visited.end()) {
            if (FindLoop(nextComp, visited, path, loopPath)) {
                return true;
            }
        }
    }
    
    // Backtrack
    path.pop_back();
    return false;
}

std::vector<Component*> CircuitAnalyzer::FindConnectedComponents(Component* startComp, bool forwardOnly) {
    std::vector<Component*> connected;
    
    if (!m_canvas || !startComp) return connected;
    
    // Find all components connected to this component through wires
    for (Wire* wire : m_canvas->GetWires()) {
        if (!wire) continue;
        
        Component* comp1 = nullptr;
        Component* comp2 = nullptr;
        
        // Find which components this wire connects
        for (Component* comp : m_canvas->GetComponents()) {
            if (!comp) continue;
            
            // Check if this wire's pins belong to this component
            bool isStartPinInComp = false, isEndPinInComp = false;
            
            for (Pin& pin : comp->GetInputPins()) {
                if (&pin == wire->GetStartPin() || &pin == wire->GetEndPin()) {
                    if (&pin == wire->GetStartPin()) isStartPinInComp = true;
                    if (&pin == wire->GetEndPin()) isEndPinInComp = true;
                }
            }
            
            for (Pin& pin : comp->GetOutputPins()) {
                if (&pin == wire->GetStartPin() || &pin == wire->GetEndPin()) {
                    if (&pin == wire->GetStartPin()) isStartPinInComp = true;
                    if (&pin == wire->GetEndPin()) isEndPinInComp = true;
                }
            }
            
            if (isStartPinInComp) comp1 = comp;
            if (isEndPinInComp) comp2 = comp;
        }
        
        // If one of the components is our start component, add the other
        if (comp1 == startComp && comp2 && comp2 != startComp) {
            connected.push_back(comp2);
        } else if (comp2 == startComp && comp1 && comp1 != startComp) {
            connected.push_back(comp1);
        }
    }
    
    return connected;
}

std::vector<CircuitPath> CircuitAnalyzer::FindCriticalPaths() {
    std::vector<CircuitPath> paths;
    
    if (!m_canvas) return paths;
    
    // Find all input components
    std::vector<Component*> inputComps = GetInputComponents();
    
    // For each input, find the longest path to any output
    for (Component* input : inputComps) {
        std::set<Component*> visited;
        std::vector<Component*> currentPath;
        CircuitPath longestPath;
        
        FindLongestPath(input, visited, currentPath, longestPath);
        
        if (longestPath.length > 0) {
            paths.push_back(longestPath);
        }
    }
    
    return paths;
}

void CircuitAnalyzer::FindLongestPath(Component* current, std::set<Component*>& visited, 
                                    std::vector<Component*>& currentPath, CircuitPath& longestPath) {
    if (!current) return;
    
    visited.insert(current);
    currentPath.push_back(current);
    
    // Find connected components
    std::vector<Component*> connectedComps = FindConnectedComponents(current, true);
    
    if (connectedComps.empty()) {
        // This is an endpoint, check if this path is longer
        if (currentPath.size() > longestPath.length) {
            longestPath.components = currentPath;
            longestPath.length = currentPath.size();
            longestPath.isLoop = false;
        }
    } else {
        for (Component* next : connectedComps) {
            if (visited.find(next) == visited.end()) {
                FindLongestPath(next, visited, currentPath, longestPath);
            }
        }
    }
    
    // Backtrack
    currentPath.pop_back();
    visited.erase(current);
}

int CircuitAnalyzer::CalculateCircuitDepth() {
    int maxDepth = 0;
    
    if (!m_canvas) return maxDepth;
    
    // Find all input components
    std::vector<Component*> inputComps = GetInputComponents();
    
    // For each input, find the maximum path length
    for (Component* input : inputComps) {
        std::set<Component*> visited;
        int depth = CalculateDepthFromInput(input, visited);
        if (depth > maxDepth) maxDepth = depth;
    }
    
    return maxDepth;
}

int CircuitAnalyzer::CalculateDepthFromInput(Component* input, std::set<Component*>& visited) {
    if (!input || visited.count(input) > 0) {
        return 0;
    }
    
    visited.insert(input);
    
    std::vector<Component*> connected = FindConnectedComponents(input, true);
    int maxDepth = 0;
    
    for (Component* comp : connected) {
        int depth = CalculateDepthFromInput(comp, visited);
        if (depth > maxDepth) maxDepth = depth;
    }
    
    return 1 + maxDepth;  // +1 for the current component
}

std::vector<Component*> CircuitAnalyzer::GetInputComponents() {
    std::vector<Component*> inputs;
    
    if (!m_canvas) return inputs;
    
    for (Component* comp : m_canvas->GetComponents()) {
        if (comp) {
            // In digital circuits, inputs are typically components that don't receive signals 
            // from other components on their input pins - for now we'll consider any component
            // that has input pins but are not connected as a potential input
            bool hasUnconnectedInputs = false;
            for (const Pin& pin : comp->GetInputPins()) {
                if (!pin.IsConnected()) {
                    hasUnconnectedInputs = true;
                    break;
                }
            }
            
            if (hasUnconnectedInputs) {
                inputs.push_back(comp);
            }
        }
    }
    
    return inputs;
}

std::vector<Component*> CircuitAnalyzer::GetOutputComponents() {
    std::vector<Component*> outputs;
    
    if (!m_canvas) return outputs;
    
    for (Component* comp : m_canvas->GetComponents()) {
        if (comp) {
            // Outputs are typically components with unconnected output pins
            bool hasUnconnectedOutputs = false;
            for (const Pin& pin : comp->GetOutputPins()) {
                if (!pin.IsConnected()) {
                    hasUnconnectedOutputs = true;
                    break;
                }
            }
            
            if (hasUnconnectedOutputs) {
                outputs.push_back(comp);
            }
        }
    }
    
    return outputs;
}

double CircuitAnalyzer::EstimateMaxPropagationDelay() {
    // For a simple estimation, assume each gate has a fixed propagation delay
    // In a real implementation, this would use actual timing data
    double estimatedDelay = 0.0;
    
    if (!m_canvas) return estimatedDelay;
    
    // Count different types of components and estimate delay
    int gateCount = 0;  // Count logic gates
    
    for (Component* comp : m_canvas->GetComponents()) {
        if (comp) {
            wxString name = comp->GetName();
            if (name == "NAND" || name == "NOR" || name == "NOT" || 
                name == "BUF" || name == "AND" || name == "OR" || name == "XOR") {
                gateCount++;
            }
        }
    }
    
    // Assume average gate delay of 10ns per gate in the critical path
    // This is a very rough estimation
    return gateCount * 10.0;  // 10 ns per gate
}