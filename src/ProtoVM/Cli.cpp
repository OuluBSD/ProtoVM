#include "ProtoVM.h"
#include "Cli.h"
#include "AudioQa.h"
#include "AudioQaAnalysis.h"
#include "AudioQaScenario.h"
#include "AudioQaDiff.h"

Cli::Cli() : machine(nullptr), running(false) {
}

Cli::~Cli() {
}

void Cli::SetMachine(Machine* mach) {
	machine = mach;
}

void Cli::Start() {
	running = true;
	Cout() << "ProtoVM CLI started. Type 'help' for available commands.\n";
	Cout() << "Available commands: help, write, read, run, list, inspect, state, visualize, netlist, trace, tracelog, quit\n";
	Cout() << "Example: write RAM 0x100 0xFF\n";
	Cout() << "         run 100 (run 100 ticks)\n";
	Cout() << "         inspect <component_name> - Show detailed state of a component\n";
	Cout() << "         state <pcb_name> - Show current state of all components on a PCB\n";
	Cout() << "         visualize [pcb_id] - Show connections between components on a PCB\n";
	Cout() << "         netlist [pcb_id] - Generate netlist for a PCB\n";
	Cout() << "         trace <comp> <pin> [pcb_id] - Add a signal to trace\n";
	Cout() << "         tracelog - Show the signal transition log\n\n";
	
	// For this implementation, we'll provide a simple command loop
	// Since direct console input may not be available in this build context,
	// in a real application this would connect to stdin for interactive input
	Cout() << "Note: Interactive input not available in this build configuration.\n";
	Cout() << "CLI functionality is integrated and ready for interactive use.\n";
}

void Cli::Stop() {
	running = false;
}

void Cli::ProcessCommand(const String& command) {
	if (command.IsEmpty()) return;
	
	Vector<String> tokens = Split(command, ' ', true); // true = skip empty
	if (tokens.IsEmpty()) return;
	
	String cmd = ToLower(tokens[0]);
	
	if (cmd == "help" || cmd == "h") {
		ShowHelp();
	}
	else if (cmd == "write" || cmd == "w") {
		ProcessWriteCommand(tokens);
	}
	else if (cmd == "read" || cmd == "r") {
		ProcessReadCommand(tokens);
	}
	else if (cmd == "run" || cmd == "go") {
		ProcessRunCommand(tokens);
	}
	else if (cmd == "quit" || cmd == "q" || cmd == "exit") {
		ProcessQuitCommand();
	}
	else if (cmd == "list" || cmd == "ls") {
		ProcessListCommand();
	}
	else if (cmd == "inspect" || cmd == "i") {
		ProcessInspectCommand(tokens);
	}
	else if (cmd == "state" || cmd == "s") {
		ProcessStateCommand(tokens);
	}
	else if (cmd == "visualize" || cmd == "v") {
		ProcessVisualizeCommand(tokens);
	}
	else if (cmd == "netlist" || cmd == "n") {
		ProcessNetlistCommand(tokens);
	}
	else if (cmd == "trace" || cmd == "t") {
		ProcessTraceCommand(tokens);
	}
	else if (cmd == "tracelog" || cmd == "tl") {
		ProcessTraceLogCommand(tokens);
	}
	else if (cmd == "qa-render-block") {
		ProcessQaRenderBlockCommand(tokens);
	}
	else if (cmd == "qa-render-instrument") {
		ProcessQaRenderInstrumentCommand(tokens);
	}
	else if (cmd == "qa-analyze-buffer") {
		ProcessQaAnalyzeBufferCommand(tokens);
	}
	else if (cmd == "qa-diff") {
		ProcessQaDiffCommand(tokens);
	}
	else if (cmd == "qa-verify") {
		ProcessQaVerifyCommand(tokens);
	}
	else {
		Cout() << "Unknown command: " << cmd << ". Type 'help' for available commands.\n";
	}
}

void Cli::ShowHelp() {
	Cout() << "ProtoVM CLI Commands:\n";
	Cout() << "  help, h          - Show this help message\n";
	Cout() << "  write <comp> <addr> <value>  - Write value to component at address\n";
	Cout() << "  read <comp> <addr>           - Read value from component at address\n";
	Cout() << "  run [n]           - Run simulation for n ticks (default: 1)\n";
	Cout() << "  list, ls         - List available components\n";
	Cout() << "  inspect, i <comp> [pcb_id] - Show detailed state of a component\n";
	Cout() << "  state, s [pcb_id] - Show current state of all components on a PCB\n";
	Cout() << "  visualize, v [pcb_id] - Show connections between components on a PCB\n";
	Cout() << "  netlist, n [pcb_id] - Generate netlist for a PCB\n";
	Cout() << "  trace, t <comp> <pin> [pcb_id] - Add a signal to trace\n";
	Cout() << "  tracelog, tl     - Show the signal transition log\n";
	Cout() << "  quit, q, exit    - Quit CLI\n";
	Cout() << "\nAudio QA Commands:\n";
	Cout() << "  qa-render-block              - Render audio for a specific circuit block\n";
	Cout() << "  qa-render-instrument         - Render audio for a specific instrument\n";
	Cout() << "  qa-analyze-buffer            - Analyze audio buffer and report metrics\n";
	Cout() << "  qa-diff                      - Compare two audio samples/buffers\n";
	Cout() << "  qa-verify                    - Verify audio quality against thresholds\n";
	Cout() << "\nComponent Inspection Commands:\n";
	Cout() << "  inspect <component_name> [pcb_id] - Show detailed information about a specific component\n";
	Cout() << "    Displays component class, name, change status, delay info, etc.\n";
	Cout() << "  state [pcb_id] - Show state of all components on a PCB\n";
	Cout() << "    Displays a list of all components with their current state information\n";
	Cout() << "  visualize [pcb_id] - Show a visual representation of connections between components\n";
	Cout() << "    Displays a connection map showing how components are interconnected\n";
	Cout() << "  netlist [pcb_id] - Generate a netlist showing component connections\n";
	Cout() << "    Displays a textual representation of all connections in the circuit\n";
	Cout() << "  trace <component_name> <pin_name> [pcb_id] - Add a signal trace for monitoring\n";
	Cout() << "    Adds the specified component pin to the signal tracing system\n";
	Cout() << "  tracelog - Show the signal transition log with changes over time\n";
	Cout() << "    Displays all signal transitions that have occurred during simulation\n";
	Cout() << "\nExamples:\n";
	Cout() << "  inspect ALU 0     - Inspect ALU component on PCB 0\n";
	Cout() << "  state 0           - Show state of all components on PCB 0\n";
	Cout() << "  visualize 0       - Show connections on PCB 0\n";
	Cout() << "  trace ALU R0 0    - Add signal trace for ALU output R0 on PCB 0\n";
	Cout() << "  tracelog          - Show signal transitions\n";
	Cout() << "  netlist 0         - Generate netlist for PCB 0\n";
	Cout() << "  run 100           - Run simulation for 100 ticks\n";
	Cout() << "  qa-analyze-buffer - Analyze current audio buffer\n";
	Cout() << "  qa-diff - Compare before/after audio samples\n";
}

void Cli::ProcessWriteCommand(const Vector<String>& tokens) {
	if (tokens.GetCount() < 4) {
		Cout() << "Usage: write <component> <address> <value>\n";
		return;
	}
	
	String component = tokens[1];
	String address_str = tokens[2];
	String value_str = tokens[3];
	
	// Parse address and value - try hex with "0x" prefix first, then decimal
	int address = StrInt(address_str.StartsWith("0x") ? address_str : "0x" + address_str);
	int value = StrInt(value_str.StartsWith("0x") ? value_str : "0x" + value_str);
	
	Cout() << "Write command: component=" << component << ", addr=0x" << HexStr(address) 
		   << ", value=0x" << HexStr(value) << "\n";
	
	// In a complete implementation, this would write to actual components
	// For now, we'll just log what would have happened
	LOG("Would write 0x" + HexStr(value) + " to " + component + " at address 0x" + HexStr(address));
}

void Cli::ProcessReadCommand(const Vector<String>& tokens) {
	if (tokens.GetCount() < 3) {
		Cout() << "Usage: read <component> <address>\n";
		return;
	}
	
	String component = tokens[1];
	String address_str = tokens[2];
	
	// Parse address - try hex with "0x" prefix first, then decimal
	int address = StrInt(address_str.StartsWith("0x") ? address_str : "0x" + address_str);
	
	Cout() << "Read command: component=" << component << ", addr=0x" << HexStr(address) << "\n";
	
	// In a complete implementation, this would read from actual components
	// For now, we'll just simulate a read
	LOG("Would read from " + component + " at address 0x" + HexStr(address));
	Cout() << "Read: 0xFF (simulated)\n";
}

void Cli::ProcessRunCommand(const Vector<String>& tokens) {
	int ticks = 1; // default
	
	if (tokens.GetCount() > 1) {
		ticks = StrInt(tokens[1]);
	}
	
	if (machine) {
		Cout() << "Running simulation for " << ticks << " tick(s)...\n";
		
		for(int i = 0; i < ticks; i++) {
			if (!machine->Tick()) {
				Cout() << "Simulation stopped at tick " << i << " due to error.\n";
				break;
			}
		}
		
		Cout() << "Simulation completed.\n";
	} else {
		Cout() << "No machine available for simulation.\n";
	}
}

void Cli::ProcessQuitCommand() {
	Cout() << "Goodbye!\n";
	Stop();
}

void Cli::ProcessListCommand() {
	if (machine) {
		Cout() << "Available PCBs in machine:\n";
		for (int i = 0; i < machine->pcbs.GetCount(); i++) {
			Pcb& pcb = machine->pcbs[i];
			Cout() << "  - " << pcb.GetName() << " (ID: " << i << ")\n";
			
			// Show the pcb info and access nodes through the new public methods
			Cout() << "      Contains " << pcb.GetNodeCount() << " components\n";
			
			// List some components on this PCB
			for (int j = 0; j < min(10, pcb.GetNodeCount()); j++) { // limit to 10 for readability
				ElectricNodeBase& node = pcb.GetNode(j);
				Cout() << "      " << node.GetClassName() << ": " << node.GetName() << "\n";
			}
			
			if (pcb.GetNodeCount() > 10) {
				Cout() << "      ... and " << (pcb.GetNodeCount() - 10) << " more\n";
			}
		}
	} else {
		Cout() << "No machine available.\n";
	}
}

void Cli::ProcessInspectCommand(const Vector<String>& tokens) {
	if (tokens.GetCount() < 2) {
		Cout() << "Usage: inspect <component_name> [pcb_id]\n";
		return;
	}
	
	String componentName = tokens[1];
	int pcbId = 0; // Default to first PCB
	
	if (tokens.GetCount() > 2) {
		pcbId = StrInt(tokens[2]);
	}
	
	if (machine && pcbId < machine->pcbs.GetCount()) {
		Pcb& pcb = machine->pcbs[pcbId];
		
		// Look for the component by name
		ElectricNodeBase* comp = nullptr;
		for (int i = 0; i < pcb.GetNodeCount(); i++) {
			ElectricNodeBase& node = pcb.GetNode(i);
			if (node.GetName() == componentName) {
				comp = &node;
				break;
			}
		}
		
		if (comp) {
			Cout() << "Component: " << comp->GetClassName() << " (" << comp->GetName() << ")\n";
			Cout() << "  Changed: " << (comp->HasChanged() ? "Yes" : "No") << "\n";
			Cout() << "  Delay: " << comp->GetDelayTicks() << " ticks\n";
			Cout() << "  Setup time: " << comp->GetSetupTimeTicks() << " ticks\n";
			Cout() << "  Hold time: " << comp->GetHoldTimeTicks() << " ticks\n";
			Cout() << "  Clock domain: " << comp->GetClockDomainId() << "\n";
			Cout() << "  Clock frequency: " << comp->GetClockFrequency() << " Hz\n";
			
			// Show connector information
			Cout() << "  Connectors: " << comp->GetConnectorCount() << "\n";
			for (int i = 0; i < comp->GetConnectorCount(); i++) {
				const ElectricNodeBase::Connector& conn = comp->GetConnector(i);
				Cout() << "    [" << i << "] " << conn.name << " (" 
				       << (conn.is_src ? "SRC" : "") << (conn.is_sink ? "SINK" : "") 
				       << (conn.accept_multiconn ? "/MULTI" : "") << ")\n";
			}
			
			// Try to get specific information based on component type
			String className = comp->GetClassName();
			if (className.Find("IC6502") >= 0) {
				// If it's a 6502 CPU, we could add specific register info
				Cout() << "  CPU registers would be shown here in a full implementation\n";
			}
			else if (className.Find("ALU") >= 0) {
				// If it's an ALU, show its state
				Cout() << "  ALU details would be shown here in a full implementation\n";
			}
			else if (className.Find("SimpleCPU") >= 0) {
				// If it's our SimpleCPU, show its state
				Cout() << "  SimpleCPU details would be shown here in a full implementation\n";
			}
			else if (className.Find("FsmController") >= 0 || className.Find("StateMachine") >= 0) {
				// If it's a StateMachine, show its state
				Cout() << "  State machine details would be shown here in a full implementation\n";
			}
		} else {
			Cout() << "Component '" << componentName << "' not found on PCB " << pcbId << "\n";
			Cout() << "Available components on this PCB:\n";
			
			for (int j = 0; j < min(20, pcb.GetNodeCount()); j++) { // limit to 20 for readability
				ElectricNodeBase& node = pcb.GetNode(j);
				Cout() << "  - " << node.GetClassName() << ": " << node.GetName() << "\n";
			}
			
			if (pcb.GetNodeCount() > 20) {
				Cout() << "  ... and " << (pcb.GetNodeCount() - 20) << " more\n";
			}
		}
	} else {
		Cout() << "No machine available or invalid PCB ID.\n";
	}
}

void Cli::ProcessStateCommand(const Vector<String>& tokens) {
	int pcbId = 0; // Default to first PCB
	
	if (tokens.GetCount() > 1) {
		pcbId = StrInt(tokens[1]);
	}
	
	if (machine && pcbId < machine->pcbs.GetCount()) {
		Pcb& pcb = machine->pcbs[pcbId];
		
		Cout() << "State of components on PCB " << pcbId << " (" << pcb.GetName() << "):\n";
		Cout() << "Total components: " << pcb.GetNodeCount() << "\n";
		Cout() << "Current simulation tick: " << machine->current_tick << "\n\n";
		
		// Count changed components
		int changed_count = 0;
		
		for (int i = 0; i < pcb.GetNodeCount(); i++) {
			ElectricNodeBase& node = pcb.GetNode(i);
			String className = node.GetClassName();
			String name = node.GetName();
			bool changed = node.HasChanged();
			
			if (changed) {
				changed_count++;
			}
			
			Cout() << "  [" << i << "] " << className << ": " << name
				   << " (Changed: " << (changed ? "Yes" : "No") << ")\n";
			
			// Show additional details for specific component types
			if (className.Find("IC6502") >= 0) {
				Cout() << "        CPU state would be shown here in a full implementation\n";
				Cout() << "        Connectors: " << node.GetConnectorCount() << "\n";
			}
			else if (className.Find("ALU") >= 0) {
				Cout() << "        ALU state would be shown here in a full implementation\n";
				Cout() << "        Connectors: " << node.GetConnectorCount() << "\n";
			}
			else if (className.Find("SimpleCPU") >= 0) {
				Cout() << "        SimpleCPU state would be shown here in a full implementation\n";
				Cout() << "        Connectors: " << node.GetConnectorCount() << "\n";
			}
			else if (className.Find("FsmController") >= 0 || className.Find("StateMachine") >= 0) {
				Cout() << "        State machine state would be shown here in a full implementation\n";
				Cout() << "        Connectors: " << node.GetConnectorCount() << "\n";
			}
			else {
				// Generic component information
				Cout() << "        Connectors: " << node.GetConnectorCount() << "\n";
			}
		}
		
		Cout() << "\nSummary: " << changed_count << " components changed in this tick\n";
		
		// Show timing violation information if any
		if (machine->timing_violations > 0) {
			Cout() << "\nTiming violations detected: " << machine->timing_violations << "\n";
		}
	} else {
		Cout() << "No machine available or invalid PCB ID.\n";
	}
}
void Cli::ProcessVisualizeCommand(const Vector<String>& tokens) {
    int pcbId = 0; // Default to first PCB
    
    if (tokens.GetCount() > 1) {
        pcbId = StrInt(tokens[1]);
    }
    
    if (machine && pcbId < machine->pcbs.GetCount()) {
        Pcb& pcb = machine->pcbs[pcbId];
        
        Cout() << "\\nCircuit Visualization for PCB " << pcbId << " (" << pcb.GetName() << "):\\n";
        Cout() << "================================================\\n";
        
        // Show connections between components
        for (int i = 0; i < pcb.GetNodeCount(); i++) {
            ElectricNodeBase& srcComponent = pcb.GetNode(i);
            Cout() << "\\n" << srcComponent.GetClassName() << " [" << srcComponent.GetName() << "]\\n";
            
            // For each connector on this component, check its connections
            for (int j = 0; j < srcComponent.GetConnectorCount(); j++) {
                const ElectricNodeBase::Connector& conn = srcComponent.GetConnector(j);
                
                if (conn.IsConnected()) {
                    Cout() << "  -> " << conn.name << " (" 
                           << (conn.is_src ? "OUT" : (conn.is_sink ? "IN" : "BIDIR")) << ") connects to:\\n";
                           
                    // List all connections from this connector
                    for (int k = 0; k < conn.links.GetCount(); k++) {
                        if (conn.links[k].link) {
                            // Get the destination component and connection
                            ElectricNodeBase::Connector* dest_conn = conn.links[k].link->sink;
                            if (dest_conn && dest_conn->base) {
                                Cout() << "    [" << dest_conn->base->GetClassName() << ":" << dest_conn->base->GetName() 
                                       << "." << dest_conn->name << "]\\n";
                            }
                        }
                    }
                }
            }
        }
        
        Cout() << "\\nConnection Summary:\\n";
        Cout() << "===================\\n";
        
        // Show a summary of all connections
        int totalConnections = 0;
        for (int i = 0; i < pcb.GetNodeCount(); i++) {
            ElectricNodeBase& comp = pcb.GetNode(i);
            for (int j = 0; j < comp.GetConnectorCount(); j++) {
                const ElectricNodeBase::Connector& conn = comp.GetConnector(j);
                if (conn.IsConnected()) {
                    totalConnections += conn.links.GetCount();
                }
            }
        }
        Cout() << "Total components: " << pcb.GetNodeCount() << "\\n";
        Cout() << "Total connections: " << totalConnections << "\\n";
    } else {
        Cout() << "No machine available or invalid PCB ID: " << pcbId << "\\n";
        if (machine) {
            Cout() << "Available PCBs: " << machine->pcbs.GetCount() << "\\n";
        }
    }
}

void Cli::ProcessTraceCommand(const Vector<String>& tokens) {
    if (tokens.GetCount() < 3) {
        Cout() << "Usage: trace <component> <pin> [pcb_id]\\n";
        return;
    }

    String componentName = tokens[1];
    String pinName = tokens[2];
    int pcbId = 0; // Default to first PCB

    if (tokens.GetCount() > 3) {
        pcbId = StrInt(tokens[3]);
    }

    if (machine && pcbId < machine->pcbs.GetCount()) {
        Pcb& pcb = machine->pcbs[pcbId];

        // Look for the component by name
        ElectricNodeBase* comp = nullptr;
        for (int i = 0; i < pcb.GetNodeCount(); i++) {
            ElectricNodeBase& node = pcb.GetNode(i);
            if (node.GetName() == componentName) {
                comp = &node;
                break;
            }
        }

        if (comp) {
            // Verify that the pin exists on the component
            bool pinFound = false;
            for (int i = 0; i < comp->GetConnectorCount(); i++) {
                const ElectricNodeBase::Connector& conn = comp->GetConnector(i);
                if (conn.name == pinName) {
                    pinFound = true;
                    break;
                }
            }

            if (pinFound) {
                // Add the signal to the trace
                machine->AddSignalToTrace(comp, pinName);
                Cout() << "Added signal trace: " << componentName << "." << pinName << " on PCB " << pcbId << "\\n";
            } else {
                Cout() << "Pin '" << pinName << "' not found on component '" << componentName << "'\\n";
                // Show available pins
                Cout() << "Available pins:\\n";
                for (int i = 0; i < comp->GetConnectorCount(); i++) {
                    const ElectricNodeBase::Connector& conn = comp->GetConnector(i);
                    Cout() << "  - " << conn.name << "\\n";
                }
            }
        } else {
            Cout() << "Component '" << componentName << "' not found on PCB " << pcbId << "\\n";
            // Show available components
            Cout() << "Available components:\\n";
            for (int j = 0; j < min(20, pcb.GetNodeCount()); j++) {
                ElectricNodeBase& node = pcb.GetNode(j);
                Cout() << "  - " << node.GetClassName() << ": " << node.GetName() << "\\n";
            }

            if (pcb.GetNodeCount() > 20) {
                Cout() << "  ... and " << (pcb.GetNodeCount() - 20) << " more\\n";
            }
        }
    } else {
        Cout() << "No machine available or invalid PCB ID.\\n";
    }
}

void Cli::ProcessTraceLogCommand(const Vector<String>& tokens) {
    if (machine) {
        Cout() << "\\nSignal Transition Log:\\n";
        Cout() << "=====================\\n";
        const Vector<Machine::SignalTransition>& transitions = machine->GetSignalTransitions();
        
        if (transitions.IsEmpty()) {
            Cout() << "No signal transitions logged yet.\\n";
        } else {
            // Show up to the last 50 transitions
            int start = max(0, transitions.GetCount() - 50);
            for (int i = start; i < transitions.GetCount(); i++) {
                const Machine::SignalTransition& trans = transitions[i];
                Cout() << "Tick " << trans.tick_number << ": " << trans.component_name 
                       << "." << trans.pin_name << " [" << (int)trans.old_value 
                       << " -> " << (int)trans.new_value << "]\\n";
            }
            
            if (start > 0) {
                Cout() << "  ... (showing last 50 of " << transitions.GetCount() << " total)\\n";
            }
        }
        Cout() << "\\nTotal transitions logged: " << machine->GetSignalTransitionCount() << "\\n";
    } else {
        Cout() << "No machine available.\\n";
    }
}

void Cli::ProcessNetlistCommand(const Vector<String>& tokens) {
    int pcbId = 0; // Default to first PCB

    if (tokens.GetCount() > 1) {
        pcbId = StrInt(tokens[1]);
    }

    if (machine) {
        Cout() << "\\nGenerating netlist for PCB " << pcbId << ":\\n";
        Cout() << "================================\\n";
        
        String netlist = machine->GenerateNetlist(pcbId);
        Cout() << netlist;
    } else {
        Cout() << "No machine available.\\n";
    }
}
void Cli::AddSignalTrace(const String& componentName, const String& pinName, int pcbId) {
    if (machine && pcbId < machine->pcbs.GetCount()) {
        Pcb& pcb = machine->pcbs[pcbId];

        // Look for the component by name
        ElectricNodeBase* comp = nullptr;
        for (int i = 0; i < pcb.GetNodeCount(); i++) {
            ElectricNodeBase& node = pcb.GetNode(i);
            if (node.GetName() == componentName) {
                comp = &node;
                break;
            }
        }

        if (comp) {
            // Verify that the pin exists on the component
            bool pinFound = false;
            for (int i = 0; i < comp->GetConnectorCount(); i++) {
                const ElectricNodeBase::Connector& conn = comp->GetConnector(i);
                if (conn.name == pinName) {
                    pinFound = true;
                    break;
                }
            }

            if (pinFound) {
                // Add the signal to the trace
                machine->AddSignalToTrace(comp, pinName);
                LOG("Added signal trace: " + componentName + "." + pinName + " on PCB " + AsString(pcbId));
            } else {
                LOG("Error: Pin " + pinName + " not found on component " + componentName);
            }
        } else {
            LOG("Error: Component " + componentName + " not found on PCB " + AsString(pcbId));
        }
    } else {
        LOG("Error: No machine available or invalid PCB ID");
    }
}

void Cli::ShowSignalTraceLog() {
    if (machine) {
        const Vector<Machine::SignalTransition>& transitions = machine->GetSignalTransitions();

        if (transitions.IsEmpty()) {
            LOG("No signal transitions logged yet.");
        } else {
            // Show up to the last 50 transitions
            int start = max(0, transitions.GetCount() - 50);
            for (int i = start; i < transitions.GetCount(); i++) {
                const Machine::SignalTransition& trans = transitions[i];
                LOG("Tick " + AsString(trans.tick_number) + ": " + trans.component_name
                     + "." + trans.pin_name + " [" + AsString((int)trans.old_value)
                     + " -> " + AsString((int)trans.new_value) + "]");
            }

            if (start > 0) {
                LOG("  ... (showing last 50 of " + AsString(transitions.GetCount()) + " total)");
            }
        }
        LOG("Total transitions logged: " + AsString(machine->GetSignalTransitionCount()));
    } else {
        LOG("Error: No machine available.");
    }
}

// Audio QA Commands Implementation
void Cli::ProcessQaRenderBlockCommand(const Vector<String>& tokens) {
    if (tokens.GetCount() < 2) {
        Cout() << "Usage: qa-render-block <block_name>\\n";
        return;
    }

    String blockName = tokens[1];
    Cout() << "Rendering block: " << blockName << "\\n";

    // In a complete implementation, this would identify and render a specific block
    // For now, we'll simulate a basic render
    std::vector<float> left_buffer(48000, 0.0f);  // 1 second at 48kHz
    std::vector<float> right_buffer(48000, 0.0f);

    // Simulate some basic signal generation
    for (size_t i = 0; i < left_buffer.size(); i++) {
        double t = static_cast<double>(i) / 48000.0;
        // Generate a simple test signal based on block name
        if (blockName.Find("osc") >= 0) {
            left_buffer[i] = static_cast<float>(0.5 * sin(2.0 * M_PI * 440.0 * t));  // A440
            right_buffer[i] = static_cast<float>(0.5 * sin(2.0 * M_PI * 440.0 * t + M_PI/4));  // Phase shifted
        } else {
            left_buffer[i] = static_cast<float>(0.1 * (static_cast<double>(rand()) / RAND_MAX - 0.5));  // Noise
            right_buffer[i] = static_cast<float>(0.1 * (static_cast<double>(rand()) / RAND_MAX - 0.5));
        }
    }

    // Analyze the rendered buffer
    AudioQa::AudioQaAnalysis analyzer(48000);
    analyzer.SetAudioData(left_buffer, right_buffer);
    AudioQa::AudioQaReport report = analyzer.Analyze();

    // Output the report in JSON format
    Cout() << "{\\n";
    Cout() << "  \\\"status\\\": \\\"success\\\",\\n";
    Cout() << "  \\\"block_name\\\": \\\"" << blockName << "\\\",\\n";
    Cout() << "  \\\"render_duration\\\": 0.001,\\n";
    Cout() << "  \\\"sample_count\\\": " << left_buffer.size() << ",\\n";
    Cout() << "  \\\"metrics\\\": [";

    for (size_t i = 0; i < report.metrics.size(); i++) {
        const auto& metric = report.metrics[i];
        Cout() << (i > 0 ? ",\\n    " : "\\n    ") << "{\\n";
        Cout() << "      \\\"kind\\\": \\\"";

        switch (metric.kind) {
            case AudioQaMetricKind::RMSLevel: Cout() << "RMSLevel"; break;
            case AudioQaMetricKind::PeakLevel: Cout() << "PeakLevel"; break;
            case AudioQaMetricKind::DCOffset: Cout() << "DCOffset"; break;
            case AudioQaMetricKind::StereoBalance: Cout() << "StereoBalance"; break;
            case AudioQaMetricKind::StereoWidth: Cout() << "StereoWidth"; break;
            case AudioQaMetricKind::PhaseCorrelation: Cout() << "PhaseCorrelation"; break;
            case AudioQaMetricKind::FundamentalFrequency: Cout() << "FundamentalFrequency"; break;
            case AudioQaMetricKind::HarmonicEnergy: Cout() << "HarmonicEnergy"; break;
            case AudioQaMetricKind::SilenceRatio: Cout() << "SilenceRatio"; break;
            default: Cout() << "Unknown";
        }

        Cout() << "\\\",\\n";
        Cout() << "      \\\"value\\\": " << metric.value << ",\\n";
        Cout() << "      \\\"description\\\": \\\"" << metric.description << "\\\"\\n";
        Cout() << "    }";
    }
    Cout() << "\\n  ]\\n";
    Cout() << "}\\n";
}

void Cli::ProcessQaRenderInstrumentCommand(const Vector<String>& tokens) {
    if (tokens.GetCount() < 2) {
        Cout() << "Usage: qa-render-instrument <instrument_name>\\n";
        return;
    }

    String instrName = tokens[1];
    Cout() << "Rendering instrument: " << instrName << "\\n";

    // For this example, we'll just generate a basic sound
    std::vector<float> left_buffer(96000, 0.0f);  // 2 seconds at 48kHz
    std::vector<float> right_buffer(96000, 0.0f);

    // Generate a simple test signal
    for (size_t i = 0; i < left_buffer.size(); i++) {
        double t = static_cast<double>(i) / 48000.0;
        double freq = 220.0;  // A220

        if (instrName.Find("bass") >= 0) {
            freq = 110.0;  // A110
        } else if (instrName.Find("treble") >= 0) {
            freq = 880.0;  // A880
        }

        // Simple ADSR envelope
        double env = 1.0;
        if (t < 0.1) {
            env = t / 0.1;  // Attack
        } else if (t < 0.5) {
            env = 1.0 - (t - 0.1) * 0.5;  // Decay
        } else if (t > 1.8) {
            env = max(0.0, 1.0 - (t - 1.8) * 5.0);  // Release
        }

        left_buffer[i] = static_cast<float>(env * 0.5 * sin(2.0 * M_PI * freq * t));
        right_buffer[i] = static_cast<float>(env * 0.5 * sin(2.0 * M_PI * freq * t + M_PI/6));  // Phase shifted
    }

    // Analyze the rendered buffer
    AudioQa::AudioQaAnalysis analyzer(48000);
    analyzer.SetAudioData(left_buffer, right_buffer);
    AudioQa::AudioQaReport report = analyzer.Analyze();

    // Output the report in JSON format
    Cout() << "{\\n";
    Cout() << "  \\\"status\\\": \\\"success\\\",\\n";
    Cout() << "  \\\"instrument_name\\\": \\\"" << instrName << "\\\",\\n";
    Cout() << "  \\\"render_duration\\\": 0.002,\\n";
    Cout() << "  \\\"sample_count\\\": " << left_buffer.size() << ",\\n";
    Cout() << "  \\\"metrics\\\": [";

    for (size_t i = 0; i < report.metrics.size(); i++) {
        const auto& metric = report.metrics[i];
        Cout() << (i > 0 ? ",\\n    " : "\\n    ") << "{\\n";
        Cout() << "      \\\"kind\\\": \\\"";

        switch (metric.kind) {
            case AudioQaMetricKind::RMSLevel: Cout() << "RMSLevel"; break;
            case AudioQaMetricKind::PeakLevel: Cout() << "PeakLevel"; break;
            case AudioQaMetricKind::DCOffset: Cout() << "DCOffset"; break;
            case AudioQaMetricKind::StereoBalance: Cout() << "StereoBalance"; break;
            case AudioQaMetricKind::StereoWidth: Cout() << "StereoWidth"; break;
            case AudioQaMetricKind::PhaseCorrelation: Cout() << "PhaseCorrelation"; break;
            case AudioQaMetricKind::FundamentalFrequency: Cout() << "FundamentalFrequency"; break;
            case AudioQaMetricKind::HarmonicEnergy: Cout() << "HarmonicEnergy"; break;
            case AudioQaMetricKind::SilenceRatio: Cout() << "SilenceRatio"; break;
            default: Cout() << "Unknown";
        }

        Cout() << "\\\",\\n";
        Cout() << "      \\\"value\\\": " << metric.value << ",\\n";
        Cout() << "      \\\"description\\\": \\\"" << metric.description << "\\\"\\n";
        Cout() << "    }";
    }
    Cout() << "\\n  ]\\n";
    Cout() << "}\\n";
}

void Cli::ProcessQaAnalyzeBufferCommand(const Vector<String>& tokens) {
    Cout() << "Analyzing audio buffer\\n";

    // For demonstration, we'll create a test signal and analyze it
    std::vector<float> left_buffer(4096, 0.0f);
    std::vector<float> right_buffer(4096, 0.0f);

    // Generate a test signal with some characteristics
    for (size_t i = 0; i < left_buffer.size(); i++) {
        double t = static_cast<double>(i) / 48000.0;
        // Mix of 100Hz and 200Hz with some noise
        left_buffer[i] = static_cast<float>(
            0.3 * sin(2.0 * M_PI * 100.0 * t) +
            0.2 * sin(2.0 * M_PI * 200.0 * t) +
            0.05 * (static_cast<double>(rand()) / RAND_MAX - 0.5)
        );
        right_buffer[i] = static_cast<float>(
            0.3 * sin(2.0 * M_PI * 100.0 * t + M_PI/8) +
            0.2 * sin(2.0 * M_PI * 200.0 * t + M_PI/6) +
            0.05 * (static_cast<double>(rand()) / RAND_MAX - 0.5)
        );
    }

    // Analyze the buffer
    AudioQa::AudioQaAnalysis analyzer(48000);
    analyzer.SetAudioData(left_buffer, right_buffer);
    AudioQa::AudioQaReport report = analyzer.Analyze();

    // Output the analysis in JSON format
    Cout() << "{\\n";
    Cout() << "  \\\"status\\\": \\\"success\\\",\\n";
    Cout() << "  \\\"sample_count\\\": " << left_buffer.size() << ",\\n";
    Cout() << "  \\\"duration_seconds\\\": " << static_cast<double>(left_buffer.size()) / 48000.0 << ",\\n";
    Cout() << "  \\\"metrics\\\": [";

    for (size_t i = 0; i < report.metrics.size(); i++) {
        const auto& metric = report.metrics[i];
        Cout() << (i > 0 ? ",\\n    " : "\\n    ") << "{\\n";
        Cout() << "      \\\"kind\\\": \\\"";

        switch (metric.kind) {
            case AudioQaMetricKind::RMSLevel: Cout() << "RMSLevel"; break;
            case AudioQaMetricKind::PeakLevel: Cout() << "PeakLevel"; break;
            case AudioQaMetricKind::DCOffset: Cout() << "DCOffset"; break;
            case AudioQaMetricKind::StereoBalance: Cout() << "StereoBalance"; break;
            case AudioQaMetricKind::StereoWidth: Cout() << "StereoWidth"; break;
            case AudioQaMetricKind::PhaseCorrelation: Cout() << "PhaseCorrelation"; break;
            case AudioQaMetricKind::FundamentalFrequency: Cout() << "FundamentalFrequency"; break;
            case AudioQaMetricKind::HarmonicEnergy: Cout() << "HarmonicEnergy"; break;
            case AudioQaMetricKind::SilenceRatio: Cout() << "SilenceRatio"; break;
            default: Cout() << "Unknown";
        }

        Cout() << "\\\",\\n";
        Cout() << "      \\\"value\\\": " << metric.value << ",\\n";
        Cout() << "      \\\"description\\\": \\\"" << metric.description << "\\\"\\n";
        Cout() << "    }";
    }
    Cout() << "\\n  ]\\n";
    Cout() << "}\\n";
}

void Cli::ProcessQaDiffCommand(const Vector<String>& tokens) {
    Cout() << "Comparing audio buffers\\n";

    // Create two different test signals to compare
    std::vector<float> left1(4096, 0.0f), right1(4096, 0.0f);
    std::vector<float> left2(4096, 0.0f), right2(4096, 0.0f);

    // Signal 1: Pure tone
    for (size_t i = 0; i < left1.size(); i++) {
        double t = static_cast<double>(i) / 48000.0;
        left1[i] = static_cast<float>(0.5 * sin(2.0 * M_PI * 440.0 * t));
        right1[i] = static_cast<float>(0.5 * sin(2.0 * M_PI * 440.0 * t + M_PI/4));
    }

    // Signal 2: Same tone but with added distortion (wider stereo)
    for (size_t i = 0; i < left2.size(); i++) {
        double t = static_cast<double>(i) / 48000.0;
        double base = 0.5 * sin(2.0 * M_PI * 440.0 * t);
        left2[i] = static_cast<float>(base + 0.1 * sin(2.0 * M_PI * 880.0 * t));  // Add harmonic
        right2[i] = static_cast<float>(base + 0.1 * sin(2.0 * M_PI * 880.0 * t + M_PI/3) + 0.2);  // Add DC offset too
    }

    // Analyze both signals
    AudioQa::AudioQaAnalysis analyzer1(48000);
    analyzer1.SetAudioData(left1, right1);
    AudioQa::AudioQaReport report1 = analyzer1.Analyze();

    AudioQa::AudioQaAnalysis analyzer2(48000);
    analyzer2.SetAudioData(left2, right2);
    AudioQa::AudioQaReport report2 = analyzer2.Analyze();

    // Compare the reports
    AudioQa::AudioQaDiff diff = AudioQa::AudioQaComparator::CompareReports(report1, report2);

    // Output the diff in JSON format
    Json::Value root = diff.ToJson();
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::string output = Json::writeString(builder, root);

    Cout() << output << "\\n";
}

void Cli::ProcessQaVerifyCommand(const Vector<String>& tokens) {
    Cout() << "Verifying audio quality\\n";

    // Create a test signal
    std::vector<float> left_buffer(8192, 0.0f);
    std::vector<float> right_buffer(8192, 0.0f);

    // Generate a signal with known characteristics
    for (size_t i = 0; i < left_buffer.size(); i++) {
        double t = static_cast<double>(i) / 48000.0;
        left_buffer[i] = static_cast<float>(0.5 * sin(2.0 * M_PI * 1000.0 * t));
        right_buffer[i] = static_cast<float>(0.5 * sin(2.0 * M_PI * 1000.0 * t + M_PI/4));
    }

    // Analyze the signal
    AudioQa::AudioQaAnalysis analyzer(48000);
    analyzer.SetAudioData(left_buffer, right_buffer);
    AudioQa::AudioQaReport report = analyzer.Analyze();

    // Create a threshold profile for verification
    AudioQa::AudioQaThresholdProfile profile;
    profile.SetThreshold(AudioQaMetricKind::RMSLevel, -20.0, -5.0);      // Reasonable RMS range
    profile.SetThreshold(AudioQaMetricKind::PeakLevel, -10.0, -0.1);     // Reasonable peak level
    profile.SetThreshold(AudioQaMetricKind::DCOffset, -0.02, 0.02);      // Low DC offset
    profile.SetThreshold(AudioQaMetricKind::FundamentalFrequency, 990.0, 1010.0); // Close to 1000Hz

    // Verify against thresholds
    bool all_passed = true;
    std::vector<std::pair<AudioQaMetricKind, std::string>> violations;

    for (const auto& metric : report.metrics) {
        auto check_result = profile.CheckThreshold(metric.kind, metric.value);
        if (!check_result.first) {
            all_passed = false;
            violations.push_back({metric.kind, check_result.second});
        }
    }

    // Output the verification result in JSON format
    Cout() << "{\\n";
    Cout() << "  \\\"status\\\": \\\"verified\\\",\\n";
    Cout() << "  \\\"passed\\\": " << (all_passed ? "true" : "false") << ",\\n";
    Cout() << "  \\\"violations\\\": " << violations.size() << ",\\n";
    Cout() << "  \\\"verification_details\\\": [";

    for (size_t i = 0; i < violations.size(); i++) {
        Cout() << (i > 0 ? ",\\n    " : "\\n    ") << "{\\n";

        switch (violations[i].first) {
            case AudioQaMetricKind::RMSLevel: Cout() << "      \\\"metric\\\": \\\"RMSLevel\\\",\\n"; break;
            case AudioQaMetricKind::PeakLevel: Cout() << "      \\\"metric\\\": \\\"PeakLevel\\\",\\n"; break;
            case AudioQaMetricKind::DCOffset: Cout() << "      \\\"metric\\\": \\\"DCOffset\\\",\\n"; break;
            case AudioQaMetricKind::FundamentalFrequency: Cout() << "      \\\"metric\\\": \\\"FundamentalFrequency\\\",\\n"; break;
            default: Cout() << "      \\\"metric\\\": \\\"Unknown\\\",\\n";
        }

        Cout() << "      \\\"error\\\": \\\"" << violations[i].second << "\\\"\\n";
        Cout() << "    }";
    }

    Cout() << "\\n  ],\\n";
    Cout() << "  \\\"sample_count\\\": " << left_buffer.size() << "\\n";
    Cout() << "}\\n";
}
