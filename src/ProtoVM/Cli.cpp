#include "ProtoVM.h"
#include "Cli.h"

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
	LOG("Would write 0x" << HexStr(value) << " to " << component << " at address 0x" << HexStr(address));
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
	LOG("Would read from " << component << " at address 0x" << HexStr(address));
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
                LOG("Added signal trace: " << componentName << "." << pinName << " on PCB " << pcbId);
            } else {
                LOG("Error: Pin  << pinName <<  not found on component  << componentName << ");
            }
        } else {
            LOG("Error: Component  << componentName <<  not found on PCB " << pcbId);
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
                LOG("Tick " << trans.tick_number << ": " << trans.component_name 
                     << "." << trans.pin_name << " [" << (int)trans.old_value 
                     << " -> " << (int)trans.new_value << "]");
            }
            
            if (start > 0) {
                LOG("  ... (showing last 50 of " << transitions.GetCount() << " total)");
            }
        }
        LOG("Total transitions logged: " << machine->GetSignalTransitionCount());
    } else {
        LOG("Error: No machine available.");
    }
}
