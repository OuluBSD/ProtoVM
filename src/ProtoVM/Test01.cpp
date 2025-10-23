#include "ProtoVM.h"


void SetupTest3_Memory(Machine& mach) {
	// Create a simple memory test circuit
	Pcb& b = mach.AddPcb();
	
	// Create reference pins
	Pin& ground = b.Add<Pin>("ground");
	ground.SetReference(0);
	Pin& vcc = b.Add<Pin>("vcc");
	vcc.SetReference(1);
	
	// Connect reference pins to maintain electrical continuity
	ground["0"] >> vcc["0"];
	
	// Create the 32KB RAM chip
	IC62256& ram32k = b.Add<IC62256>("ram32k");
	
	// Connect control lines
	ground["0"] >> ram32k["~CS"];   // Chip select active (low)
	vcc["0"] >> ram32k["~OE"];      // Output enable inactive (high)
	vcc["0"] >> ram32k["~WR"];      // Write enable inactive (high)
	
	// Connect address lines to ground (they need to be driven)
	// For a simple test, we'll tie all address lines to ground (address 0)
	for (int i = 0; i < 16; i++) {
		ground["0"] >> ram32k["A" + IntStr(i)];
	}
	
	// Connect data lines to ground
	// For a simple test, we'll tie all data lines to ground
	for (int i = 0; i < 8; i++) {
		ground["0"] >> ram32k["D" + IntStr(i)];
	}
	
	LOG("Memory test circuit initialized with 32KB RAM (IC62256)");
	LOG("RAM chip control lines set: ~CS=0, ~OE=1, ~WR=1");
	LOG("All address and data lines grounded for simplicity");
	LOG("Power continuity maintained through ground-vcc connection");
}