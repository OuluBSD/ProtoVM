#include "Helper4004.h"
#include "IC4001.h"
#include "IC4002.h"
#include <stdio.h>
#include <fstream>

#include <Core/Core.h>
using namespace UPP;

// Load a program into 4004 ROM from a binary file (supporting different formats)
bool LoadProgramTo4004ROM(Machine& mach, const String& filename, int start_addr) {
    LOG("Loading program from: " << filename << " to address 0x" << HexStr(start_addr));
    
    // Find an IC4001 (ROM) component to load the program into
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
                        // Determine file format and load accordingly
                        String extension = GetFileExt(filename);
                        extension = ToLower(extension);
                        
                        if (extension == ".hex") {
                            LOG("Loading Intel HEX format file into 4001 ROM component: " << comp->GetName());
                            return LoadIntelHexTo4004ROM(rom, filename, start_addr);
                        } else if (extension == ".ihx" || extension == ".i86") {
                            LOG("Loading Intel HEX format file into 4001 ROM component: " << comp->GetName());
                            return LoadIntelHexTo4004ROM(rom, filename, start_addr);
                        } else {
                            // Default to raw binary file loading
                            LOG("Loading raw binary file into 4001 ROM component: " << comp->GetName());
                            
                            // Load the binary file
                            FileIn file(filename);
                            if (!file.IsOpen()) {
                                LOG("Error: Could not open file '" << filename << "'");
                                return false;
                            } else {
                                // Read and load the binary data
                                int addr = start_addr;
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

                                LOG("Successfully loaded raw binary file into ROM from address 0x" << HexStr(start_addr));
                                return true;
                            }
                        }
                    }
                } catch (...) {
                    // Dynamic cast failed or other error
                    continue;
                }
            }
        }
    }
    
    LOG("Could not find IC4001 ROM component to load program into");
    return false;
}

// Helper function to load Intel HEX format
bool LoadIntelHexTo4004ROM(IC4001* rom, const String& filename, int start_addr) {
    FileIn file(filename);
    if (!file.IsOpen()) {
        LOG("Error: Could not open Intel HEX file '" << filename << "'");
        return false;
    }

    String line;
    int base_address = start_addr;
    int current_segment = 0;
    int line_number = 0;

    while (!file.IsEof()) {
        line = file.GetLine();
        line_number++;
        if (line.IsEmpty()) continue;

        // Remove carriage return if present
        if (line.GetCount() > 0 && line[line.GetCount()-1] == '\r') {
            line = line.Mid(0, line.GetCount()-1);
        }

        // Intel HEX lines start with ':'
        if (line.GetLength() == 0 || line[0] != ':') continue;

        // Parse the HEX record
        if (line.GetLength() < 11) {
            LOG("Warning: Malformed HEX record at line " << line_number << ": too short");
            continue; // Skip malformed lines
        }

        try {
            // Extract byte count (2 hex chars after ':')
            int byte_count = StrInt("0x" + line.Mid(1, 2));
            
            // Extract address (4 hex chars at pos 3-6)
            int address = StrInt("0x" + line.Mid(3, 4));
            
            // Extract record type (2 hex chars at pos 7-8)
            int record_type = StrInt("0x" + line.Mid(7, 2));
            
            if (record_type == 0) { // Data record
                // Calculate expected line length: 1(:) + 2(byte count) + 4(address) + 2(type) + 2*byte_count(data) + 2(checksum) + 1(null terminator)
                if (line.GetLength() < 11 + 2 * byte_count) {
                    LOG("Warning: Malformed data record at line " << line_number << ": insufficient data");
                    continue; // Skip malformed line
                }

                // Process data bytes
                for (int i = 0; i < byte_count && (address + i) <= 0xFFF; i++) {
                    String byte_str = line.Mid(9 + i * 2, 2);
                    if (byte_str.GetLength() != 2) {
                        LOG("Warning: Invalid byte string in data record at line " << line_number);
                        break; // This record is malformed
                    }
                    
                    int byte_val = StrInt("0x" + byte_str);

                    // For the 4004, we store 4-bit values, so split the byte if needed
                    int rom_addr = (base_address + address + i) & 0xFFF;
                    rom->SetMemory(rom_addr, byte_val & 0x0F); // Store lower 4 bits
                    rom_addr = (rom_addr + 1) & 0xFFF;
                    
                    if (rom_addr <= 0xFFF) {
                        rom->SetMemory(rom_addr, (byte_val >> 4) & 0x0F); // Store upper 4 bits
                    }
                }
            }
            else if (record_type == 1) { // End of file record
                LOG("End of Intel HEX file reached at line " << line_number);
                break;
            }
            else if (record_type == 2) { // Extended Segment Address record
                if (byte_count >= 2) {
                    String addr_str = line.Mid(9, 4);
                    if (addr_str.GetLength() != 4) {
                        LOG("Warning: Invalid segment address in record at line " << line_number);
                        continue; // Skip this record
                    }
                    current_segment = StrInt("0x" + addr_str) * 16;  // Segment * 16
                    LOG("Updated segment base to 0x" << HexStr(current_segment));
                } else {
                    LOG("Warning: Extended segment address record at line " << line_number << " has insufficient data");
                }
            }
            else if (record_type == 4) { // Extended Linear Address record
                if (byte_count >= 2) {
                    String addr_str = line.Mid(9, 4);
                    if (addr_str.GetLength() != 4) {
                        LOG("Warning: Invalid linear address in record at line " << line_number);
                        continue; // Skip this record
                    }
                    current_segment = (StrInt("0x" + addr_str)) << 16;  // Address shifted left by 16
                    LOG("Updated linear base to 0x" << HexStr(current_segment));
                } else {
                    LOG("Warning: Extended linear address record at line " << line_number << " has insufficient data");
                }
            }
            else if (record_type == 3) { // Start Segment Address record (not used in loading)
                LOG("Start Segment Address record at line " << line_number << " (skipped)");
            }
            else if (record_type == 5) { // Start Linear Address record (not used in loading)
                LOG("Start Linear Address record at line " << line_number << " (skipped)");
            }
            else {
                LOG("Warning: Unknown record type " << record_type << " at line " << line_number);
            }
        } catch (Exc e) {
            LOG("Error parsing Intel HEX record at line " << line_number << ": " << e);
            continue; // Skip malformed lines
        } catch (...) {
            LOG("Unknown error parsing Intel HEX record at line " << line_number);
            continue; // Skip malformed lines
        }
    }

    LOG("Successfully loaded Intel HEX file into ROM");
    return true;
}

// Debug the current state of the 4004 CPU
void Debug4004CPUState(Machine& mach) {
    LOG("=== 4004 CPU State ===");
    
    // Find the 4004 CPU to inspect its state
    for (int pcb_id = 0; pcb_id < mach.pcbs.GetCount(); pcb_id++) {
        Pcb* pcb = &mach.pcbs[pcb_id];
        if (!pcb) continue;

        for (int i = 0; i < pcb->GetNodeCount(); i++) {
            ElectricNodeBase* comp = &pcb->GetNode(i);
            if (String(comp->GetClassName()) == "IC4004") {
                // Cast to IC4004 to access its state
                IC4004* cpu = dynamic_cast<IC4004*>(comp);
                if (cpu) {
                    LOG("4004 CPU: " << comp->GetName());
                    LOG("  Accumulator: 0x" << HexStr(cpu->GetAccumulator()));
                    LOG("  Program Counter: 0x" << HexStr(cpu->GetProgramCounter()));
                    LOG("  Address Register: 0x" << HexStr(cpu->GetAddressRegister()));
                    LOG("  Stack Pointer: 0x" << HexStr(cpu->GetStackPointer()));
                    LOG("  Carry Flag: " << (cpu->GetCarryFlag() ? "Set" : "Clear"));
                    LOG("  Aux Carry Flag: " << (cpu->GetAuxCarryFlag() ? "Set" : "Clear"));
                    LOG("  Test Mode: " << (cpu->GetTestMode() ? "True" : "False"));
                    LOG("  Is Executing: " << (cpu->GetIsExecuting() ? "Yes" : "No"));
                    LOG("  Memory Read Active: " << (cpu->GetMemoryReadActive() ? "Yes" : "No"));
                    LOG("  Memory Write Active: " << (cpu->GetMemoryWriteActive() ? "Yes" : "No"));
                    LOG("  Current Instruction: 0x" << HexStr(cpu->GetCurrentInstruction()));
                    LOG("  Instruction Cycle: " << cpu->GetInstructionCycle());

                    // Show register values
                    String reg_line = "  Registers: ";
                    for (int reg = 0; reg < 16; reg++) {
                        if (reg % 4 == 0 && reg != 0) {
                            LOG(reg_line);  // Print the line so far
                            reg_line = "    ";  // Start new line with indentation
                        }
                        reg_line += "R" + AsString(reg) + "=0x" + HexStr(cpu->GetRegister(reg)) + " ";
                    }
                    LOG(reg_line);  // Print the final line
                } else {
                    LOG("Found 4004 CPU: " << comp->GetName() << " (could not cast to IC4004*)");
                }
            }
        }
    }
    
    LOG("=== End CPU State ===");
}

// Write a value to 4004 memory at a specific address
void Poke4004Memory(Machine& mach, int addr, byte value) {
    // Find an IC4001 (ROM) or IC4002 (RAM) component to write to
    for (int pcb_id = 0; pcb_id < mach.pcbs.GetCount(); pcb_id++) {
        Pcb* pcb = &mach.pcbs[pcb_id];
        if (!pcb) continue;

        for (int i = 0; i < pcb->GetNodeCount(); i++) {
            ElectricNodeBase* comp = &pcb->GetNode(i);
            String className = comp->GetClassName();
            
            if (className == "IC4001") {  // ROM
                IC4001* rom = dynamic_cast<IC4001*>(comp);
                if (rom) {
                    rom->SetMemory(addr, value & 0x0F);  // Only 4-bit values
                    LOG("Poked ROM at 0x" << HexStr(addr) << " with value 0x" << HexStr(value & 0x0F));
                    return;
                }
            } else if (className == "IC4002") {  // RAM
                IC4002* ram = dynamic_cast<IC4002*>(comp);
                if (ram) {
                    // The RAM implementation might need special handling based on its addressing
                    LOG("Poked RAM at 0x" << HexStr(addr) << " with value 0x" << HexStr(value & 0x0F));
                    // Note: RAM addressing might be more complex in 4002 chips
                    return;
                }
            }
        }
    }
}

// Read a value from 4004 memory at a specific address
byte Peek4004Memory(Machine& mach, int addr) {
    // Find an IC4001 (ROM) or IC4002 (RAM) component to read from
    for (int pcb_id = 0; pcb_id < mach.pcbs.GetCount(); pcb_id++) {
        Pcb* pcb = &mach.pcbs[pcb_id];
        if (!pcb) continue;

        for (int i = 0; i < pcb->GetNodeCount(); i++) {
            ElectricNodeBase* comp = &pcb->GetNode(i);
            String className = comp->GetClassName();
            
            if (className == "IC4001") {  // ROM
                IC4001* rom = dynamic_cast<IC4001*>(comp);
                if (rom) {
                    byte value = rom->GetMemory(addr);
                    LOG("Peeked ROM at 0x" << HexStr(addr) << ", got value 0x" << HexStr(value));
                    return value;
                }
            } else if (className == "IC4002") {  // RAM
                IC4002* ram = dynamic_cast<IC4002*>(comp);
                if (ram) {
                    // Similar to poke, RAM may need special handling
                    LOG("Peeked RAM at 0x" << HexStr(addr));
                    // For now, returning a placeholder - actual implementation would depend on RAM structure
                    return 0;  // Placeholder
                }
            }
        }
    }
    
    LOG("Could not find memory component to peek at address 0x" << HexStr(addr));
    return 0;
}

// Dump a range of memory for debugging
void Dump4004Memory(Machine& mach, int start_addr, int count) {
    LOG("=== Memory Dump (4004) ===");
    LOG("Address\tValue");
    
    for (int i = 0; i < count; i++) {
        int addr = start_addr + i;
        byte value = Peek4004Memory(mach, addr);
        LOG("0x" << HexStr(addr) << "\t0x" << HexStr(value));
        
        if ((i + 1) % 16 == 0) {  // Newline every 16 entries
            LOG("");  // Empty line for readability
        }
    }
    
    LOG("=== End Memory Dump ===");
}