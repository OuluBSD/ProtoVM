#include "ProtoVM.h"
#include "Cli.h"
#include "MDS1101SchematicTool.h"
#include "IC4001.h"
#include "IC4002.h"
#include "IC4003.h"
#include "AddressDecoder4004.h"
#include "ClockGenerator4004.h"
#include "PowerOnReset4004.h"
#include "Test4004CPU.h"

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
void TestMDS1101SchematicTool();
void Run4004UnitTests();

// Character output function
void OutputCharacter(char c);



// Helper functions for 4004 memory initialization and debugging
bool LoadProgramTo4004ROM(Machine& mach, const String& filename, int start_addr = 0);
void Debug4004CPUState(Machine& mach);
void Poke4004Memory(Machine& mach, int addr, byte value);
byte Peek4004Memory(Machine& mach, int addr);
void Dump4004Memory(Machine& mach, int start_addr, int count);

void TestPslParserFunction();  // Forward declaration

#include "Helper4004.h"

void SetupMiniMax4004(Machine& mach) {
    Pcb& pcb = mach.AddPcb();

    // Create and configure the 4004 CPU
    IC4004& cpu = pcb.Add<IC4004>("CPU4004");

    // Create memory components for the 4004 system
    ICRamRom& rom = pcb.Add<ICRamRom>("ROM4001");  // 4001 ROM equivalent
    rom.SetSize(256);      // Example size: 256 bytes
    rom.SetReadOnly(true); // ROM is read-only

    ICRamRom& ram = pcb.Add<ICRamRom>("RAM4002");  // 4002 RAM equivalent
    ram.SetSize(40);       // 40 bytes as in original 4002
    ram.SetReadOnly(false); // RAM is read/write

    // Create buses for the 4004 system
    Bus<4>& data_bus = pcb.Add<Bus<4>>("DATA_BUS");
    Bus<12>& addr_bus = pcb.Add<Bus<12>>("ADDR_BUS");

    // Create control pins
    Pin& clk = pcb.Add<Pin>("CLK").SetReference(1);    // Clock HIGH
    Pin& reset = pcb.Add<Pin>("RESET").SetReference(0); // Reset LOW
    Pin& ground = pcb.Add<Pin>("ground").SetReference(0); // Ground
    Pin& vcc = pcb.Add<Pin>("vcc").SetReference(1);     // VCC

    try {
        // Connect CPU data bus pins to data bus
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

        // Connect CPU control signals
        clk >> cpu["CM4"];        // Clock to CPU
        reset >> cpu["RES"];      // Reset to CPU

        // Connect CPU control signals that were unconnected before
        cpu["CM"] >> ground["0"];   // CPU clock output to ground
        cpu["BUSY"] >> ground["0"]; // Busy signal to ground
        cpu["R/W"] >> vcc["0"];     // Default to read mode
        cpu["MR"] >> vcc["0"];      // Memory Read inactive
        cpu["MW"] >> ground["0"];   // Memory Write inactive
        cpu["SBY"] >> ground["0"];  // System busy to ground

        // Connect ROM to buses
        addr_bus[0] >> rom["A0"];
        addr_bus[1] >> rom["A1"];
        addr_bus[2] >> rom["A2"];
        addr_bus[3] >> rom["A3"];
        addr_bus[4] >> rom["A4"];
        addr_bus[5] >> rom["A5"];
        addr_bus[6] >> rom["A6"];
        addr_bus[7] >> rom["A7"];
        addr_bus[8] >> rom["A8"];
        addr_bus[9] >> rom["A9"];
        addr_bus[10] >> rom["A10"];
        addr_bus[11] >> rom["A11"];

        // ROM data connections (outputs to bus)
        rom["D0"] >> data_bus[0];
        rom["D1"] >> data_bus[1];
        rom["D2"] >> data_bus[2];
        rom["D3"] >> data_bus[3];

        // ROM control signals (active low)
        vcc["0"] >> rom["~OE"];  // Output enable inactive
        vcc["0"] >> rom["~CS"];  // Chip select inactive
        vcc["0"] >> rom["~WR"];  // Write enable (not used for ROM)

        // Connect RAM to buses
        addr_bus[0] >> ram["A0"];
        addr_bus[1] >> ram["A1"];
        addr_bus[2] >> ram["A2"];
        addr_bus[3] >> ram["A3"];

        // RAM data connections (bidirectional)
        ram["D0"] >> data_bus[0];  // Output to bus
        ram["D1"] >> data_bus[1];
        ram["D2"] >> data_bus[2];
        ram["D3"] >> data_bus[3];

        // RAM control signals (active low)
        vcc["0"] >> ram["~OE"];  // Output enable inactive
        vcc["0"] >> ram["~CS"];  // Chip select inactive
        vcc["0"] >> ram["~WR"];  // Write enable inactive

        LOG("MiniMax4004 system configured with 4004 CPU, ROM, and RAM");
    }
    catch (Exc e) {
        LOG("Connection error in SetupMiniMax4004: " << e);
    }
}

// Character output function
void OutputCharacter(char c) {
    // Output the character to the console
    UPP::Cout() << c << UPP::EOL;
    LOG("Character output: '" << c << "' (0x" << HexStr(c) << ")");
}

void TestMDS1101SchematicTool() {
    LOG("Testing MDS-1101 Schematic Tool...");

    // Initialize the schematic tool
    MDS1101SchematicTool tool;

    // Find an appropriate PCB image from the MDS-1101 directory
    // For now, use the first available image as a demonstration
    std::string pcb_image_path = "circuitboards/MDS-1101/machine1.jpg";

    // Load PCB image
    if (!tool.LoadPCBImage(pcb_image_path)) {
        LOG("Failed to load PCB image: " << pcb_image_path);
        return;
    }

    // Analyze the image to detect components and connections
    if (!tool.AnalyzeImage()) {
        LOG("Failed to analyze PCB image");
        return;
    }

    // Generate the schematic from the analysis
    Schematic schematic = tool.GenerateSchematic();

    // Render the schematic for display
    tool.RenderSchematic();

    // Export the schematic to ProtoVM format
    if (!tool.ExportToProtoVM("MDS1101_schematic_output.psl")) {
        LOG("Failed to export schematic to ProtoVM format");
        return;
    }

    LOG("MDS-1101 Schematic Tool test completed successfully");
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
		Cout() << "  --load-binary <file> [addr]  Load binary program file into memory at specified address\n";
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
		Cout() << "  mds1101      - MDS-1101 schematic tool demonstration\n";
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
	String binary_file = "";  // Binary file to load
	int load_address = 0; // Address to load the binary file

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
		else if (arg == "--load-binary" || arg == "-lb") {
			if (i + 1 < args.GetCount()) {
				binary_file = args[i + 1];
				i++; // skip next argument
				
				// Optionally parse address too
				if (i + 1 < args.GetCount() && !args[i + 1].StartsWith("-")) {
					String addr_str = args[i + 1];
					if (!addr_str.StartsWith("0x") && !addr_str.StartsWith("0X")) {
						addr_str = "0x" + addr_str; // Add 0x prefix for hex parsing
					}
					load_address = StrInt(addr_str);
					i++; // skip next argument
				}
			}
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
		else if (arg == "flipflop" || arg == "andgate" || arg == "counter" || arg == "memory" || arg == "6502" || arg == "basiclogic" || arg == "test4bit" || arg == "test4bitmemory" || arg == "muxdemux" || arg == "decenc" || arg == "testgates" || arg == "uk101" || arg == "interak" || arg == "minimax" || arg == "minimax4004" || arg == "unittests" || arg == "statemachine" || arg == "basiccpu" || arg == "clkdivider" || arg == "clkgate" || arg == "pll" || arg == "signaltrace" || arg == "mds1101") {
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
	else if (circuit_name == "mds1101") {
		TestMDS1101SchematicTool();
		LOG("Loaded MDS-1101 Schematic Tool test");
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
			Cout() << "  mds1101      - MDS-1101 schematic tool demonstration\n";
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

	// If there's a binary file to load, load it before starting
	if (!binary_file.IsEmpty()) {
		// Attempt to load the binary file into appropriate memory components
		LOG("Attempting to load binary file: " << binary_file << " at address 0x" << HexStr(load_address));
		
		// Try to find an IC4001 (ROM) component to load the program into
		for (int pcb_id = 0; pcb_id < mach.pcbs.GetCount(); pcb_id++) {
			Pcb* pcb = &mach.pcbs[pcb_id];
			if (!pcb) continue;
			
			for (int i = 0; i < pcb->GetNodeCount(); i++) {
				ElectricNodeBase* comp = &pcb->GetNode(i);
				if (String(comp->GetClassName()) == "IC4001") {
					try {
						// Cast to IC4001 to access memory loading capabilities
						IC4001* rom = dynamic_cast<IC4001*>(comp);
						if (rom) {
							// Load the binary file
							FileIn file(binary_file);
							if (!file.IsOpen()) {
								LOG("Error: Could not open file '" << binary_file << "'");
							} else {
								LOG("Loading binary file into 4001 ROM component: " << comp->GetName());
								
								// Read and load the binary data
								int addr = load_address;
								while (!file.IsEof() && addr <= 0xFFF) {
									int byte_val = file.Get();
									if (byte_val == -1) break; // End of file
									
									// For the 4004, we store 4-bit values, so split the byte if needed
									rom->SetMemory(addr, byte_val & 0x0F); // Store lower 4 bits
									addr++;
									
									// Also store upper 4 bits if there's space
									if (addr <= 0xFFF) {
										rom->SetMemory(addr, (byte_val >> 4) & 0x0F); // Store upper 4 bits
										addr++;
									}
								}
								
								LOG("Successfully loaded binary file into ROM starting at address 0x" << HexStr(load_address));
								break; // Stop after loading into first found ROM
							}
						}
					} catch (...) {
						// Dynamic cast failed or other error
						continue;
					}
				}
			}
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