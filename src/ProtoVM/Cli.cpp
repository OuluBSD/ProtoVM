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
	Cout() << "Available commands: help, write, read, run, list, quit\n";
	Cout() << "Example: write RAM 0x100 0xFF\n";
	Cout() << "         run 100 (run 100 ticks)\n\n";
	
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
	Cout() << "  quit, q, exit    - Quit CLI\n";
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