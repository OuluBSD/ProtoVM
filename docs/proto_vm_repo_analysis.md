# ProtoVM Repository Analysis

## 1. Overview

ProtoVM is a digital logic simulator designed to model digital circuits and processors at the component level. The system provides a flexible framework for simulating various digital components, from basic logic gates to complex CPUs like the 6502 and Intel 4004, with support for analog components as well. The core architecture uses a component-based approach with signal propagation and state management capabilities.

The primary technologies involved are C++ with the Ultimate++ (U++) framework for the core simulation engine, and wxWidgets for the GUI components. The system has both command-line and GUI interfaces.

## 2. Top-level project structure

```
ProtoVM/
├── analog_synth/           # Analog synthesis components
├── bin/                    # Compiled binaries
├── build_tests/           # Build test files
├── circuitboards/         # Circuit board images and related files
├── docs/                  # Documentation files
├── external/              # External dependencies
├── schematics/            # Circuit schematics
├── scripts/               # Build and execution scripts
├── src/                   # Source code (core engine and components)
│   ├── AnalogTests/       # Analog component tests
│   ├── CADC/              # CADC (Central Air Data Computer) components
│   └── ProtoVM/           # Main simulation engine and components
├── task/                  # Task-related files
├── tests/                 # Test files
├── utils/                 # Utility scripts and tools
├── wxsrc/                 # wxWidgets GUI source code
├── build.sh              # Build script using U++ framework
├── CMakeLists.txt        # CMake build configuration for GUI
└── run.sh                # Run script
```

## 3. Core library / engine

### Key Modules:
- **Machine** (`src/ProtoVM/Machine.h/cpp`): The central simulation manager that orchestrates component evaluation and maintains state across ticks
- **ElectricNodeBase** (`src/ProtoVM/Common.h`): The base class for all electronic components
- **Pcb** (`src/ProtoVM/Pcb.h/cpp`): Represents a circuit board containing interconnected components
- **Link** (`src/ProtoVM/Link.h/cpp`): Manages connections between components
- **Component** (`src/ProtoVM/Component.h/cpp`): Implements various electronic components (logic gates, flip-flops, etc.)

### Key Classes and Responsibilities:
- **Machine**: Main simulation orchestrator that manages multiple PCBs, runs simulation ticks, handles timing analysis, and manages breakpoints
- **ElectricNodeBase**: Base class for all components, providing connection management, state tracking, and timing constraint handling
- **IC6502**: Implements 6502 CPU simulation
- **IC4004**: Intel 4004 microprocessor simulation
- **ICRamRom**: Memory component with RAM/ROM functionality
- **ICs** (`src/ProtoVM/ICs.cpp`): Collection of integrated circuit implementations
- **Bus** (`src/ProtoVM/Bus.cpp`): Bus components for signal routing

### Important public functions:
- `Machine::Tick()`: Execute one simulation step with convergence algorithm
- `Machine::Init()`: Initialize the simulation with all connections verified
- `ElectricNodeBase::Process()`: Handle data flow between components
- `ElectricNodeBase::PutRaw()`: Send data to a component pin
- `Machine::RunRtOpsWithChangeDetection()`: Execute simulation operations with change detection

## 4. Graphical user interface (GUI)

The GUI is built using wxWidgets and provides a circuit design environment:

### Main GUI Components:
- **ProtoVMApp** (`wxsrc/ProtoVMApp.*`): Main application class
- **MainFrame** (`wxsrc/MainFrame.*`): Main window with menus, toolbars, and layout management
- **CircuitCanvas** (`wxsrc/CircuitCanvas.*`): Canvas for placing and connecting components
- **ComponentPalette** (`wxsrc/ComponentPalette.*`): Panel for selecting components to add
- **PropertiesPanel** (`wxsrc/PropertiesPanel.*`): Panel for viewing/editing component properties
- **SimulationController** (`wxsrc/SimulationController.*`): Controls simulation state from GUI
- **SimulationBridge** (`wxsrc/SimulationBridge.*`): Bridges GUI and simulation engine components

### GUI-Simulation Integration:
- **SimulationBridge**: Maps GUI components to simulation components and synchronizes state
- **SimulationInterface** (`wxsrc/SimulationInterface.h`): Abstract interfaces for connecting GUI to simulation engine
- **CircuitCanvas** connects to **SimulationController** for real-time simulation updates

### User-facing actions:
- File operations: New, Open, Save (`.circuit` files)
- Edit operations: Undo, Redo
- View operations: Zoom, Grid display, Snap to grid
- Simulation: Start, Pause, Stop, Step
- Component operations: Add components from palette, connect components, edit properties

## 5. Existing command-line interfaces

### Primary CLI Entry Point:
- **Main function** (`src/ProtoVM/ProtoVM.cpp` with `#ifdef flagMAIN`): Command-line entry point with various options

### CLI Command Support:
- **Options**: `-h/--help`, `-V/--version`, `-v/--verbose`, `-vv`, `--verbosity`, `-t/--ticks N`, `--cli`, `--load-binary`
- **Circuit Selection**: Over 30 different test circuits (flipflop, andgate, counter, memory, 6502, basiclogic, test4bit, etc.)
- **Actions**: Load circuits, run simulation for N ticks, start interactive CLI mode, load binary programs

### Interactive CLI Module:
- **Cli class** (`src/ProtoVM/Cli.h/cpp`): Interactive command-line interface with multiple commands
- **Commands**: `help`, `write`, `read`, `run`, `list`, `inspect`, `state`, `visualize`, `netlist`, `trace`, `tracelog`, `quit`
- **Functionality**: Component inspection, state viewing, signal tracing, simulation control

### Scripted CLI Tools:
- **Shell scripts** (`scripts/`): Various helper scripts for running specific demos and tests
  - `run_4004_demo.sh`, `run_cadc_demo.sh`, `create_4004_binary.sh`, etc.

## 6. State, sessions, and storage

### In-memory structures:
- **Machine class**: Maintains current simulation state across multiple PCBs
- **Pcb class**: Contains component nodes and their connectivity information
- **ElectricNodeBase**: Tracks component state, timing information, and change flags
- **SignalTrace and SignalTransition**: Track signal values and transitions over time

### On-disk storage:
- **.circuit files**: Text-based format for circuit designs (loaded/saved by CircuitSerializer)
- **No database layer**: All data is stored in memory or simple file formats

### Storage access:
- **CircuitSerializer** (`wxsrc/CircuitSerializer.*`): Handles loading/saving of `.circuit` files
- **CircuitData structure** (`wxsrc/CircuitData.h`): Data model for circuit serialization

### Session-like behavior:
- **Simulation state** is maintained in the Machine object during execution
- **Current tick counter** tracks simulation progression
- **Breakpoint system** allows pausing simulation at specified ticks
- **Signal tracing** system maintains logs of signal transitions

### Abstractions for future file/DB support:
- The CircuitSerializer could be extended to support multiple formats
- The Machine and Pcb classes abstract the core simulation state
- The SimulationInterface provides a potential abstraction for storage engines

## 7. Configuration and "project" concepts

### Project/workspace concept:
- **Circuit** is the main project concept, implemented via the CircuitData structure
- **.circuit files** represent saved circuit designs
- **PCB objects** contain the actual components that make up a circuit

### Data model (CircuitData):
- **name**: String identifier for the circuit
- **description**: Text description of the circuit
- **components**: Array of ComponentData objects
- **wires**: Array of WireData objects connecting components

### Definition files:
- **CircuitData.h**: Defines the data structures for circuit serialization
- **CircuitSerializer.cpp**: Implements save/load functionality
- Located in `wxsrc/` directory

### Persistence:
- **File-based**: .circuit files stored in human-readable text format
- **Simple format**: ASCII text with components and wire connections
- **Location**: Stored in filesystem as individual files

## 8. Interaction points for a future AI-friendly CLI

### Good CLI command candidates:

1. **Machine::Tick()** (`src/ProtoVM/Machine.cpp`)
   - File path: `src/ProtoVM/Machine.cpp`
   - Function: Execute one simulation step with convergence algorithm
   - Input: None required (uses internal state)
   - Return: bool indicating success/failure
   - Side effect: Updates simulation state by one step

2. **Machine::RunRtOpsWithChangeDetection()** (`src/ProtoVM/Machine.cpp`)
   - File path: `src/ProtoVM/Machine.cpp`
   - Function: Execute real-time operations with change detection
   - Input: bool reference for change detection
   - Return: bool indicating success
   - Side effect: Processes all operations for current tick

3. **Machine::GenerateNetlist()** (`src/ProtoVM/Machine.cpp`)
   - File path: `src/ProtoVM/Machine.cpp`
   - Function: Generate netlist representation of the circuit
   - Input: int (pcb_id)
   - Return: String containing netlist
   - Side effect: None

4. **Machine::AddSignalToTrace()** (`src/ProtoVM/Machine.cpp`)
   - File path: `src/ProtoVM/Machine.cpp`
   - Function: Add a signal to the tracing system
   - Input: ElectricNodeBase*, String (pin name)
   - Return: None
   - Side effect: Adds signal to monitoring system

5. **Machine::PerformTimingAnalysis()** (`src/ProtoVM/Machine.cpp`)
   - File path: `src/ProtoVM/Machine.cpp`
   - Function: Perform complete timing analysis on circuit
   - Input: None
   - Return: None (outputs to logs)
   - Side effect: Analyzes timing constraints and reports violations

6. **Machine::CreateClockDomain()** (`src/ProtoVM/Machine.cpp`)
   - File path: `src/ProtoVM/Machine.cpp`
   - Function: Create a new clock domain
   - Input: int (frequency in Hz)
   - Return: int (domain ID)
   - Side effect: Adds new clock domain to machine

7. **Machine::ReportProfilingResults()** (`src/ProtoVM/Machine.cpp`)
   - File path: `src/ProtoVM/Machine.cpp`
   - Function: Report performance profiling results
   - Input: None
   - Return: None (outputs to logs)
   - Side effect: Displays profiling data

8. **Cli::ProcessCommand()** (`src/ProtoVM/Cli.cpp`)
   - File path: `src/ProtoVM/Cli.cpp`
   - Function: Process a CLI command string
   - Input: String (command)
   - Return: None
   - Side effect: Executes the appropriate action based on command

### Session ID abstractions:
- Current tick number serves as a basic session identifier
- Machine object instance could be referenced by an ID in a multi-simulation environment
- Breakpoint system could be extended to support session-specific breakpoints

## 9. Potential risks / complexity hotspots

### Complicated areas:
- **Convergence algorithm in Machine::Tick()**: Complex logic to handle feedback loops and oscillations
- **Topological sorting and dependency management**: Non-trivial graph algorithms for component evaluation order
- **Mixed analog-digital simulation**: Complex interaction between different simulation paradigms
- **Timing constraint checking**: Complex handling of setup/hold time requirements across clock domains

### Highly coupled areas:
- **ElectricNodeBase and Machine**: Deep integration between component base class and simulation engine
- **Pcb and Link**: Complex interdependencies for connection management
- **GUI and simulation components**: The bridge between visual representation and simulation state

### Hard to expose as simple CLI commands:
- **Real-time signal tracing**: Complex state management across multiple simulation runs
- **Convergence algorithm internals**: Requires understanding of complex internal state
- **Analog simulation integration**: Difficult to abstract from digital components

## 10. Summary for future CLI design

ProtoVM is architected around a component-based simulation engine with a clear separation between the core simulation logic and the GUI. The main simulation engine (Machine) manages components on PCBs, handles connectivity through Links, and executes simulation steps with a sophisticated convergence algorithm.

The most natural CLI integration points are the public methods of the Machine class which already support the existing CLI functionality. These methods are well-encapsulated and provide access to simulation control, component inspection, timing analysis, and circuit visualization.

The current architecture supports both file-based "repositories" (the .circuit files) through the CircuitSerializer and could be extended to support database-backed storage by implementing similar interfaces. The CircuitData structure provides a clean abstraction for circuit persistence.

Session IDs could be implemented by creating multiple Machine instances and tracking them with string or numeric IDs. The existing breakpoint and signal tracing systems already provide hooks for long-running operations with state preservation.

For an AI-friendly CLI, the existing Cli class provides a strong foundation that could be enhanced with more structured output formats (JSON/XML) and more granular command options while keeping the same underlying simulation engine calls.