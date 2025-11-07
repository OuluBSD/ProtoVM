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
void SetupTest3_BasicLogicGates(Machine& mach);
void SetupTest4_MuxDemux(Machine& mach);
void SetupTest5_DecoderEncoder(Machine& mach);
void TestBasicLogicGates(Machine& mach);
void Test4BitRegister(Machine& mach);
void Test4BitMemory(Machine& mach);
void Test4BitAdder(Machine& mach);
void SetupUK101(Machine& mach);
void SetupInterak(Machine& mach);
void SetupMiniMax8085(Machine& mach);
void RunArithmeticUnitTests(Machine& mach);
void Test60_StateMachine();
void Test70_Basic8BitCPU();
void Test80_ClockDivider();
void Test81_ClockGate();
void Test82_PLL();



void TestPslParserFunction();  // Forward declaration

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
		Cout() << "  flipflop    - Simple flip-flop test circuit\n";
		Cout() << "  andgate     - Simple AND gate test circuit\n";
		Cout() << "  counter     - 4-bit counter test circuit\n";
		Cout() << "  memory      - Memory test circuit\n";
		Cout() << "  6502        - 6502 CPU test circuit\n";
		Cout() << "  basiclogic  - Basic logic gates test circuit\n";
		Cout() << "  test4bit    - 4-bit register test circuit\n";
		Cout() << "  test4bitmemory - 4-bit memory test circuit\n";
		Cout() << "  muxdemux    - Multiplexer/demultiplexer test circuit\n";
		Cout() << "  decenc      - Decoder/encoder test circuit\n";
		Cout() << "  testgates   - Comprehensive logic gates test\n";
		Cout() << "  uk101       - UK101 computer circuit\n";
		Cout() << "  interak     - Interak computer circuit\n";
		Cout() << "  unittests   - Run unit tests for arithmetic components\n";
		Cout() << "  minimax     - MiniMax 8085 computer circuit\n";
		Cout() << "  statemachine - State machine test circuit\n";
		Cout() << "  basiccpu     - Basic 8-bit CPU test circuit\n";
		Cout() << "  clkdivider   - Clock divider test circuit\n";
		Cout() << "  clkgate      - Clock gating test circuit\n";
		Cout() << "  pll          - Phase-locked loop test circuit\n";
		Cout() << "\nExamples:\n";
		Cout() << "  " << GetExeTitle() << " 6502 -t 1000    # Run 6502 circuit for 1000 ticks\n";
		Cout() << "  " << GetExeTitle() << " --cli           # Start interactive CLI mode\n";
		Cout() << "  " << GetExeTitle() << " testgates       # Run comprehensive logic gate test\n";
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
	bool run_psl_test = false;
	
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
		else if (arg == "--psl-test") {
			run_psl_test = true;
		}
		else if (arg == "flipflop" || arg == "andgate" || arg == "counter" || arg == "memory" || arg == "6502" || arg == "basiclogic" || arg == "test4bit" || arg == "test4bitmemory" || arg == "muxdemux" || arg == "decenc" || arg == "testgates" || arg == "uk101" || arg == "interak" || arg == "minimax" || arg == "unittests" || arg == "statemachine" || arg == "basiccpu" || arg == "clkdivider" || arg == "clkgate" || arg == "pll") {
			circuit_name = arg;
		}
	}

	// Check if running PSL parser test
	if (run_psl_test) {
		LOG("Running PSL Parser Test...");
		TestPslParserFunction();
		return;
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
	else if (circuit_name == "basiclogic") {
		SetupTest3_BasicLogicGates(mach);
		LOG("Loaded Basic Logic Gates Test circuit");
	}
	else if (circuit_name == "test4bit") {
		Test4BitRegister(mach);
		LOG("Loaded 4-bit Register Test circuit");
	}
	else if (circuit_name == "test4bitmemory") {
		Test4BitMemory(mach);
		LOG("Loaded 4-bit Memory Test circuit");
	}
	else if (circuit_name == "testgates") {
		TestBasicLogicGates(mach);
		LOG("Loaded Comprehensive Logic Gates Test circuit");
	}
	else if (circuit_name == "muxdemux") {
		SetupTest4_MuxDemux(mach);
		LOG("Loaded Mux/Demux Test circuit");
	}
	else if (circuit_name == "decenc") {
		SetupTest5_DecoderEncoder(mach);
		LOG("Loaded Decoder/Encoder Test circuit");
	}
	else if (circuit_name == "uk101") {
		SetupUK101(mach);
		LOG("Loaded UK101 circuit");
	}
	else if (circuit_name == "interak") {
		SetupInterak(mach);
		LOG("Loaded Interak circuit");
	}
	else if (circuit_name == "unittests") {
		RunArithmeticUnitTests(mach);
		LOG("Loaded Arithmetic Unit Tests");
	}
	else if (circuit_name == "minimax") {
		SetupMiniMax8085(mach);
		LOG("Loaded MiniMax8085 circuit");
	}
	else if (circuit_name == "statemachine") {
		Test60_StateMachine();
		LOG("Loaded State Machine Test circuit");
	}
	else if (circuit_name == "basiccpu") {
		Test70_Basic8BitCPU();
		LOG("Loaded Basic 8-bit CPU Test circuit");
	}
	else if (circuit_name == "clkdivider") {
		Test80_ClockDivider();
		LOG("Loaded Clock Divider Test circuit");
	}
	else if (circuit_name == "clkgate") {
		Test81_ClockGate();
		LOG("Loaded Clock Gate Test circuit");
	}
	else if (circuit_name == "pll") {
		Test82_PLL();
		LOG("Loaded Phase-Locked Loop Test circuit");
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
