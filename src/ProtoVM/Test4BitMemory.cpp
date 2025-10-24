#include "ProtoVM.h"


// 4-bit memory module with address decoding
class Memory4Bit : public ElcBase {
	//RTTI_DECL1(Memory4Bit, ElcBase);
	
	// Memory array - 16 locations of 4 bits each (total 64 bits)
	byte mem[16];
	
	// Address inputs (4 bits for 16 locations)
	bool addr[4] = {0, 0, 0, 0};
	
	// Data inputs (4 bits)
	bool din[4] = {0, 0, 0, 0};
	
	// Control inputs
	bool cs = 0;    // Chip select
	bool oe = 1;    // Output enable (active low)
	bool we = 1;    // Write enable (active low)
	
	// Data outputs (4 bits)
	bool dout[4] = {0, 0, 0, 0};
	
	// Internal state
	int decoded_addr = 0;
	
public:
	Memory4Bit() {
		// Initialize memory to zero
		memset(mem, 0, sizeof(mem));
		
		// Add address inputs
		AddSink("A0");
		AddSink("A1");
		AddSink("A2");
		AddSink("A3");
		
		// Add data inputs
		AddSink("D0");
		AddSink("D1");
		AddSink("D2");
		AddSink("D3");
		
		// Add control inputs
		AddSink("~CS");  // Chip select (active low)
		AddSink("~OE");  // Output enable (active low)
		AddSink("~WE");  // Write enable (active low)
		
		// Add data outputs
		AddSource("Q0").SetMultiConn();
		AddSource("Q1").SetMultiConn();
		AddSource("Q2").SetMultiConn();
		AddSource("Q3").SetMultiConn();
	}
	
	bool Tick() override {
		// Decode address lines to get memory location
		decoded_addr = (addr[3] << 3) | (addr[2] << 2) | (addr[1] << 1) | addr[0];
		
		// Ensure address is within bounds
		if (decoded_addr >= 16) {
			decoded_addr = 0;  // Default to address 0 if out of bounds
		}
		
		// If chip is selected
		if (!cs) {  // CS is active low
			// Handle write operation
			if (!we) {  // WE is active low
				// Write data to memory location
				byte data = (din[3] << 3) | (din[2] << 2) | (din[1] << 1) | din[0];
				mem[decoded_addr] = data;
			}
			
			// Handle read operation
			if (!oe) {  // OE is active low
				// Read data from memory location
				byte data = mem[decoded_addr];
				dout[0] = data & 1;
				dout[1] = (data >> 1) & 1;
				dout[2] = (data >> 2) & 1;
				dout[3] = (data >> 3) & 1;
			} else {
				// Tri-state outputs when not enabled
				dout[0] = 0;
				dout[1] = 0;
				dout[2] = 0;
				dout[3] = 0;
			}
		} else {
			// Tri-state outputs when chip is not selected
			dout[0] = 0;
			dout[1] = 0;
			dout[2] = 0;
			dout[3] = 0;
		}
		
		return true;
	}
	
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
		if (type == WRITE) {
			// Handle input connections - skip as they're handled by PutRaw
			switch (conn_id) {
			case 0:  // A0
			case 1:  // A1
			case 2:  // A2
			case 3:  // A3
			case 4:  // D0
			case 5:  // D1
			case 6:  // D2
			case 7:  // D3
			case 8:  // ~CS
			case 9:  // ~OE
			case 10: // ~WE
				// Inputs handled by PutRaw
				break;
			case 11: // Q0 (Output 0)
				return dest.PutRaw(dest_conn_id, (byte*)&dout[0], 0, 1);
				break;
			case 12: // Q1 (Output 1)
				return dest.PutRaw(dest_conn_id, (byte*)&dout[1], 0, 1);
				break;
			case 13: // Q2 (Output 2)
				return dest.PutRaw(dest_conn_id, (byte*)&dout[2], 0, 1);
				break;
			case 14: // Q3 (Output 3)
				return dest.PutRaw(dest_conn_id, (byte*)&dout[3], 0, 1);
				break;
			default:
				LOG("error: Memory4Bit: unimplemented conn-id " << conn_id);
				return false;
			}
		}
		else {
			LOG("error: Memory4Bit: unimplemented ProcessType");
			return false;
		}
		return true;
	}
	
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
		ASSERT(data_bytes == 0 && data_bits == 1);
		bool val = *data & 1;
		
		switch (conn_id) {
		case 0: // A0 (Address bit 0)
			addr[0] = val;
			break;
		case 1: // A1 (Address bit 1)
			addr[1] = val;
			break;
		case 2: // A2 (Address bit 2)
			addr[2] = val;
			break;
		case 3: // A3 (Address bit 3)
			addr[3] = val;
			break;
		case 4: // D0 (Data input bit 0)
			din[0] = val;
			break;
		case 5: // D1 (Data input bit 1)
			din[1] = val;
			break;
		case 6: // D2 (Data input bit 2)
			din[2] = val;
			break;
		case 7: // D3 (Data input bit 3)
			din[3] = val;
			break;
		case 8: // ~CS (Chip select - active low)
			cs = val;
			break;
		case 9: // ~OE (Output enable - active low)
			oe = val;
			break;
		case 10: // ~WE (Write enable - active low)
			we = val;
			break;
		default:
			LOG("error: Memory4Bit: unimplemented conn-id " << conn_id);
			return false;
		}
		return true;
	}
};

// Test function for 4-bit memory
void Test4BitMemory(Machine& mach) {
	Pcb& b = mach.AddPcb();
	
	// Create reference pins
	Pin& ground = b.Add<Pin>("ground");
	ground.SetReference(0);
	Pin& vcc = b.Add<Pin>("vcc");
	vcc.SetReference(1);
	
	// Create the 4-bit memory module
	Memory4Bit& mem4bit = b.Add<Memory4Bit>("mem4bit");
	
	// Connect address lines to ground (address 0)
	ground["0"] >> mem4bit["A0"];
	ground["0"] >> mem4bit["A1"];
	ground["0"] >> mem4bit["A2"];
	ground["0"] >> mem4bit["A3"];
	
	// Connect data lines to ground (data = 0)
	ground["0"] >> mem4bit["D0"];
	ground["0"] >> mem4bit["D1"];
	ground["0"] >> mem4bit["D2"];
	ground["0"] >> mem4bit["D3"];
	
	// Connect control lines
	vcc["0"] >> mem4bit["~CS"];  // Chip not selected initially
	vcc["0"] >> mem4bit["~OE"];  // Output disabled initially
	vcc["0"] >> mem4bit["~WE"];  // Write disabled initially
	
	// Connect outputs to vcc for electrical continuity
	mem4bit["Q0"] >> vcc["0"];
	mem4bit["Q1"] >> vcc["0"];
	mem4bit["Q2"] >> vcc["0"];
	mem4bit["Q3"] >> vcc["0"];
	
	LOG("4-bit memory test circuit initialized");
	LOG("Memory size: 16 locations × 4 bits (64 bits total)");
	LOG("Default address: 0, Default data: 0");
	LOG("Control lines: ~CS=1, ~OE=1, ~WE=1");
}