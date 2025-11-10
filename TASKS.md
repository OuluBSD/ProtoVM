# ProtoVM Digital Logic Simulation Tasks

## TODO

### Schematic Drawing Tools
- [ ] The MDS-1104 is very early single-transistor calculator from 1950s

### 4004 Circuit Connection Fixes
- [ ] Fix missing connections in MiniMax4004 circuit setup for proper CPU operation
- [ ] Connect CPU control signals (CM, BUSY, R/W, MR, MW, SBY, CM4, RES) properly
- [ ] Connect ROM address, data, and control lines (A0-A11, O0-O3, CM4, JAM)
- [ ] Connect RAM address, data, and control lines (A0-A3, I0-I3, O0-O3, CM4, WM)
- [ ] Connect I/O shift register data and control lines (SR0, SO0, O0-O3, L0-L3, CM4)
- [ ] Connect address decoder to proper address lines and chip select outputs
- [ ] Connect clock generator to CPU and memory components
- [ ] Connect power-on reset circuit to CPU and other components
- [ ] Verify all pin connections are properly established in SetupMiniMax4004 function

### Binary Loading and Execution
- [ ] Implement proper binary loader for 4004 programs into ROM
- [ ] Create helper functions for memory initialization and debugging
- [ ] Add CLI commands for loading and managing 4004 binaries
- [ ] Implement memory initialization functions to load programs at specific addresses
- [ ] Create debugging functions to inspect CPU state during execution
- [ ] Add support for different binary formats (raw, Intel HEX, etc.)
- [ ] Implement error handling for binary loading operations

### 4004 Program Execution
- [ ] Fix circuit initialization to allow proper program execution
- [ ] Configure clock signals for proper 4004 timing
- [ ] Implement I/O port handling for character output
- [ ] Test 4004_putchar.bin program execution
- [ ] Verify that the 'A' character is properly output from the program
- [ ] Create additional test programs to validate functionality
- [ ] Document the execution process and troubleshooting steps

## IN PROGRESS


## DONE

### Basic Logic Components
- [x] Implement NAND gate component
- [x] Implement NOR gate component
- [x] Implement XOR gate component
- [x] Implement XNOR gate component
- [x] Implement NOT gate component
- [x] Create multiplexer component for data routing
- [x] Create demultiplexer component for data routing
- [x] Implement decoder component for addressing
- [x] Implement encoder component for addressing

### Memory Components
- [x] Fix existing "memory" example circuit
- [x] Implement register components with multiple bits  # D flip-flop provides basic register functionality
- [x] Add register enable/clear functionality # Enhanced D flip-flop now includes Enable and Clear inputs
- [x] Create memory circuits with addressable storage # Implemented 4-bit register component
- [x] Build 4-bit memory with address and data lines # Implemented 4-bit memory with 16 locations × 4 bits

### Complex Test Circuits
- [x] Create 4-bit adder/subtractor using basic gates
- [x] Create ALU (Arithmetic Logic Unit) components
- [x] Implement state machine components for control logic
- [x] Build more complex CPU core examples

### CLI Enhancements
- [x] Add commands to inspect component states during simulation
- [x] Implement breakpoint functionality for debugging circuits
- [x] Add signal tracing to monitor specific signals during simulation
- [x] Create visualization commands to show circuit connections

### Simulation Features
- [x] Add timing analysis tools to measure propagation delays
- [x] Implement signal transition logging to track when signals change
- [x] Add waveform generation for visual representation of signals over time
- [x] Create performance profiling to identify bottlenecks in large circuits

### Clock System Enhancements
- [x] Add multiple clock domains with different frequencies
- [x] Implement clock dividers to generate slower clocks from faster ones
- [x] Add clock gating functionality for power optimization
- [x] Implement phase-locked loop (PLL) simulation for frequency synthesis

### Circuit Design Tools
- [x] Add a simple schematic editor or netlist parser
- [x] Create a library of standard components that can be easily instantiated
- [x] Implement component hierarchy for building modular designs
- [x] Add parameterized components that can be configured by size or function

### Verification and Testing
- [x] Implement unit testing framework for individual components
- [x] Create a test vector generator for comprehensive verification
- [x] Add formal verification tools for critical circuits
- [x] Build in fault injection capabilities to test robustness

### Documentation and Examples
- [x] Create detailed tutorials for building complex circuits
- [x] Add more comprehensive examples of real digital systems
- [x] Document the API for component creation and simulation
- [x] Create best practices guidelines for efficient circuit design

### Schematic Drawing Tools
- [x] Add tools for GUI app to draw schematics based on PCB images in "circuitboards/MDS-1101/"

### Schematic Programming Language
- [x] Design Pythonic schematic language with indentation-based blocks instead of C-like {}
- [x] Create directory structure: "scripts/" for computer schematics (6502, uk101, interak, minimax) and "tests/" for unit tests
- [x] Implement parser for the new schematic language
- [x] Develop compiler/transpiler to convert schematic language to ProtoVM C++ components
- [x] Create test framework to validate expected outputs for specific inputs
- [x] Write initial schematic files for 6502, uk101, interak, and minimax computers
- [x] Implement unit tests for basic components using the new language
- [x] Add documentation and examples for the schematic programming language
- [x] Integrate schematic language execution into the main ProtoVM application

### Intel 4004 CPU Implementation
- [x] Add Intel 4004 CPU component implementation following 6502 example
- [x] Implement 4004 instruction set architecture (ISA) with 45 instructions
- [x] Create 4-bit accumulator and 12 registers (4-bit each)
- [x] Implement 4002 RAM (40 bytes of 4-bit memory per chip)
- [x] Implement 4003 shift register for I/O expansion
- [x] Implement 4001 ROM (2048 8-bit words, 256 bytes per chip)
- [x] Create bus interface for 4-bit data and 12-bit address
- [x] Add support for 4004 timing requirements and clock signals
- [x] Implement proper handling of BUSY and CM signals
- [x] Add support for 4004 instruction fetch/decode/execute cycle

### Minimax Computer System with 4004
- [x] Design Minimax computer system schematic using 4004 CPU
- [x] Integrate 4001 ROM chips for program storage (2048 x 8-bit words)
- [x] Implement memory mapping for 4001 ROM and 4002 RAM chips
- [x] Add I/O subsystem using 4003 shift register
- [x] Implement address decoding for memory and I/O chips
- [x] Create proper data/address bus connections between components
- [x] Add clock generator for 4004 timing requirements
- [x] Implement reset circuitry for proper initialization
- [x] Add power-on sequence handling

### HLA3 Assembler Integration
- [x] Download and set up High-Level Assembler 3 (HLA3) in external/thirdparty directory
- [x] Create directory structure for HLA3: "external/hla3/"
- [x] Set up HLA3 "Assembler Developer's Kit" for creating 4004 assembler
- [x] Develop 4004-specific assembler using HLA3 kit
- [x] Create 4004 instruction syntax definitions
- [x] Implement 4004 assembler code generation
- [x] Create example programs for 4004 to demonstrate functionality
- [x] Document how to write and assemble programs for 4004

### CLI and Runtime Enhancements
- [x] Add 4004 computer system to circuit selection in main executable
- [x] Implement command-line option to load binary program file into memory
- [x] Add option to switch between serial I/O and CLI output
- [x] Create memory poking commands in CLI to directly access memory
- [x] Add CPU register inspection commands in CLI
- [x] Implement interactive mode to control program execution (start, stop, step)
- [x] Add support for loading and running programs from binary files
- [x] Create command to show CPU state during program execution
- [x] Add memory dump functionality in CLI

### Testing and Verification
- [x] Create unit tests for 4004 CPU component
- [x] Verify 4004 instruction execution accuracy
- [x] Test memory read/write operations with 4001 ROM and 4002 RAM
- [x] Validate I/O operations through 4003 shift register
- [x] Create comprehensive test programs for 4004 functionality
- [x] Verify timing behavior and clock synchronization
- [x] Test complex programs that use multiple 4004 components
- [x] Validate interrupt and control signal handling

- [x] Implement multi-tick convergence algorithm in Machine::Tick()
- [x] Add proper bus arbitration with tri-state buffer support
- [x] Modify Bus classes to support tri-state logic
- [x] Update IC6502 to handle bidirectional buses properly
- [x] Update ICRamRom to handle bidirectional buses properly
- [x] Implement oscillation detection to prevent infinite loops
- [x] Add feedback loop resolution within each tick
- [x] Implement state change detection to optimize processing
- [x] Add propagation delay modeling system
- [x] Model setup and hold time constraints
- [x] Add topological ordering for component evaluation
- [x] Add timing simulation for different clock domains
- [x] Enhanced ProtoVM digital logic simulation with convergence algorithm
- [x] Added proper tri-state bus support with driver arbitration
- [x] Enhanced component change detection to optimize processing
- [x] Added feedback loop resolution within each tick
- [x] Implemented oscillation detection to prevent infinite loops
- [x] Improved state tracking for more accurate simulation
- [x] Added command-line interface with help and version options
- [x] Implemented circuit selection via command-line arguments
- [x] Added support for multiple test circuits
- [x] Added interactive CLI mode for circuit debugging
- [x] Updated project documentation and task tracking
- [x] Enhanced 4-bit counter implementation with clock generator
- [x] Enhanced AND gate test with dynamic inputs- [ ] Fix "trying to connect two sources" error in 4004 CPU simulation
