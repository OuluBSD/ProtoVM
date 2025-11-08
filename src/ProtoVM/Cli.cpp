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

	// Start the interactive command loop
	Cout() << "Enter commands (type 'quit' to exit):\n";
	
	while (running) {
		Cout() << "> ";
		fflush(stdout);  // Ensure prompt is displayed

		String command;
		int ch;
		while ((ch = getchar()) != '\n' && ch != EOF) {
			if (ch == '\r') continue; // Skip carriage returns
			command.Cat() << (char)ch;
		}

		if (ch == EOF) {
			// Handle EOF (like Ctrl+D)
			Cout() << "\n";
			ProcessCommand("quit");
			break;
		}

		ProcessCommand(command);
	}
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
	else if (cmd == "load" || cmd == "l") {
		ProcessLoadCommand(tokens);
	}
	else if (cmd == "step" || cmd == "s") {
		ProcessStepCommand(tokens);
	}
	else if (cmd == "continue" || cmd == "cont" || cmd == "c") {
		ProcessContinueCommand(tokens);
	}
	else if (cmd == "break" || cmd == "b") {
		ProcessBreakCommand(tokens);
	}
	else if (cmd == "dump" || cmd == "d" || cmd == "memory") {
		ProcessMemoryDumpCommand(tokens);
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
	Cout() << "  inspect, i <comp> [pcb_id] - Show detailed state of a specific component\n";
	Cout() << "  state, s [pcb_id] - Show current state of all components on a PCB\n";
	Cout() << "  visualize, v [pcb_id] - Show connections between components on a PCB\n";
	Cout() << "  netlist, n [pcb_id] - Generate netlist for a PCB\n";
	Cout() << "  trace, t <comp> <pin> [pcb_id] - Add a signal to trace\n";
	Cout() << "  tracelog, tl     - Show the signal transition log\n";
	Cout() << "  load, l <file> [addr] [pcb_id] - Load binary program file into memory\n";
	Cout() << "  step, s [n]      - Step through execution, n ticks at a time (default: 1)\n";
	Cout() << "  continue, cont, c [n] - Run simulation for n ticks (default: 100)\n";
	Cout() << "  break, b [subcmd] - Manage breakpoints (list, set, clear)\n";
	Cout() << "  dump, d, memory [start] [end] - Display memory contents in hex dump format\n";
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

bool Cli::running_in_step_mode = false;

void Cli::ProcessStepCommand(const Vector<String>& tokens) {
    if (!machine) {
        Cout() << "Error: No machine available.\n";
        return;
    }

    int num_steps = 1; // Default to 1 step
    if (tokens.GetCount() > 1) {
        num_steps = StrInt(tokens[1]); // Parse the number of steps
    }

    if (num_steps < 1) {
        Cout() << "Error: Number of steps must be positive.\n";
        return;
    }

    Cout() << "Stepping " << num_steps << " tick(s)...\n";

    // Execute the specified number of steps
    for (int i = 0; i < num_steps; i++) {
        if (!machine->Tick()) {
            Cout() << "Simulation halted at tick " << i << "\n";
            break;
        }
        
        // Log the tick if needed for debugging
        Cout() << "Completed tick " << i + 1 << "\n";
    }

    Cout() << "Step completed.\n";
}

void Cli::ProcessContinueCommand(const Vector<String>& tokens) {
    if (!machine) {
        Cout() << "Error: No machine available.\n";
        return;
    }

    int num_ticks = 100; // Default to 100 ticks
    if (tokens.GetCount() > 1) {
        num_ticks = StrInt(tokens[1]); // Parse the number of ticks
    }

    if (num_ticks < 1) {
        Cout() << "Error: Number of ticks must be positive.\n";
        return;
    }

    Cout() << "Running for " << num_ticks << " tick(s)...\n";

    // Execute the specified number of ticks
    for (int i = 0; i < num_ticks; i++) {
        if (!machine->Tick()) {
            Cout() << "Simulation halted at tick " << i << "\n";
            break;
        }
    }

    Cout() << "Execution completed.\n";
}

void Cli::ProcessBreakCommand(const Vector<String>& tokens) {
    // This would implement breakpoint functionality
    // For now, we'll just show available breakpoints
    Cout() << "Breakpoint functionality:\n";
    Cout() << "  break list - List all breakpoints\n";
    Cout() << "  break set <component> <condition> - Set a breakpoint\n";
    Cout() << "  break clear [index] - Clear breakpoints\n";
    Cout() << "  break clear all - Clear all breakpoints\n";
    
    if (tokens.GetCount() < 2) {
        Cout() << "Available subcommands: list, set, clear\n";
        return;
    }
    
    String subcmd = ToLower(tokens[1]);
    if (subcmd == "set") {
        // For now, just acknowledge the command
        Cout() << "Breakpoint set functionality would be implemented here.\n";
    } else if (subcmd == "list") {
        Cout() << "No breakpoints currently set.\n";
    } else if (subcmd == "clear") {
        Cout() << "Breakpoints cleared.\n";
    } else {
        Cout() << "Unknown breakpoint subcommand: " << subcmd << "\n";
    }
}

void Cli::ProcessMemoryDumpCommand(const Vector<String>& tokens) {
    if (!machine) {
        Cout() << "Error: No machine available.\n";
        return;
    }

    // Default values for memory dump
    int start_address = 0;
    int end_address = 0xFF; // Default to first 256 bytes
    
    if (tokens.GetCount() >= 2) {
        // Parse start address
        String addr_str = tokens[1];
        if (addr_str.StartsWith("0x") || addr_str.StartsWith("0X")) {
            addr_str = addr_str.Mid(2); // Remove 0x prefix
        }
        start_address = StrIntHex(addr_str);
        
        if (tokens.GetCount() >= 3) {
            // Parse end address
            String end_str = tokens[2];
            if (end_str.StartsWith("0x") || end_str.StartsWith("0X")) {
                end_str = end_str.Mid(2); // Remove 0x prefix
            }
            end_address = StrIntHex(end_str);
            
            // Ensure end is greater than start
            if (end_address < start_address) {
                end_address = start_address + 0xFF; // Default to showing next 256 bytes
            }
        } else {
            // If only start address specified, show next 16 bytes
            end_address = start_address + 0xF;
        }
    }

    // Validate address range is within 4004's 12-bit address space (0x000-0xFFF)
    if (start_address > 0xFFF || end_address > 0xFFF) {
        Cout() << "Error: Address must be between 0x000 and 0xFFF (12-bit range)\n";
        return;
    }
    
    if (end_address < start_address) {
        int temp = end_address;
        end_address = start_address;
        start_address = temp;
    }
    
    // Limit to a reasonable range to avoid overwhelming output
    if (end_address - start_address > 0x100) {  // More than 256 bytes
        end_address = start_address + 0xFF;  // Limit to 256 bytes
        Cout() << "Limiting display to 256 bytes for readability\n";
    }

    Cout() << "Memory dump from 0x" << HexStr(start_address) 
           << " to 0x" << HexStr(end_address) << ":\n";

    // Try to find memory components (ROM/RAM) to read from
    // We'll look for ICRamRom components on all PCBs
    bool found_memory = false;
    
    for (int pcb_id = 0; pcb_id < machine->GetPcbCount(); pcb_id++) {
        Pcb* pcb = machine->GetPcb(pcb_id);
        if (!pcb) continue;

        for (int i = 0; i < pcb->GetComponentCount(); i++) {
            ElectricNodeBase* comp = pcb->GetComponent(i);
            String comp_class = comp->GetClassName();
            
            // Check for memory components (ROM and RAM)
            if (comp_class == "IC4001" || comp_class == "IC4002" || comp_class == "ICRamRom") {
                Cout() << "Found " << comp_class << " component: " << comp->GetName() 
                       << " (PCB " << pcb_id << ")\n";
                
                // For demonstration purposes, print address range
                int addr = start_address;
                while (addr <= end_address) {
                    // Calculate row start address (align to 16-byte boundary)
                    int row_start = addr & ~0xF;
                    
                    // Print address offset
                    Cout() << "0x" << HexStr(row_start, 3) << ": ";
                    
                    // Print hex values for this row (16 bytes)
                    for (int col = 0; col < 16 && (row_start + col) <= end_address; col++) {
                        int current_addr = row_start + col;
                        if (current_addr >= addr) {
                            // In a real implementation, we would read the actual memory value
                            // For now, we'll show a placeholder value
                            byte mem_val = 0x00; // Placeholder - would get real value from memory
                            
                            // Check what type of memory component we have
                            if (comp_class == "IC4001") { // ROM
                                IC4001* rom = dynamic_cast<IC4001*>(comp);
                                if (rom) {
                                    try {
                                        mem_val = rom->GetMemory(current_addr) & 0x0F; // Get 4-bit value
                                    } catch (...) {
                                        mem_val = 0x00; // Default value if not readable
                                    }
                                }
                            } else if (comp_class == "IC4002") { // RAM
                                // 4002 has a different memory access method
                                // This is simplified - actual implementation would vary
                            } else if (comp_class == "ICRamRom") { // Generic memory
                                ICRamRom* memory = dynamic_cast<ICRamRom*>(comp);
                                if (memory) {
                                    try {
                                        if (current_addr < memory->GetSize()) {
                                            mem_val = memory->GetMemory(current_addr) & 0xFF; // Get 8-bit value
                                        }
                                    } catch (...) {
                                        mem_val = 0x00; // Default value if not readable
                                    }
                                }
                            }
                            
                            Cout() << HexStr(mem_val, 2) << " "; // Print in hex with 2 digits
                        } else {
                            Cout() << ".. "; // Not in our range
                        }
                    }
                    
                    // Print ASCII representation for this row
                    Cout() << "|";
                    for (int col = 0; col < 16 && (row_start + col) <= end_address; col++) {
                        int current_addr = row_start + col;
                        if (current_addr >= addr) {
                            // Get the value again for ASCII representation
                            byte mem_val = 0x00;
                            if (comp_class == "IC4001") {
                                IC4001* rom = dynamic_cast<IC4001*>(comp);
                                if (rom) {
                                    try {
                                        mem_val = rom->GetMemory(current_addr) & 0x0F;
                                    } catch (...) {
                                        mem_val = 0x00;
                                    }
                                }
                            } else if (comp_class == "IC4001") { // ROM
                                IC4001* rom = dynamic_cast<IC4001*>(comp);
                                if (rom) {
                                    try {
                                        mem_val = rom->GetMemory(current_addr) & 0x0F;
                                    } catch (...) {
                                        mem_val = 0x00;
                                    }
                                }
                            } else if (comp_class == "ICRamRom") { // Generic memory
                                ICRamRom* memory = dynamic_cast<ICRamRom*>(comp);
                                if (memory) {
                                    try {
                                        if (current_addr < memory->GetSize()) {
                                            mem_val = memory->GetMemory(current_addr) & 0xFF;
                                        }
                                    } catch (...) {
                                        mem_val = 0x00;
                                    }
                                }
                            }
                            
                            char ascii_char = (mem_val >= 0x20 && mem_val < 0x7F) ? (char)mem_val : '.';
                            Cout() << ascii_char;
                        } else {
                            Cout() << "."; // Not in our range
                        }
                    }
                    Cout() << "|\n";
                    
                    addr = row_start + 16; // Move to next row
                    found_memory = true;
                }
            }
        }
    }
    
    if (!found_memory) {
        Cout() << "No memory components found to dump.\n";
        
        // If no specific memory components found, at least show the range requested
        Cout() << "Requested range: 0x" << HexStr(start_address) 
               << " to 0x" << HexStr(end_address) << "\n";
    }
}

void Cli::ProcessLoadCommand(const Vector<String>& tokens) {
    if (!machine) {
        Cout() << "Error: No machine available.\n";
        return;
    }

    if (tokens.GetCount() < 2) {
        Cout() << "Usage: load <filename> [address] [pcb_id]\n";
        Cout() << "  filename: Path to the binary file to load\n";
        Cout() << "  address:  Starting address (default: 0x000)\n";
        Cout() << "  pcb_id:   PCB ID (default: 0)\n";
        return;
    }

    String filename = tokens[1];
    int start_address = 0; // Default start address
    int pcb_id = 0; // Default PCB ID

    // Parse optional address parameter
    if (tokens.GetCount() >= 3) {
        String addr_str = tokens[2];
        if (addr_str.StartsWith("0x") || addr_str.StartsWith("0X")) {
            addr_str = addr_str.Mid(2); // Remove 0x prefix
        }
        start_address = StrIntHex(addr_str);
        
        // Validate address is within 4004's 12-bit range
        if (start_address < 0 || start_address > 0xFFF) {
            Cout() << "Error: Address must be between 0x000 and 0xFFF (12-bit range)\n";
            return;
        }
    }

    // Parse optional PCB ID parameter
    if (tokens.GetCount() >= 4) {
        pcb_id = StrInt(tokens[3]);
    }

    // Try to find an IC4001 ROM component to load the program into
    Pcb* pcb = machine->GetPcb(pcb_id);
    if (!pcb) {
        Cout() << "Error: PCB " << pcb_id << " not found\n";
        return;
    }

    // Look for any IC4001 ROM components on this PCB
    bool found_rom = false;
    for (int i = 0; i < pcb->GetComponentCount(); i++) {
        ElectricNodeBase* comp = pcb->GetComponent(i);
        if (String(comp->GetClassName()) == "IC4001") {
            try {
                // Cast to IC4001 to access memory loading capabilities
                IC4001* rom = dynamic_cast<IC4001*>(comp);
                if (rom) {
                    // Load the binary file
                    FileIn file(filename);
                    if (!file.IsOpen()) {
                        Cout() << "Error: Could not open file '" << filename << "'\n";
                        return;
                    }

                    // Read the binary data
                    int pos = start_address;
                    while (!file.IsEof() && pos <= 0xFFF) {
                        int byte_val = file.Get();
                        if (byte_val == -1) break; // End of file
                        
                        // For 4004, we store 4-bit values, so split the byte if needed
                        // For now, we'll store it directly - the ROM component handles 4-bit values internally
                        rom->SetMemory(pos, byte_val & 0x0F); // Store lower 4 bits
                        pos++;
                        
                        // Also store upper 4 bits if there's space
                        if (pos <= 0xFFF) {
                            rom->SetMemory(pos, (byte_val >> 4) & 0x0F); // Store upper 4 bits
                            pos++;
                        }
                    }

                    Cout() << "Successfully loaded " << pos - start_address 
                           << " bytes from '" << filename << "' to address 0x" 
                           << HexStr(start_address) << " on PCB " << pcb_id << "\n";
                    found_rom = true;
                    break;
                }
            } catch (...) {
                // Dynamic cast failed or other error
                continue;
            }
        }
    }

    if (!found_rom) {
        Cout() << "Warning: No IC4001 ROM component found on PCB " << pcb_id 
               << ". If loading for a different memory type, use the appropriate command.\n";
        
        // Also try to find other memory components like ICRamRom
        for (int i = 0; i < pcb->GetComponentCount(); i++) {
            ElectricNodeBase* comp = pcb->GetComponent(i);
            if (String(comp->GetClassName()) == "ICRamRom") {
                Cout() << "Found ICRamRom component: " << comp->GetName() << "\n";
            }
        }
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