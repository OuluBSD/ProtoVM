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
void SetupMiniMax4004(Machine& mach);
void RunArithmeticUnitTests(Machine& mach);
void Test60_StateMachine();
void Test70_Basic8BitCPU();
void Test80_ClockDivider();
void Test81_ClockGate();
void Test82_PLL();
void Test90_SignalTracing();



void TestPslParserFunction();  // Forward declaration

void SetupMiniMax4004(Machine& mach) {
    Pcb& pcb = mach.AddPcb();
    
    // Create and configure the 4004 CPU
    IC4004& cpu = pcb.Add<IC4004>("CPU4004");
    
    // Create memory components for the 4004 system
    ICRamRom& rom = pcb.Add<ICRamRom>("ROM4001");  // 4001 ROM
    rom.SetSize(256);      // Example size: 256 bytes
    rom.SetReadOnly(true); // ROM is read-only
    
    ICRamRom& ram = pcb.Add<ICRamRom>("RAM4002");  // 4002 RAM
    ram.SetSize(40);       // 40 bytes as in original 4002
    ram.SetReadOnly(false); // RAM is read/write
    
    // Create buses for the 4004 system
    Bus<4>& data_bus = pcb.Add<Bus<4>>("DATA_BUS");
    Bus<12>& addr_bus = pcb.Add<Bus<12>>("ADDR_BUS");
    
    // Create a clock source for the system
    Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);  // Set to logic HIGH initially
    Pin& reset = pcb.Add<Pin>("RESET").SetReference(0);  // Set to logic LOW (not in reset)
    
    try {
        // Connect CPU data bus pins to data bus (CPU D0-D3 to Bus 0-3)
        cpu["D0"] >> data_bus[0];
        cpu["D1"] >> data_bus[1];
        cpu["D2"] >> data_bus[2];
        cpu["D3"] >> data_bus[3];
        
        // Connect CPU address bus pins to address bus
        cpu["A0"] >> addr_bus[0];
        cpu["A1"] >> addr_bus[1];
        cpu["A2"] >> addr_bus[2];
        cpu["A3"] >> addr_bus[3];
        cpu["A4"] >> addr_bus[4];
        cpu["A5"] >> addr_bus[5];
        cpu["A6"] >> addr_bus[6];
        cpu["A7"] >> addr_bus[7];
        cpu["A8"] >> addr_bus[8];
        cpu["A9"] >> addr_bus[9];
        cpu["A10"] >> addr_bus[10];
        cpu["A11"] >> addr_bus[11];
        
        // Connect clock and reset signals
        clk >> cpu["CM4"];  // System clock to CPU clock input
        reset >> cpu["RES"]; // Reset signal to CPU
    
        LOG("MiniMax4004 system configured with 4004 CPU, ROM, and RAM");
    }
    catch (Exc e) {
        LOG("Connection error in SetupMiniMax4004: " << e);
    }
}

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
		Cout() << "  -V, --version  Show version information\n";
		Cout() << "  -v              Show verbose output during simulation\n";
		Cout() << "  -vv             Show more verbose output (very verbose)\n";
		Cout() << "  --verbosity=N   Set verbosity level directly (0=minimal, 1=default, 2=verbose, 3=very verbose)\n";
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
		Cout() << "  signaltrace  - Signal tracing functionality test circuit\n";
		Cout() << "\nExamples:\n";
		Cout() << "  " << GetExeTitle() << " 6502 -t 1000    # Run 6502 circuit for 1000 ticks\n";
		Cout() << "  " << GetExeTitle() << " --cli           # Start interactive CLI mode\n";
		Cout() << "  " << GetExeTitle() << " testgates       # Run comprehensive logic gate test\n";
		Cout() << "  " << GetExeTitle() << " signaltrace     # Run signal tracing test\n";
		return;
	}
	
	// Check for version flag (now using -V or --version)
	if (FindIndex(args, "-V") >= 0 || FindIndex(args, "--version") >= 0) {
		Cout() << "ProtoVM Digital Logic Simulator v1.0\n";
		return;
	}
	
	// Find circuit selection and options
	String circuit_name = ""; // no default - will show help if no circuit provided
	int max_ticks = 100;
	bool interactive_cli = false;
	bool run_psl_test = false;
	int verbosity_level = 0;  // 0=minimal output, 1=default output, 2=verbose output, 3=very verbose
	
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
		else if (arg == "-v") {
			verbosity_level = 1;  // Default verbose
		}
		else if (arg == "-vv") {
			verbosity_level = 2;  // More verbose
		}
		else if (arg == "-vvv") {
			verbosity_level = 3;  // Very verbose
		}
		else if (arg.StartsWith("--verbosity=")) {
			String level_str = arg.Mid(12); // Length of "--verbosity="
			int parsed_level = StrInt(level_str);
			verbosity_level = parsed_level > 0 ? parsed_level : 1; // Default to 1 if conversion fails
		}
		else if (arg == "flipflop" || arg == "andgate" || arg == "counter" || arg == "memory" || arg == "6502" || arg == "basiclogic" || arg == "test4bit" || arg == "test4bitmemory" || arg == "muxdemux" || arg == "decenc" || arg == "testgates" || arg == "uk101" || arg == "interak" || arg == "minimax" || arg == "minimax4004" || arg == "unittests" || arg == "statemachine" || arg == "basiccpu" || arg == "clkdivider" || arg == "clkgate" || arg == "pll" || arg == "signaltrace") {
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
	else if (circuit_name == "minimax4004") {
		SetupMiniMax4004(mach);
		LOG("Loaded MiniMax4004 circuit");
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
	else if (circuit_name == "signaltrace") {
		Test90_SignalTracing();
		LOG("Loaded Signal Tracing Test circuit");
	}
	else {
		if (circuit_name.IsEmpty()) {
			// Show help when no circuit is specified
			Cout() << "ProtoVM Digital Logic Simulator\n";
			Cout() << "Usage: " << GetExeTitle() << " [options] [circuit_name]\n";
			Cout() << "Options:\n";
			Cout() << "  -h, --help     Show this help message\n";
			Cout() << "  -V, --version  Show version information\n";
			Cout() << "  -v              Show verbose output during simulation\n";
			Cout() << "  -vv             Show more verbose output (very verbose)\n";
			Cout() << "  --verbosity=N   Set verbosity level directly (0=minimal, 1=default, 2=verbose, 3=very verbose)\n";
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
			Cout() << "  minimax4004 - MiniMax 4004 computer circuit\n";
			Cout() << "  statemachine - State machine test circuit\n";
			Cout() << "  basiccpu     - Basic 8-bit CPU test circuit\n";
			Cout() << "  clkdivider   - Clock divider test circuit\n";
			Cout() << "  clkgate      - Clock gating test circuit\n";
			Cout() << "  pll          - Phase-locked loop test circuit\n";
			Cout() << "  signaltrace  - Signal tracing functionality test circuit\n";
			Cout() << "\nExamples:\n";
			Cout() << "  " << GetExeTitle() << " 6502 -t 1000    # Run 6502 circuit for 1000 ticks\n";
			Cout() << "  " << GetExeTitle() << " --cli           # Start interactive CLI mode\n";
			Cout() << "  " << GetExeTitle() << " testgates       # Run comprehensive logic gate test\n";
			Cout() << "  " << GetExeTitle() << " signaltrace     # Run signal tracing test\n";
			return;
		} else {
			LOG("Unknown circuit: " << circuit_name << ", defaulting to 6502");
			SetupTest4_6502(mach);
			circuit_name = "6502";
		}
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
