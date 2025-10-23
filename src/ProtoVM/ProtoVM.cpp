#include "ProtoVM.h"
#include "Cli.h"

/*
LinkBases:
	- https://github.com/vygr/C-PCB
*/




void SetupTest0_FlipFlop(Machine& mach);
void SetupTest1_ANDGate(Machine& mach);
void SetupTest2_Counter(Machine& mach);
void SetupTest3_Memory(Machine& mach);
void SetupTest4_6502(Machine& mach);
void SetupUK101(Machine& mach);
void SetupInterak(Machine& mach);
void SetupMiniMax8085(Machine& mach);



#ifdef flagMAIN

CONSOLE_APP_MAIN {
	using namespace Upp;
	SetCoutLog();
	
	// Parse command line arguments
	const Vector<String>& args = CommandLine();
	
	// Check for help flag
	if (FindIndex(args, "-h") >= 0 || FindIndex(args, "--help") >= 0) {
		Cout() << "ProtoVM Digital Logic Simulator\n";
		Cout() << "Usage: " << GetExeTitle() << " [options] [circuit_name]\n";
		Cout() << "Options:\n";
		Cout() << "  -h, --help     Show this help message\n";
		Cout() << "  -v, --version  Show version information\n";
		Cout() << "  -t, --ticks N  Run simulation for N ticks (default: 100)\n";
		Cout() << "  --cli          Start in interactive CLI mode\n";
		Cout() << "Circuits:\n";
		Cout() << "  flipflop  - Simple flip-flop test circuit\n";
		Cout() << "  andgate   - Simple AND gate test circuit\n";
		Cout() << "  counter   - 4-bit counter test circuit\n";
		Cout() << "  memory    - Memory test circuit\n";
		Cout() << "  6502      - 6502 CPU test circuit\n";
		Cout() << "  uk101     - UK101 computer circuit\n";
		Cout() << "  interak   - Interak computer circuit\n";
		Cout() << "  minimax   - MiniMax 8085 computer circuit\n";
		Cout() << "\nExamples:\n";
		Cout() << "  " << GetExeTitle() << " 6502 -t 1000    # Run 6502 circuit for 1000 ticks\n";
		Cout() << "  " << GetExeTitle() << " --cli           # Start interactive CLI mode\n";
		return;
	}
	
	// Check for version flag
	if (FindIndex(args, "-v") >= 0 || FindIndex(args, "--version") >= 0) {
		Cout() << "ProtoVM Digital Logic Simulator v1.0\n";
		return;
	}
	
	// Find circuit selection and options
	String circuit_name = "6502"; // default
	int max_ticks = 100;
	bool interactive_cli = false;
	
	for (int i = 0; i < args.GetCount(); i++) {
		String arg = args[i];
		
		if (arg == "-t" || arg == "--ticks") {
			if (i + 1 < args.GetCount()) {
				max_ticks = StrInt(args[i + 1]);
				i++; // skip next argument
			}
		}
		else if (arg == "--cli") {
			interactive_cli = true;
		}
		else if (arg == "flipflop" || arg == "andgate" || arg == "counter" || arg == "memory" || arg == "6502" || arg == "uk101" || arg == "interak" || arg == "minimax") {
			circuit_name = arg;
		}
	}
	
	// Initialize machine
	Machine mach;
	
	// Set up selected circuit
	if (circuit_name == "flipflop") {
		SetupTest0_FlipFlop(mach);
		LOG("Loaded Flip-Flop Test circuit");
	}
	else if (circuit_name == "andgate") {
		SetupTest1_ANDGate(mach);
		LOG("Loaded AND Gate Test circuit");
	}
	else if (circuit_name == "counter") {
		SetupTest2_Counter(mach);
		LOG("Loaded 4-bit Counter Test circuit");
	}
	else if (circuit_name == "memory") {
		SetupTest3_Memory(mach);
		LOG("Loaded Memory Test circuit");
	}
	else if (circuit_name == "6502") {
		SetupTest4_6502(mach);
		LOG("Loaded 6502 Test circuit");
	}
	else if (circuit_name == "uk101") {
		SetupUK101(mach);
		LOG("Loaded UK101 circuit");
	}
	else if (circuit_name == "interak") {
		SetupInterak(mach);
		LOG("Loaded Interak circuit");
	}
	else if (circuit_name == "minimax") {
		SetupMiniMax8085(mach);
		LOG("Loaded MiniMax8085 circuit");
	}
	else {
		LOG("Unknown circuit: " << circuit_name << ", defaulting to 6502");
		SetupTest4_6502(mach);
		circuit_name = "6502";
	}
	
	// If CLI mode is requested, start interactive mode, otherwise run simulation
	if (interactive_cli) {
		if (mach.Init()) {
			LOG("Starting interactive CLI mode for circuit: " << circuit_name);
			
			Cli cli;
			cli.SetMachine(&mach);
			cli.Start();
		}
		else {
			LOG("Failed to initialize machine for CLI mode");
		}
	}
	else {
		// Run the simulation with the specified number of ticks
		if (mach.Init()) {
			LOG("Running " << circuit_name << " circuit for " << max_ticks << " ticks");
			
			for(int i = 0; i < max_ticks; i++) {
				LOG("Tick " << i);
				
				if (!mach.Tick())
					break;
			}
			
			LOG("Simulation completed after " << max_ticks << " ticks");
		}
		else {
			LOG("Failed to initialize machine");
		}
	}
}

#endif
