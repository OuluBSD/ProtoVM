#include "CircuitAnalysis.h"
#include <set>
#include <map>
#include <algorithm>

namespace ProtoVMCLI {

Result<std::vector<CircuitDiagnostic>> CircuitAnalysis::AnalyzeCircuit(const CircuitData& circuit) {
    std::vector<CircuitDiagnostic> diagnostics;
    
    try {
        CheckFloatingNets(circuit, diagnostics);
        CheckShortCircuits(circuit, diagnostics);
        CheckUnconnectedPins(circuit, diagnostics);
        CheckFanout(circuit, diagnostics);
        
        return Result<std::vector<CircuitDiagnostic>>::MakeOk(diagnostics);
    }
    catch (const std::exception& e) {
        return Result<std::vector<CircuitDiagnostic>>::MakeError(
            ErrorCode::InternalError,
            std::string("Exception in AnalyzeCircuit: ") + e.what()
        );
    }
}

void CircuitAnalysis::CheckFloatingNets(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics) {
    // A "net" is defined as a wire connecting component pins
    // A net is floating if it has no driving component (no output pin connected)
    
    for (const auto& wire : circuit.wires) {
        // Find the components connected to this wire
        const ComponentData* start_comp = nullptr;
        const ComponentData* end_comp = nullptr;
        
        // Find the start component
        for (const auto& comp : circuit.components) {
            if (comp.id.id == wire.start_component_id.id) {
                start_comp = &comp;
                break;
            }
        }
        
        // Find the end component
        for (const auto& comp : circuit.components) {
            if (comp.id.id == wire.end_component_id.id) {
                end_comp = &comp;
                break;
            }
        }
        
        if (start_comp && end_comp) {
            // Check if either pin is an output (driving pin)
            bool is_driver = false;
            
            // Check start pin
            for (const auto& output : start_comp->outputs) {
                if (output.name == wire.start_pin_name) {
                    is_driver = true;
                    break;
                }
            }
            
            // Check end pin
            for (const auto& output : end_comp->outputs) {
                if (output.name == wire.end_pin_name) {
                    is_driver = true;
                    break;
                }
            }
            
            // If no driver is found, it's a floating net
            if (!is_driver) {
                CircuitDiagnostic diagnostic;
                diagnostic.severity = DiagnosticSeverity::Warning;
                diagnostic.kind = DiagnosticKind::FloatingNet;
                diagnostic.location.wire_id = wire.id.id;
                diagnostic.message = "Net " + wire.id.id + " has no driver";
                diagnostic.suggested_fix = "Connect " + wire.id.id + " to a valid output pin";
                
                diagnostics.push_back(diagnostic);
            }
        }
    }
}

void CircuitAnalysis::CheckShortCircuits(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics) {
    // A short circuit occurs when multiple output pins are directly connected
    // without proper tri-state arbitration
    
    // Group all wires by connected pins to identify potential conflicts
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> pin_connections; // component_id.pin_name -> list of connected pins
    
    for (const auto& wire : circuit.wires) {
        std::string start_key = wire.start_component_id.id + "." + wire.start_pin_name;
        std::string end_key = wire.end_component_id.id + "." + wire.end_pin_name;
        
        pin_connections[start_key].push_back({wire.end_component_id.id, wire.end_pin_name});
        pin_connections[end_key].push_back({wire.start_component_id.id, wire.start_pin_name});
    }
    
    // Check for multiple drivers on the same net
    for (const auto& connection : pin_connections) {
        std::vector<std::pair<std::string, std::string>> connected_pins = connection.second;
        
        // Identify output pins in this connection
        std::vector<std::pair<std::string, std::string>> output_pins;
        
        for (const auto& pin : connected_pins) {
            // Find the component and check if the pin is an output
            for (const auto& comp : circuit.components) {
                if (comp.id.id == pin.first) {
                    // Check if it's an output pin
                    for (const auto& output : comp.outputs) {
                        if (output.name == pin.second) {
                            output_pins.push_back(pin);
                            break;
                        }
                    }
                }
            }
        }
        
        // If multiple outputs are connected, that's a potential short circuit
        if (output_pins.size() > 1) {
            CircuitDiagnostic diagnostic;
            diagnostic.severity = DiagnosticSeverity::Error;
            diagnostic.kind = DiagnosticKind::ShortCircuit;
            diagnostic.location.wire_id = connection.first; // This is not ideal, but indicates the problematic connection
            diagnostic.message = "Multiple output drivers connected to same net: " + std::to_string(output_pins.size());
            diagnostic.suggested_fix = "Use tri-state buffers or multiplexers to avoid multiple drivers on a single net";
            
            diagnostics.push_back(diagnostic);
        }
    }
}

void CircuitAnalysis::CheckUnconnectedPins(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics) {
    // Check for input pins that are not connected to anything
    for (const auto& comp : circuit.components) {
        for (const auto& input : comp.inputs) {
            bool connected = false;
            
            for (const auto& wire : circuit.wires) {
                if ((wire.start_component_id.id == comp.id.id && wire.start_pin_name == input.name) ||
                    (wire.end_component_id.id == comp.id.id && wire.end_pin_name == input.name)) {
                    connected = true;
                    break;
                }
            }
            
            if (!connected) {
                CircuitDiagnostic diagnostic;
                diagnostic.severity = DiagnosticSeverity::Warning;
                diagnostic.kind = DiagnosticKind::UnconnectedPin;
                diagnostic.location.component_id = comp.id.id;
                diagnostic.location.pin_name = input.name;
                diagnostic.message = "Input pin " + comp.name + "." + input.name + " is not connected";
                diagnostic.suggested_fix = "Connect this input pin to a valid output pin";
                
                diagnostics.push_back(diagnostic);
            }
        }
    }
}

void CircuitAnalysis::CheckFanout(const CircuitData& circuit, std::vector<CircuitDiagnostic>& diagnostics) {
    // Check for nets driving too many inputs (basic fanout check)
    const int MAX_FANOUT = 10; // Configurable threshold
    
    // Count fanouts for each output pin
    std::map<std::string, int> fanout_counts; // "comp_id.pin_name" -> count
    
    for (const auto& wire : circuit.wires) {
        // The start pin is typically the driver (output)
        std::string driver_key = wire.start_component_id.id + "." + wire.start_pin_name;
        
        // Find the start component to make sure it's an output
        for (const auto& comp : circuit.components) {
            if (comp.id.id == wire.start_component_id.id) {
                // Check if this pin is an output
                for (const auto& output : comp.outputs) {
                    if (output.name == wire.start_pin_name) {
                        // This is a valid output driver, increment count
                        fanout_counts[driver_key]++;
                        break;
                    }
                }
            }
        }
    }
    
    // Check for any fanouts exceeding the threshold
    for (const auto& fanout : fanout_counts) {
        if (fanout.second > MAX_FANOUT) {
            // Extract component and pin from key
            size_t dot_pos = fanout.first.find('.');
            if (dot_pos != std::string::npos) {
                std::string comp_id = fanout.first.substr(0, dot_pos);
                std::string pin_name = fanout.first.substr(dot_pos + 1);
                
                CircuitDiagnostic diagnostic;
                diagnostic.severity = DiagnosticSeverity::Warning;
                diagnostic.kind = DiagnosticKind::InvalidFanout;
                diagnostic.location.component_id = comp_id;
                diagnostic.location.pin_name = pin_name;
                diagnostic.message = "Pin " + comp_id + "." + pin_name + " drives " + 
                                    std::to_string(fanout.second) + " inputs (exceeds limit of " + 
                                    std::to_string(MAX_FANOUT) + ")";
                diagnostic.suggested_fix = "Consider using buffers to reduce fanout or verify this is intentional";
                
                diagnostics.push_back(diagnostic);
            }
        }
    }
}

} // namespace ProtoVMCLI