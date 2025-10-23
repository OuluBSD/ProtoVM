#include "ProtoVM.h"




Pin::Pin() {
	AddBidirectional("bi");
	
}





Pin& Pin::SetReference(bool is_high) {
	this->is_high = is_high;
	Clear();
	AddBidirectional("0").SetMultiConn();
	return *this;
}

bool Pin::Tick() {
	return true;
}

bool Pin::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		ASSERT(bytes == 0 && bits == 1);
		return dest.PutRaw(dest_conn_id, &is_high, 0, 1);
	}
	else {
		LOG("error: Pin: unimplemented ProcessType");
		return false;
	}
	return true;
}


bool Pin::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	// this can be put if it's high, because... that's the order of connections
	return is_high;
}

int Pin::GetFixedPriority() const {
	return is_high ? INT_MAX : 0;
}







FlipFlopJK::FlipFlopJK() {
	AddSink("Ck");
	AddSource("~Pr").SetMultiConn();
	AddSource("~Clr").SetMultiConn();
	AddSource("D").SetMultiConn();
	AddSource("~Q").SetMultiConn();
	
}




Crystal::Crystal() {
	AddSink("I");
	AddSource("O").SetMultiConn();
	
}

ElcNor::ElcNor() {
	AddSink("I0");
	AddSink("I1");
	AddSource("O").SetMultiConn();
	
}

bool ElcNor::Tick() {
	out = !(in0 || in1);
	return true;
}

bool ElcNor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	/*if (type == READ) {
		TODO
	}
	else*/ if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I0
		case 1:  // I1
			// skip (write of inputs)
			break;
		case 2:  // O (Output)
			return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: ElcNor: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool ElcNor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I0
		ASSERT(data_bytes == 0 && data_bits == 1);
		in0 = *data & 1;
		break;
	case 1: // I1
		ASSERT(data_bytes == 0 && data_bits == 1);
		in1 = *data & 1;
		break;
	default:
		LOG("error: ElcNor: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

ElcNand::ElcNand() {
	AddSink("I0");
	AddSink("I1");
	AddSource("O").SetMultiConn();
	
}

bool ElcNand::Tick() {
	out = !(in0 && in1);
	LOG("ElcNand::Tick(" << GetName() << "): " << 1*in0 << ", " << 1*in1 << ", " << 1*out);
	return true;
}

bool ElcNand::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	/*if (type == READ) {
		TODO
	}
	else*/ if (type == WRITE) {
		switch (conn_id) {
		case 0:
		case 1:
			// skip (write of RW)
			break;
		case 2:
			return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
			break;
		default:
			LOG("error: ElcNand: unimplemented conn-id");
			return false;
		}
	}
	else {
		LOG("error: ElcNand: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool ElcNand:: PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I0
		ASSERT(data_bytes == 0 && data_bits == 1);
		in0 = *data & 1;
		break;
	case 1: // I1
		ASSERT(data_bytes == 0 && data_bits == 1);
		in1 = *data & 1;
		break;
	default:
		LOG("error: ElcNand: unimplemented conn-id");
		return false;
	}
	return true;
}

ElcNot::ElcNot() {
	AddSink("I");
	AddSource("O").SetMultiConn();
	
}

ElcXor::ElcXor() {
	AddSink("I0");
	AddSink("I1");
	AddSource("O").SetMultiConn();
	
}

bool ElcXor::Tick() {
	out = in0 ^ in1;  // XOR operation
	return true;
}

bool ElcXor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	/*if (type == READ) {
		TODO
	}
	else*/ if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I0
		case 1:  // I1
			// skip (write of inputs)
			break;
		case 2:  // O (Output)
			return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: ElcXor: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool ElcXor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I0
		ASSERT(data_bytes == 0 && data_bits == 1);
		in0 = *data & 1;
		break;
	case 1: // I1
		ASSERT(data_bytes == 0 && data_bits == 1);
		in1 = *data & 1;
		break;
	default:
		LOG("error: ElcXor: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

ElcXnor::ElcXnor() {
	AddSink("I0");
	AddSink("I1");
	AddSource("O").SetMultiConn();
	
}

bool ElcXnor::Tick() {
	out = !(in0 ^ in1);  // XNOR operation (NOT of XOR)
	return true;
}

bool ElcXnor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	/*if (type == READ) {
		TODO
	}
	else*/ if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I0
		case 1:  // I1
			// skip (write of inputs)
			break;
		case 2:  // O (Output)
			return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: ElcXnor: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool ElcXnor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I0
		ASSERT(data_bytes == 0 && data_bits == 1);
		in0 = *data & 1;
		break;
	case 1: // I1
		ASSERT(data_bytes == 0 && data_bits == 1);
		in1 = *data & 1;
		break;
	default:
		LOG("error: ElcXnor: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Mux2to1::Mux2to1() {
	AddSink("I0");      // Input 0
	AddSink("I1");      // Input 1
	AddSink("SEL");     // Select line
	AddSource("O").SetMultiConn(); // Output
	
}

bool Mux2to1::Tick() {
	// If select is 0, output I0, else output I1
	out = sel ? in1 : in0;
	return true;
}

bool Mux2to1::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I0
		case 1:  // I1
		case 2:  // SEL
			// Skip inputs - they are handled by PutRaw
			break;
		case 3:  // O (Output)
			return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Mux2to1: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Mux2to1::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I0
		ASSERT(data_bytes == 0 && data_bits == 1);
		in0 = *data & 1;
		break;
	case 1: // I1
		ASSERT(data_bytes == 0 && data_bits == 1);
		in1 = *data & 1;
		break;
	case 2: // SEL
		ASSERT(data_bytes == 0 && data_bits == 1);
		sel = *data & 1;
		break;
	default:
		LOG("error: Mux2to1: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Mux4to1::Mux4to1() {
	AddSink("I0");      // Input 0
	AddSink("I1");      // Input 1
	AddSink("I2");      // Input 2
	AddSink("I3");      // Input 3
	AddSink("S0");      // Select line 0
	AddSink("S1");      // Select line 1
	AddSource("O").SetMultiConn(); // Output
	
}

bool Mux4to1::Tick() {
	// Use both select lines to choose the appropriate input
	// S1 S0 -> output
	//  0  0  -> I0
	//  0  1  -> I1
	//  1  0  -> I2
	//  1  1  -> I3
	int index = (sel[1] ? 2 : 0) + (sel[0] ? 1 : 0);
	out = in[index];
	return true;
}

bool Mux4to1::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I0
		case 1:  // I1
		case 2:  // I2
		case 3:  // I3
		case 4:  // S0
		case 5:  // S1
			// Skip inputs - they are handled by PutRaw
			break;
		case 6:  // O (Output)
			return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Mux4to1: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Mux4to1::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I0
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[0] = *data & 1;
		break;
	case 1: // I1
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[1] = *data & 1;
		break;
	case 2: // I2
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[2] = *data & 1;
		break;
	case 3: // I3
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[3] = *data & 1;
		break;
	case 4: // S0
		ASSERT(data_bytes == 0 && data_bits == 1);
		sel[0] = *data & 1;
		break;
	case 5: // S1
		ASSERT(data_bytes == 0 && data_bits == 1);
		sel[1] = *data & 1;
		break;
	default:
		LOG("error: Mux4to1: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Demux1to2::Demux1to2() {
	AddSink("I");       // Input
	AddSink("SEL");     // Select line
	AddSource("O0").SetMultiConn(); // Output 0
	AddSource("O1").SetMultiConn(); // Output 1
	
}

bool Demux1to2::Tick() {
	// Clear all outputs first
	out[0] = 0;
	out[1] = 0;
	
	// Route input to appropriate output based on select line
	if (sel) {
		out[1] = input;
	} else {
		out[0] = input;
	}
	
	return true;
}

bool Demux1to2::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I (Input)
		case 1:  // SEL (Select)
			// Skip inputs - they are handled by PutRaw
			break;
		case 2:  // O0 (Output 0)
			return dest.PutRaw(dest_conn_id, (byte*)&out[0], 0, 1);
		case 3:  // O1 (Output 1)
			return dest.PutRaw(dest_conn_id, (byte*)&out[1], 0, 1);
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Demux1to2: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Demux1to2::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I (Input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		input = *data & 1;
		break;
	case 1: // SEL (Select)
		ASSERT(data_bytes == 0 && data_bits == 1);
		sel = *data & 1;
		break;
	default:
		LOG("error: Demux1to2: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Demux1to4::Demux1to4() {
	AddSink("I");       // Input
	AddSink("S0");      // Select line 0
	AddSink("S1");      // Select line 1
	AddSource("O0").SetMultiConn(); // Output 0
	AddSource("O1").SetMultiConn(); // Output 1
	AddSource("O2").SetMultiConn(); // Output 2
	AddSource("O3").SetMultiConn(); // Output 3
	
}

bool Demux1to4::Tick() {
	// Clear all outputs first
	for (int i = 0; i < 4; i++) {
		out[i] = 0;
	}
	
	// Route input to appropriate output based on both select lines
	// S1 S0 -> output
	//  0  0  -> O0
	//  0  1  -> O1
	//  1  0  -> O2
	//  1  1  -> O3
	int index = (sel[1] ? 2 : 0) + (sel[0] ? 1 : 0);
	out[index] = input;
	
	return true;
}

bool Demux1to4::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I (Input)
		case 1:  // S0 (Select 0)
		case 2:  // S1 (Select 1)
			// Skip inputs - they are handled by PutRaw
			break;
		case 3:  // O0 (Output 0)
			return dest.PutRaw(dest_conn_id, (byte*)&out[0], 0, 1);
		case 4:  // O1 (Output 1)
			return dest.PutRaw(dest_conn_id, (byte*)&out[1], 0, 1);
		case 5:  // O2 (Output 2)
			return dest.PutRaw(dest_conn_id, (byte*)&out[2], 0, 1);
		case 6:  // O3 (Output 3)
			return dest.PutRaw(dest_conn_id, (byte*)&out[3], 0, 1);
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Demux1to4: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Demux1to4::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I (Input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		input = *data & 1;
		break;
	case 1: // S0 (Select 0)
		ASSERT(data_bytes == 0 && data_bits == 1);
		sel[0] = *data & 1;
		break;
	case 2: // S1 (Select 1)
		ASSERT(data_bytes == 0 && data_bits == 1);
		sel[1] = *data & 1;
		break;
	default:
		LOG("error: Demux1to4: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Decoder2to4::Decoder2to4() {
	AddSink("A0");      // Input bit 0
	AddSink("A1");      // Input bit 1
	AddSink("EN");      // Enable (active high)
	AddSource("Y0").SetMultiConn(); // Output 0
	AddSource("Y1").SetMultiConn(); // Output 1
	AddSource("Y2").SetMultiConn(); // Output 2
	AddSource("Y3").SetMultiConn(); // Output 3
	
}

bool Decoder2to4::Tick() {
	// Clear all outputs first
	for (int i = 0; i < 4; i++) {
		out[i] = 0;
	}
	
	// If disabled, all outputs remain low
	if (!en) {
		return true;
	}
	
	// Calculate which output should be high based on the input
	// A1 A0 -> output
	//  0  0  -> Y0
	//  0  1  -> Y1
	//  1  0  -> Y2
	//  1  1  -> Y3
	int index = (in[1] ? 2 : 0) + (in[0] ? 1 : 0);
	out[index] = 1;
	
	return true;
}

bool Decoder2to4::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // A0 (Input bit 0)
		case 1:  // A1 (Input bit 1)
		case 2:  // EN (Enable)
			// Skip inputs - they are handled by PutRaw
			break;
		case 3:  // Y0 (Output 0)
			return dest.PutRaw(dest_conn_id, (byte*)&out[0], 0, 1);
		case 4:  // Y1 (Output 1)
			return dest.PutRaw(dest_conn_id, (byte*)&out[1], 0, 1);
		case 5:  // Y2 (Output 2)
			return dest.PutRaw(dest_conn_id, (byte*)&out[2], 0, 1);
		case 6:  // Y3 (Output 3)
			return dest.PutRaw(dest_conn_id, (byte*)&out[3], 0, 1);
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Decoder2to4: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Decoder2to4::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // A0 (Input bit 0)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[0] = *data & 1;
		break;
	case 1: // A1 (Input bit 1)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[1] = *data & 1;
		break;
	case 2: // EN (Enable)
		ASSERT(data_bytes == 0 && data_bits == 1);
		en = *data & 1;
		break;
	default:
		LOG("error: Decoder2to4: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Decoder3to8::Decoder3to8() {
	AddSink("A0");      // Input bit 0
	AddSink("A1");      // Input bit 1
	AddSink("A2");      // Input bit 2
	AddSink("EN");      // Enable (active high)
	AddSource("Y0").SetMultiConn(); // Output 0
	AddSource("Y1").SetMultiConn(); // Output 1
	AddSource("Y2").SetMultiConn(); // Output 2
	AddSource("Y3").SetMultiConn(); // Output 3
	AddSource("Y4").SetMultiConn(); // Output 4
	AddSource("Y5").SetMultiConn(); // Output 5
	AddSource("Y6").SetMultiConn(); // Output 6
	AddSource("Y7").SetMultiConn(); // Output 7
	
}

bool Decoder3to8::Tick() {
	// Clear all outputs first
	for (int i = 0; i < 8; i++) {
		out[i] = 0;
	}
	
	// If disabled, all outputs remain low
	if (!en) {
		return true;
	}
	
	// Calculate which output should be high based on the input
	// A2 A1 A0 -> output
	//  0  0  0  -> Y0
	//  0  0  1  -> Y1
	//  0  1  0  -> Y2
	//  0  1  1  -> Y3
	//  1  0  0  -> Y4
	//  1  0  1  -> Y5
	//  1  1  0  -> Y6
	//  1  1  1  -> Y7
	int index = (in[2] ? 4 : 0) + (in[1] ? 2 : 0) + (in[0] ? 1 : 0);
	out[index] = 1;
	
	return true;
}

bool Decoder3to8::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // A0 (Input bit 0)
		case 1:  // A1 (Input bit 1)
		case 2:  // A2 (Input bit 2)
		case 3:  // EN (Enable)
			// Skip inputs - they are handled by PutRaw
			break;
		case 4:  // Y0 (Output 0)
			return dest.PutRaw(dest_conn_id, (byte*)&out[0], 0, 1);
		case 5:  // Y1 (Output 1)
			return dest.PutRaw(dest_conn_id, (byte*)&out[1], 0, 1);
		case 6:  // Y2 (Output 2)
			return dest.PutRaw(dest_conn_id, (byte*)&out[2], 0, 1);
		case 7:  // Y3 (Output 3)
			return dest.PutRaw(dest_conn_id, (byte*)&out[3], 0, 1);
		case 8:  // Y4 (Output 4)
			return dest.PutRaw(dest_conn_id, (byte*)&out[4], 0, 1);
		case 9:  // Y5 (Output 5)
			return dest.PutRaw(dest_conn_id, (byte*)&out[5], 0, 1);
		case 10: // Y6 (Output 6)
			return dest.PutRaw(dest_conn_id, (byte*)&out[6], 0, 1);
		case 11: // Y7 (Output 7)
			return dest.PutRaw(dest_conn_id, (byte*)&out[7], 0, 1);
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Decoder3to8: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Decoder3to8::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // A0 (Input bit 0)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[0] = *data & 1;
		break;
	case 1: // A1 (Input bit 1)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[1] = *data & 1;
		break;
	case 2: // A2 (Input bit 2)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[2] = *data & 1;
		break;
	case 3: // EN (Enable)
		ASSERT(data_bytes == 0 && data_bits == 1);
		en = *data & 1;
		break;
	default:
		LOG("error: Decoder3to8: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Encoder4to2::Encoder4to2() {
	AddSink("I0");      // Input 0
	AddSink("I1");      // Input 1
	AddSink("I2");      // Input 2
	AddSink("I3");      // Input 3
	AddSource("A0").SetMultiConn(); // Output bit 0
	AddSource("A1").SetMultiConn(); // Output bit 1
	AddSource("V").SetMultiConn();  // Valid output (indicates at least one input is high)
	
}

bool Encoder4to2::Tick() {
	// Initialize outputs
	out[0] = 0;
	out[1] = 0;
	valid = 0;
	
	// Priority encoder: higher index takes priority
	// I3 > I2 > I1 > I0
	for (int i = 3; i >= 0; i--) {
		if (in[i]) {
			// Convert index to binary (0=00, 1=01, 2=10, 3=11)
			out[0] = i & 1;      // Bit 0
			out[1] = (i >> 1) & 1;  // Bit 1
			valid = 1;
			break;  // First high input wins (priority)
		}
	}
	
	return true;
}

bool Encoder4to2::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I0 (Input 0)
		case 1:  // I1 (Input 1)
		case 2:  // I2 (Input 2)
		case 3:  // I3 (Input 3)
			// Skip inputs - they are handled by PutRaw
			break;
		case 4:  // A0 (Output bit 0)
			return dest.PutRaw(dest_conn_id, (byte*)&out[0], 0, 1);
		case 5:  // A1 (Output bit 1)
			return dest.PutRaw(dest_conn_id, (byte*)&out[1], 0, 1);
		case 6:  // V (Valid output)
			return dest.PutRaw(dest_conn_id, (byte*)&valid, 0, 1);
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Encoder4to2: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Encoder4to2::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I0 (Input 0)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[0] = *data & 1;
		break;
	case 1: // I1 (Input 1)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[1] = *data & 1;
		break;
	case 2: // I2 (Input 2)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[2] = *data & 1;
		break;
	case 3: // I3 (Input 3)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[3] = *data & 1;
		break;
	default:
		LOG("error: Encoder4to2: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

Encoder8to3::Encoder8to3() {
	AddSink("I0");      // Input 0
	AddSink("I1");      // Input 1
	AddSink("I2");      // Input 2
	AddSink("I3");      // Input 3
	AddSink("I4");      // Input 4
	AddSink("I5");      // Input 5
	AddSink("I6");      // Input 6
	AddSink("I7");      // Input 7
	AddSource("A0").SetMultiConn(); // Output bit 0
	AddSource("A1").SetMultiConn(); // Output bit 1
	AddSource("A2").SetMultiConn(); // Output bit 2
	AddSource("V").SetMultiConn();  // Valid output (indicates at least one input is high)
	
}

bool Encoder8to3::Tick() {
	// Initialize outputs
	out[0] = 0;
	out[1] = 0;
	out[2] = 0;
	valid = 0;
	
	// Priority encoder: higher index takes priority
	// I7 > I6 > ... > I1 > I0
	for (int i = 7; i >= 0; i--) {
		if (in[i]) {
			// Convert index to 3-bit binary
			out[0] = i & 1;        // Bit 0
			out[1] = (i >> 1) & 1; // Bit 1
			out[2] = (i >> 2) & 1; // Bit 2
			valid = 1;
			break;  // First high input wins (priority)
		}
	}
	
	return true;
}

bool Encoder8to3::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:   // I0 (Input 0)
		case 1:   // I1 (Input 1)
		case 2:   // I2 (Input 2)
		case 3:   // I3 (Input 3)
		case 4:   // I4 (Input 4)
		case 5:   // I5 (Input 5)
		case 6:   // I6 (Input 6)
		case 7:   // I7 (Input 7)
			// Skip inputs - they are handled by PutRaw
			break;
		case 8:   // A0 (Output bit 0)
			return dest.PutRaw(dest_conn_id, (byte*)&out[0], 0, 1);
		case 9:   // A1 (Output bit 1)
			return dest.PutRaw(dest_conn_id, (byte*)&out[1], 0, 1);
		case 10:  // A2 (Output bit 2)
			return dest.PutRaw(dest_conn_id, (byte*)&out[2], 0, 1);
		case 11:  // V (Valid output)
			return dest.PutRaw(dest_conn_id, (byte*)&valid, 0, 1);
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Encoder8to3: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Encoder8to3::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0:  // I0 (Input 0)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[0] = *data & 1;
		break;
	case 1:  // I1 (Input 1)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[1] = *data & 1;
		break;
	case 2:  // I2 (Input 2)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[2] = *data & 1;
		break;
	case 3:  // I3 (Input 3)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[3] = *data & 1;
		break;
	case 4:  // I4 (Input 4)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[4] = *data & 1;
		break;
	case 5:  // I5 (Input 5)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[5] = *data & 1;
		break;
	case 6:  // I6 (Input 6)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[6] = *data & 1;
		break;
	case 7:  // I7 (Input 7)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in[7] = *data & 1;
		break;
	default:
		LOG("error: Encoder8to3: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}

ElcCapacitor::ElcCapacitor() {
	AddSink("I");
	AddSource("O").SetMultiConn();
	
}



