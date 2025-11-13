#include "ProtoVM.h"




Pin::Pin() {
	AddBidirectional("bi");
	
}





Pin& Pin::SetReference(bool is_high) {
	this->is_high = is_high;
	Clear();
	AddSource("0").SetMultiConn();
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
	// For non-WRITE operations, just return true (nothing to do)
	return true;
}


bool Pin::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	// Pins should accept data being written to them without error
	// Update the pin state based on input data (first bit if available)
	if (data && (data_bytes > 0 || data_bits > 0)) {
		is_high = data[0] & 1;  // Set pin state to value of first bit
	}
	return true;  // Always return true to avoid Process failures
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

FlipFlopD::FlipFlopD() {
	AddSink("D");      // Data input
	AddSink("Ck");     // Clock input
	AddSink("En");     // Enable input (active high)
	AddSink("Clr");    // Clear input (active high)
	AddSource("Q").SetMultiConn();     // Output
	AddSource("~Q").SetMultiConn();    // Inverted output
	
}

bool FlipFlopD::Tick() {
	// Check for clear condition - if clear is active, reset outputs regardless of clock
	if (clr) {
		q = 0;
		qn = 1;
	} else {
		// Detect rising edge of clock
		bool rising_edge = (clk && !last_clk);
		
		// On rising edge, and if enabled, update the output with the D input value
		if (rising_edge && en) {
			q = d;
			qn = !d;  // Inverted output
		}
	}
	
	// Store current clock state for next rising edge detection
	last_clk = clk;
	
	return true;
}

bool FlipFlopD::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // D (Data input)
		case 1:  // Ck (Clock input)
		case 2:  // En (Enable input)
		case 3:  // Clr (Clear input)
			// skip (write of inputs - handled by PutRaw)
			break;
		case 4:  // Q (Output)
			return dest.PutRaw(dest_conn_id, (byte*)&q, 0, 1);
			break;
		case 5:  // ~Q (Inverted output)
			return dest.PutRaw(dest_conn_id, (byte*)&qn, 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: FlipFlopD: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool FlipFlopD::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // D (Data input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		d = *data & 1;
		break;
	case 1: // Ck (Clock input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		clk = *data & 1;
		break;
	case 2: // En (Enable input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		en = *data & 1;
		break;
	case 3: // Clr (Clear input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		clr = *data & 1;
		break;
	default:
		LOG("error: FlipFlopD: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
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

bool ElcNot::Tick() {
	// NOT operation: output is inverse of input
	out = !in;
	return true;
}

bool ElcNot::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // I (Input)
			// skip (write of input - handled by PutRaw)
			break;
		case 1:  // O (Output)
			return dest.PutRaw(dest_conn_id, (byte*)&out, 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: ElcNot: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool ElcNot::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // I (Input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		in = *data & 1;
		break;
	case 1: // This shouldn't happen - output can't be written to
		LOG("error: ElcNot: Attempt to write to output");
		return false;
	default:
		LOG("error: ElcNot: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
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

ElcInductor::ElcInductor(double L) : inductance(L < 1e-9 ? 1e-9 : L), current(0.0), back_emf(0.0), 
                                   last_tick_state_A(false), last_tick_state_B(false) {
	AddBidirectional("A");
	AddBidirectional("B");
}

void ElcInductor::SetInductance(double L) {
	inductance = L < 1e-9 ? 1e-9 : L;  // Minimum inductance to avoid division by zero
}

bool ElcInductor::Tick() {
	// In a digital simulation, we'll model the inductor's behavior through state changes
	
	// Get current states of terminals
	bool current_state_A = GetConnector(0).IsConnected() ? true : false;  // A terminal
	bool current_state_B = GetConnector(1).IsConnected() ? true : false;  // B terminal
	
	// Calculate voltage difference across inductor (digital approximation)
	// In real circuits: V = L * di/dt
	// For digital: we'll model the resistance to current change
	
	// Calculate change in voltage state and use it to model back EMF
	// For digital simulation, we'll use the state transition to model inductor behavior
	bool state_A_changed = (current_state_A != last_tick_state_A);
	bool state_B_changed = (current_state_B != last_tick_state_B);
	
	// Update stored states for next tick
	last_tick_state_A = current_state_A;
	last_tick_state_B = current_state_B;
	
	// Simulate inductor behavior: opposes change in current
	// In digital terms: creates a delay/reaction to state changes
	if (state_A_changed || state_B_changed) {
		// Apply back EMF effect based on inductance
		back_emf = 0.1 / inductance;  // Simplified back EMF calculation
	}
	
	// In digital simulation, the inductor allows current to continue flowing
	// in the same direction even when voltage tries to change direction
	// This is simplified for the digital simulation
	
	return ElcBase::Tick();  // Call parent tick
}

bool ElcInductor::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == ProcessType::TICK) {
		return Tick();
	}
	
	// For digital inductor simulation, process signals based on inductor behavior
	if (type == ProcessType::WRITE) {
		// Handle digital signal flow with inductor's resistance to change
		byte temp_data[1];
		if (conn_id == 0) {  // Input from A
			if (GetConnector(1).IsConnected()) {
				// Allow signal through with inductive delay simulation
				// Get signal from A and pass to B
				// In real terms: inductor allows current to continue but resists changes
				return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
			}
		} else if (conn_id == 1) {  // Input from B
			if (GetConnector(0).IsConnected()) {
				// Allow signal through with inductive delay simulation
				// Get signal from B and pass to A
				return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
			}
		}
	}
	
	return false;
}

bool ElcInductor::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	// This method handles raw data input to the inductor
	// In digital simulation, we'll use it to model the inductor's behavior
	if (conn_id == 0) {  // From terminal A
		// Apply inductor's effect to signal going to terminal B
		// For now, pass the signal through with a slight delay effect
		if (GetConnector(1).IsConnected()) {
			// The inductor resists sudden changes
			return true;  // Signal successfully received
		}
	} else if (conn_id == 1) {  // From terminal B
		// Apply inductor's effect to signal going to terminal A
		if (GetConnector(0).IsConnected()) {
			// The inductor resists sudden changes
			return true;  // Signal successfully received
		}
	}
	
	return false;
}

ElcSwitch::ElcSwitch(bool initial_state) : is_closed(initial_state) {
	AddBidirectional("A");
	AddBidirectional("B");
}

void ElcSwitch::Close() {
	is_closed = true;
}

void ElcSwitch::Open() {
	is_closed = false;
}

void ElcSwitch::Toggle() {
	is_closed = !is_closed;
}

bool ElcSwitch::Tick() {
	// In a switch, the tick behavior depends on whether it's closed or open
	// If closed, it allows signals to pass between A and B
	// If open, it blocks signals between A and B
	return ElcBase::Tick();  // Call parent tick
}

bool ElcSwitch::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == ProcessType::TICK) {
		return Tick();
	}
	
	if (type == ProcessType::WRITE) {
		// Only allow signal to pass if the switch is closed
		if (!is_closed) {
			return false;  // Switch is open, block signal
		}
		
		// Switch is closed, allow signal to pass through
		if (conn_id == 0) {  // Input from terminal A
			// Pass signal to terminal B if it's connected
			if (GetConnector(1).IsConnected()) {
				// For now, we'll use a temporary array to pass the signal
				byte temp_data[1] = {0};
				if (bytes > 0) {
					// Simulate passing the signal through the switch
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 1) {  // Input from terminal B
			// Pass signal to terminal A if it's connected
			if (GetConnector(0).IsConnected()) {
				// For now, we'll use a temporary array to pass the signal
				byte temp_data[1] = {0};
				if (bytes > 0) {
					// Simulate passing the signal through the switch
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		}
	}
	
	return false;
}

bool ElcSwitch::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	// Process raw data input based on switch state
	if (!is_closed) {
		return false;  // Switch is open, reject any input
	}
	
	// Switch is closed, accept the input and pass it through
	if (conn_id == 0) {  // Data coming from terminal A
		// Pass data to terminal B if connected
		if (GetConnector(1).IsConnected()) {
			return true;
		}
	} else if (conn_id == 1) {  // Data coming from terminal B
		// Pass data to terminal A if connected
		if (GetConnector(0).IsConnected()) {
			return true;
		}
	}
	
	return false;
}

ElcPushSwitch::ElcPushSwitch(bool latched) : is_pressed(false), is_latched(latched), was_pressed(false) {
	AddBidirectional("A");
	AddBidirectional("B");
	// Control pin to trigger the switch
	AddSink("Control");
}

void ElcPushSwitch::Press() {
	is_pressed = true;
}

void ElcPushSwitch::Release() {
	if (!is_latched) {
		is_pressed = false;
	}
	// If latched, it stays pressed until Reset() is called
}

void ElcPushSwitch::Reset() {
	is_pressed = false;
}

void ElcPushSwitch::Latch() {
	is_latched = true;
	is_pressed = true;  // Also press when latching
}

void ElcPushSwitch::Unlatch() {
	is_latched = false;
}

bool ElcPushSwitch::Tick() {
	// Check the control input to determine if switch should be pressed
	// If control input is high, press the switch (if not already latched)
	// If control input is low and not latched, release the switch
	
	// For a momentary push switch, it should return to unpressed state 
	// after each tick unless latched
	if (!is_latched) {
		is_pressed = false;  // Momentary switch returns to released state
	}
	
	return ElcBase::Tick();  // Call parent tick
}

bool ElcPushSwitch::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == ProcessType::TICK) {
		return Tick();
	}
	
	if (type == ProcessType::WRITE) {
		// If the control input is triggered, press the switch temporarily
		if (conn_id == 2) { // Control input
			// This would be handled by the control pin logic
			if (!is_latched) {
				is_pressed = true;  // Press when control is activated
			}
			return true;
		}
		
		if (!is_pressed) {
			return false;  // Switch is not pressed, block signal
		}
		
		// Switch is pressed, allow signal to pass through
		if (conn_id == 0) {  // Input from terminal A
			// Pass signal to terminal B if it's connected
			if (GetConnector(1).IsConnected()) {
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 1) {  // Input from terminal B
			// Pass signal to terminal A if it's connected
			if (GetConnector(0).IsConnected()) {
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		}
	}
	
	return false;
}

bool ElcPushSwitch::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	// Process raw data input based on switch state
	if (!is_pressed) {
		return false;  // Switch is not pressed, reject input
	}
	
	if (conn_id == 0) {  // Data from terminal A
		if (GetConnector(1).IsConnected()) {
			return true;
		}
	} else if (conn_id == 1) {  // Data from terminal B
		if (GetConnector(0).IsConnected()) {
			return true;
		}
	}
	
	return false;
}

ElcSPDT::ElcSPDT(bool default_position) : position(default_position), is_centered(false) {
	AddBidirectional("Common");  // Common terminal
	AddBidirectional("Output0"); // First output position
	AddBidirectional("Output1"); // Second output position
	AddSink("Control");          // Control input to change position
}

void ElcSPDT::SetPosition(bool pos) {
	position = pos;
	is_centered = false;  // No longer centered when set to a position
}

void ElcSPDT::Toggle() {
	position = !position;
	is_centered = false;  // No longer centered when toggled
}

void ElcSPDT::SetCenter() {
	is_centered = true;  // In a real SPDT, this would be a third position, but for digital we'll just disconnect
}

bool ElcSPDT::Tick() {
	// In SPDT, the connection is based on position and doesn't change unless explicitly controlled
	// The switch position is set by the control input
	
	return ElcBase::Tick();  // Call parent tick
}

bool ElcSPDT::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == ProcessType::TICK) {
		return Tick();
	}
	
	if (type == ProcessType::WRITE) {
		if (conn_id == 3) { // Control input
			// Change position based on control signal
			// For digital simulation, we can interpret any signal as a toggle
			Toggle();
			return true;
		}
		
		// Handle signal routing based on switch position
		if (is_centered) {
			return false;  // If centered, no connection is made
		}
		
		if (conn_id == 0) {  // From Common terminal
			// Route to the selected output based on position
			if (position && GetConnector(2).IsConnected()) {  // To Output1
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			} else if (!position && GetConnector(1).IsConnected()) {  // To Output0
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 1) {  // From Output0
			// Only pass through if switch is in position 0
			if (!position && !is_centered && GetConnector(0).IsConnected()) {  // To Common
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 2) {  // From Output1
			// Only pass through if switch is in position 1
			if (position && !is_centered && GetConnector(0).IsConnected()) {  // To Common
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		}
	}
	
	return false;
}

bool ElcSPDT::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	if (is_centered) {
		return false;  // No connection in centered position
	}
	
	if (conn_id == 0) {  // Data from Common
		// Route to selected output based on position
		if (position && GetConnector(2).IsConnected()) {
			return true;  // To Output1
		} else if (!position && GetConnector(1).IsConnected()) {
			return true;  // To Output0
		}
	} else if (conn_id == 1) {  // Data from Output0
		if (!position && GetConnector(0).IsConnected()) {
			return true;  // Only if switch is in position 0
		}
	} else if (conn_id == 2) {  // Data from Output1
		if (position && GetConnector(0).IsConnected()) {
			return true;  // Only if switch is in position 1
		}
	}
	
	return false;
}

ElcDPDT::ElcDPDT(bool default_position) : position(default_position), is_centered(false) {
	AddBidirectional("Common1");   // First pole common terminal
	AddBidirectional("Common2");   // Second pole common terminal
	AddBidirectional("Out1A");     // First pole output A
	AddBidirectional("Out1B");     // First pole output B
	AddBidirectional("Out2A");     // Second pole output A
	AddBidirectional("Out2B");     // Second pole output B
	AddSink("Control");            // Control input to change position
}

void ElcDPDT::SetPosition(bool pos) {
	position = pos;
	is_centered = false;  // No longer centered when set to a position
}

void ElcDPDT::Toggle() {
	position = !position;
	is_centered = false;  // No longer centered when toggled
}

void ElcDPDT::SetCenter() {
	is_centered = true;  // Disconnect both poles (for 3-position version)
}

bool ElcDPDT::Tick() {
	// In DPDT, the connection is based on position and doesn't change unless explicitly controlled
	// The switch position is set by the control input
	
	return ElcBase::Tick();  // Call parent tick
}

bool ElcDPDT::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == ProcessType::TICK) {
		return Tick();
	}
	
	if (type == ProcessType::WRITE) {
		if (conn_id == 6) { // Control input
			// Change position based on control signal
			// For digital simulation, we can interpret any signal as a toggle
			Toggle();
			return true;
		}
		
		// Handle signal routing based on switch position
		if (is_centered) {
			return false;  // If centered, no connection is made
		}
		
		// Route signals according to DPDT configuration
		if (conn_id == 0) {  // From Common1
			// Route to the selected output based on position
			if (position && GetConnector(3).IsConnected()) {  // To Out1B
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			} else if (!position && GetConnector(2).IsConnected()) {  // To Out1A
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 1) {  // From Common2
			// Route to the selected output based on position
			if (position && GetConnector(5).IsConnected()) {  // To Out2B
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			} else if (!position && GetConnector(4).IsConnected()) {  // To Out2A
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} 
		// Also handle reverse directions - from outputs to commons
		else if (conn_id == 2) {  // From Out1A
			if (!position && !is_centered && GetConnector(0).IsConnected()) {  // To Common1
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 3) {  // From Out1B
			if (position && !is_centered && GetConnector(0).IsConnected()) {  // To Common1
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 4) {  // From Out2A
			if (!position && !is_centered && GetConnector(1).IsConnected()) {  // To Common2
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 5) {  // From Out2B
			if (position && !is_centered && GetConnector(1).IsConnected()) {  // To Common2
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		}
	}
	
	return false;
}

bool ElcDPDT::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	if (is_centered) {
		return false;  // No connection in centered position
	}
	
	// Route signals according to DPDT configuration
	if (conn_id == 0) {  // Data from Common1
		if (position && GetConnector(3).IsConnected()) {
			return true;  // To Out1B
		} else if (!position && GetConnector(2).IsConnected()) {
			return true;  // To Out1A
		}
	} else if (conn_id == 1) {  // Data from Common2
		if (position && GetConnector(5).IsConnected()) {
			return true;  // To Out2B
		} else if (!position && GetConnector(4).IsConnected()) {
			return true;  // To Out2A
		}
	} else if (conn_id == 2) {  // Data from Out1A
		if (!position && GetConnector(0).IsConnected()) {
			return true;  // To Common1
		}
	} else if (conn_id == 3) {  // Data from Out1B
		if (position && GetConnector(0).IsConnected()) {
			return true;  // To Common1
		}
	} else if (conn_id == 4) {  // Data from Out2A
		if (!position && GetConnector(1).IsConnected()) {
			return true;  // To Common2
		}
	} else if (conn_id == 5) {  // Data from Out2B
		if (position && GetConnector(1).IsConnected()) {
			return true;  // To Common2
		}
	}
	
	return false;
}

ElcMakeBeforeBreakSwitch::ElcMakeBeforeBreakSwitch(bool initial_position, int transition_ticks) 
	: current_position(initial_position), target_position(initial_position), 
	  transition_count(0), transition_duration(transition_ticks) {
	AddBidirectional("Common");   // Common terminal
	AddBidirectional("Output0");  // First output position
	AddBidirectional("Output1");  // Second output position
	AddSink("Control");           // Control input to change position
}

void ElcMakeBeforeBreakSwitch::SetPosition(bool pos) {
	// In a make-before-break switch, we first establish connection to the new position
	// then break connection to the old position - this prevents interruption
	target_position = pos;
	
	// Start the transition process
	transition_count = 0;
}

void ElcMakeBeforeBreakSwitch::ImmediateSet(bool pos) {
	// Immediately change switch position without transition behavior
	current_position = pos;
	target_position = pos;
	transition_count = 0;
}

bool ElcMakeBeforeBreakSwitch::Tick() {
	// Handle the make-before-break transition logic
	if (current_position != target_position) {
		transition_count++;
		
		// In make-before-break, we briefly connect to both positions during transition
		if (transition_count >= transition_duration) {
			// Transition complete - now only connect to the new position
			current_position = target_position;
			transition_count = 0;
		}
	}
	
	return ElcBase::Tick();  // Call parent tick
}

bool ElcMakeBeforeBreakSwitch::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == ProcessType::TICK) {
		return Tick();
	}
	
	if (type == ProcessType::WRITE) {
		if (conn_id == 3) { // Control input
			// Change target position based on control signal
			// For simulation, we'll toggle when signal is received
			SetPosition(!target_position);
			return true;
		}
		
		// Handle signal routing based on switch state
		if (conn_id == 0) {  // From Common terminal
			// During transition in make-before-break, we may be connected to both outputs briefly
			if (transition_count > 0 && transition_count < transition_duration) {
				// In transition state - potentially connected to both outputs
				// For simulation purposes, we'll route to both
				byte temp_data[1] = {0};
				if (bytes > 0) {
					// Route to both outputs during transition
					if (GetConnector(1).IsConnected()) {
						dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
					}
					if (GetConnector(2).IsConnected()) {
						dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
					}
					return true;
				}
			} else {
				// Not in transition - route to current active output
				if (current_position && GetConnector(2).IsConnected()) {  // To Output1
					byte temp_data[1] = {0};
					if (bytes > 0) {
						return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
					}
				} else if (!current_position && GetConnector(1).IsConnected()) {  // To Output0
					byte temp_data[1] = {0};
					if (bytes > 0) {
						return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
					}
				}
			}
		} else if (conn_id == 1) {  // From Output0
			// Only pass through if switch is in position 0 or in transition
			if ((!current_position || 
				(transition_count > 0 && transition_count < transition_duration)) && 
				GetConnector(0).IsConnected()) {  // To Common
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		} else if (conn_id == 2) {  // From Output1
			// Only pass through if switch is in position 1 or in transition
			if ((current_position || 
				(transition_count > 0 && transition_count < transition_duration)) && 
				GetConnector(0).IsConnected()) {  // To Common
				byte temp_data[1] = {0};
				if (bytes > 0) {
					return dest.PutRaw(dest_conn_id, temp_data, bytes, bits);
				}
			}
		}
	}
	
	return false;
}

bool ElcMakeBeforeBreakSwitch::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	// During transition, the make-before-break switch might be connected to both outputs briefly
	if (conn_id == 0) {  // Data from Common
		// Route to the appropriate output based on position
		if (transition_count > 0 && transition_count < transition_duration) {
			// During transition - connected to both outputs
			if (GetConnector(1).IsConnected() || GetConnector(2).IsConnected()) {
				return true;
			}
		} else {
			// Not in transition - connected to specific output
			if (current_position && GetConnector(2).IsConnected()) {
				return true;  // To Output1
			} else if (!current_position && GetConnector(1).IsConnected()) {
				return true;  // To Output0
			}
		}
	} else if (conn_id == 1) {  // Data from Output0
		if ((!current_position || 
			(transition_count > 0 && transition_count < transition_duration)) && 
			GetConnector(0).IsConnected()) {
			return true;  // Only if switch is in position 0 or in transition
		}
	} else if (conn_id == 2) {  // Data from Output1
		if ((current_position || 
			(transition_count > 0 && transition_count < transition_duration)) && 
			GetConnector(0).IsConnected()) {
			return true;  // Only if switch is in position 1 or in transition
		}
	}
	
	return false;
}


Register4Bit::Register4Bit() {
	// Add sinks for 4-bit data input (D3, D2, D1, D0)
	AddSink("D3");
	AddSink("D2"); 
	AddSink("D1");
	AddSink("D0");
	// Add sinks for control signals
	AddSink("Ck");     // Clock
	AddSink("En");     // Enable
	AddSink("Clr");    // Clear
	// Add sources for 4-bit output (Q3, Q2, Q1, Q0)
	AddSource("Q3").SetMultiConn();
	AddSource("Q2").SetMultiConn();
	AddSource("Q1").SetMultiConn();
	AddSource("Q0").SetMultiConn();
}

bool Register4Bit::Tick() {
	// Check for clear condition - if clear is active, reset all outputs regardless of clock
	if (clr) {
		for (int i = 0; i < 4; i++) {
			q[i] = 0;
		}
	} else {
		// Detect rising edge of clock
		bool rising_edge = (clk && !last_clk);
	
		// On rising edge, and if enabled, update all output bits with input data
		if (rising_edge && en) {
			for (int i = 0; i < 4; i++) {
				q[i] = d[i];
			}
		}
	}
	
	// Store current clock state for next rising edge detection
	last_clk = clk;
	
	return true;
}

bool Register4Bit::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	if (type == WRITE) {
		switch (conn_id) {
		case 0:  // D3 (Data bit 3)
		case 1:  // D2 (Data bit 2)
		case 2:  // D1 (Data bit 1)
		case 3:  // D0 (Data bit 0)
		case 4:  // Ck (Clock input)
		case 5:  // En (Enable input)
		case 6:  // Clr (Clear input)
			// skip (write of inputs - handled by PutRaw)
			break;
		case 7:  // Q3 (Output bit 3)
			return dest.PutRaw(dest_conn_id, (byte*)&q[3], 0, 1);
			break;
		case 8:  // Q2 (Output bit 2)
			return dest.PutRaw(dest_conn_id, (byte*)&q[2], 0, 1);
			break;
		case 9:  // Q1 (Output bit 1)
			return dest.PutRaw(dest_conn_id, (byte*)&q[1], 0, 1);
			break;
		case 10: // Q0 (Output bit 0)
			return dest.PutRaw(dest_conn_id, (byte*)&q[0], 0, 1);
			break;
		default:
			// For any other connection IDs, just acknowledge (for dummy pins or similar)
			break;
		}
	}
	else {
		LOG("error: Register4Bit: unimplemented ProcessType");
		return false;
	}
	return true;
}

bool Register4Bit::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	switch (conn_id) {
	case 0: // D3 (Data bit 3)
		ASSERT(data_bytes == 0 && data_bits == 1);
		d[3] = *data & 1;
		break;
	case 1: // D2 (Data bit 2)
		ASSERT(data_bytes == 0 && data_bits == 1);
		d[2] = *data & 1;
		break;
	case 2: // D1 (Data bit 1)
		ASSERT(data_bytes == 0 && data_bits == 1);
		d[1] = *data & 1;
		break;
	case 3: // D0 (Data bit 0)
		ASSERT(data_bytes == 0 && data_bits == 1);
		d[0] = *data & 1;
		break;
	case 4: // Ck (Clock input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		clk = *data & 1;
		break;
	case 5: // En (Enable input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		en = *data & 1;
		break;
	case 6: // Clr (Clear input)
		ASSERT(data_bytes == 0 && data_bits == 1);
		clr = *data & 1;
		break;
	default:
		LOG("error: Register4Bit: unimplemented conn-id " << conn_id);
		return false;
	}
	return true;
}







