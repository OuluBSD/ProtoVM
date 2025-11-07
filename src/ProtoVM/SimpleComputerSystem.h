#ifndef _ProtoVM_SimpleComputerSystem_h_
#define _ProtoVM_SimpleComputerSystem_h_

#include "ProtoVM.h"
#include "StandardLibrary.h"
#include "ComponentHierarchy.h"
#include "ALU.h"
#include "IC6502.h"
#include "ICRamRom.h"

// A complete, simplified computer system based on 6502 processor
class SimpleComputerSystem : public HierarchicalComponent {
private:
    // Main CPU
    IC6502* cpu;
    
    // Memory components
    ICRamRom* rom;     // Read-only memory for bootloader and BASIC interpreter
    ICRamRom* ram;     // Random access memory for user programs and data
    
    // I/O components
    Bus8* data_bus;
    Bus16* addr_bus;
    
    // Control logic
    ElcNot* read_not;      // Invert CPU R/W to get read signal
    ElcNot* write_not;     // Invert CPU R/W to get write signal
    Mux4to1* addr_mux;    // For address decoding
    
    // Memory mapping
    int rom_start_addr;
    int rom_end_addr;
    int ram_start_addr;
    int ram_end_addr;
    
public:
    SimpleComputerSystem();
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
    
    // Memory mapping methods
    void SetMemoryMap(int rom_start = 0xE000, int rom_end = 0xFFFF, 
                      int ram_start = 0x0000, int ram_end = 0xDFFF);
    
    // Program loading methods
    void LoadROMProgram(const byte* program, int size);
    void LoadRAMData(const byte* data, int size, int start_addr = 0x0000);
    
    // Diagnostic methods
    void RunSelfTest();
    void DumpSystemStatus();
};

// A complete implementation of the UK101 computer system
class UK101System : public HierarchicalComponent {
private:
    // Main components
    IC6502* cpu;                    // 6502 CPU
    ICRamRom* rom_bios;            // BIOS ROM (Monitor)
    ICRamRom* rom_basic;           // BASIC ROM
    ICRamRom* ram_main;            // Main RAM
    
    // I/O components
    IC6850* acia;                  // Asynchronous Communication Interface Adapter
    Bus8* data_bus;
    Bus16* addr_bus;
    
    // Address decoding logic
    Decoder3to8* addr_decoder;     // For memory mapping
    ElcNot* rw_invert;             // Invert CPU R/W signal
    ElcNand* ram_cs_nand;         // RAM chip select logic
    ElcNand* rom_cs_nand;         // ROM chip select logic
    
    // Clock generation
    ClockDivider* clock_divider;   // To generate appropriate system clock
    
    // Memory mapping (addresses in hex)
    static const int RAM_START  = 0x0000;  // 0x0000-0x7FFF (32KB)
    static const int RAM_END    = 0x7FFF;
    static const int BASIC_START = 0xA000; // 0xA000-0xBFFF (8KB)
    static const int BASIC_END   = 0xBFFF;
    static const int BIOS_START  = 0xF800; // 0xF800-0xFFFF (2KB)
    static const int BIOS_END    = 0xFFFF;
    
public:
    UK101System();
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
    
    // Setup methods
    void LoadBIOS(const byte* bios_code, int size);
    void LoadBASIC(const byte* basic_code, int size);
    
    // Utility methods
    void DumpMemoryMap();
    void RunUK101Diagnostics();
};

// Interak computer system example - a Z80-based system
class InterakSystem : public HierarchicalComponent {
private:
    // Note: We'll simulate with 6502 as Z80 implementation might not be available
    IC6502* cpu;                   // Using 6502 to simulate Z80 functionality
    ICRamRom* rom_monitor;         // Monitor ROM
    ICRamRom* ram_main;            // Main RAM
    
    // I/O components
    Bus8* data_bus;
    Bus16* addr_bus;
    
    // Control logic
    ElcNot* rd_invert;             // Read signal inverter
    ElcNot* wr_invert;             // Write signal inverter
    Decoder2to4* mem_decoder;      // Simple memory decoder
    
    // Memory mapping
    static const int RAM_START  = 0x0000;  // Main RAM
    static const int RAM_SIZE   = 0x2000;  // 8KB RAM
    static const int ROM_START  = 0xE000;  // Monitor ROM
    static const int ROM_SIZE   = 0x2000;  // 8KB ROM
    
public:
    InterakSystem();
    
    void SetupSubcomponents() override;
    void ConnectSubcomponents() override;
    bool Tick() override;
    
    // Setup methods
    void LoadMonitor(const byte* monitor_code, int size);
    
    // Diagnostic methods
    void RunInterakDiagnostics();
};

// A comprehensive test program for validation
class ComprehensiveSystemTest {
public:
    static void TestSimpleComputerSystem(Machine& machine);
    static void TestUK101System(Machine& machine);
    static void TestInterakSystem(Machine& machine);
    
    // Common test routines
    static void TestMemoryRead(Machine& machine, int addr, byte expected_value);
    static void TestMemoryWrite(Machine& machine, int addr, byte value);
    static void TestCPUExecution(Machine& machine, int instruction_address);
    static void TestIOOperation(Machine& machine, int io_addr);
};

#endif