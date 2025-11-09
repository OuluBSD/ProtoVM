#ifndef _ProtoVM_Helper4004_h_
#define _ProtoVM_Helper4004_h_

#include "ProtoVM.h"

// Helper functions for 4004 memory initialization and debugging
bool LoadProgramTo4004ROM(Machine& mach, const String& filename, int start_addr);
void Debug4004CPUState(Machine& mach);
void Poke4004Memory(Machine& mach, int addr, byte value);
byte Peek4004Memory(Machine& mach, int addr);
void Dump4004Memory(Machine& mach, int start_addr, int count);

// Helper functions for binary format loading
bool LoadIntelHexTo4004ROM(IC4001* rom, const String& filename, int start_addr);

#endif