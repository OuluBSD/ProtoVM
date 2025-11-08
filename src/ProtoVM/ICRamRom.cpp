#include "ProtoVM.h"




ICMem8Base::ICMem8Base(byte* data, int size, bool writable) {
	this->data = data;
	this->size = size;
	this->writable = writable;
	
	memset(data, 0, size);
	
	// NOTE: incorrect order for any real package
	
	AddSink("A0"); // 0
	AddSink("A1");
	AddSink("A2");
	AddSink("A3");
	AddSink("A4");
	AddSink("A5");
	AddSink("A6");
	AddSink("A7");
	AddSink("A8");
	AddSink("A9");
	AddSink("A10");
	AddSink("A11");
	AddSink("A12");
	AddSink("A13");
	AddSink("A14");
	AddSink("A15");
	AddBidirectional("D0"); // 16
	AddBidirectional("D1");
	AddBidirectional("D2");
	AddBidirectional("D3");
	AddBidirectional("D4");
	AddBidirectional("D5");
	AddBidirectional("D6");
	AddBidirectional("D7");
	AddSink("~OE"); // 24
	AddSink("~CS");
	
	if (writable)
		AddSink("~WR");
	
}

bool ICMem8Base::Tick() {
	// Store old values to detect changes
	uint16_t old_addr = addr;
	uint8_t old_reading = reading;
	uint8_t old_enabled = enabled;
	uint8_t old_writing = writing;
	uint8_t old_data_at_addr = (addr < size) ? data[addr] : 0;
	
	bool verbose = 1;
	addr = in_addr;
	writing = in_writing;
	reading = in_reading;
	enabled = in_enabled;
	
	if (writing && addr < size) {
		data[addr] = in_data;
	}
	
	if (verbose) {
		LOG("ICMem8Base::Tick: r=" << (int)in_reading << ", w=" << (int)in_writing << ", addr=" << HexStr(in_addr) << ", data=" << HexStr(in_data));
	}
	
	// Detect if any important state changed
	bool state_changed = (addr != old_addr) || 
	                     (reading != old_reading) || 
	                     (enabled != old_enabled) ||
	                     (writing != old_writing) ||
	                     (writing && addr < size && data[addr] != old_data_at_addr);  // Write operation changes memory state
	
	// Update change status
	SetChanged(state_changed);
	
	in_data = 0;
	in_addr = 0;
	return true;
}

bool ICMem8Base::Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) {
	union {
		byte tmp[2];
		uint16 tmp16;
	};
	/*if (type == READ) {
		if (writable) {
			ProcessType dest_type = ProcessType::INVALID;
			switch(type) {
				case READ:      dest_type = WRITE;  break;
				case RW:        dest_type = WRITE; break;
				default: break;
			}
			processing = true;
			bool ret = dest.Process(dest_type, bytes, bits, dest_conn_id, *this, conn_id);
			processing = false;
			return ret;
		}
	}*/
	/*if (type == BYTE_WRITE) {
		switch (conn_id) {
		case 0:
			if (reading && enabled) {
				tmp[0] = this->addr < size ? this->data[this->addr] : 0;
				return dest.PutRaw(dest_conn_id, &tmp[0], 1);
			}
			break;
		case 8:
			break;
		default:
			LOG("error: ICMem8Base: unimplemented conn-id");
			return false;
		}
	}*/
	if (type == WRITE) {
		switch (conn_id) {
		case A0:
		case OE:
		case CS:
		case WR:
			// skip (write of control signals)
			break;
		case D0:  // Data bus - only drive when reading from memory
			if (reading && enabled && !writing) {  // When reading and not writing, drive the data bus
				tmp[0] = this->addr < size ? this->data[this->addr] : 0;
				return dest.PutRaw(dest_conn_id, &tmp[0], 1, 0);
			}
			// When not reading or disabled, don't drive the bus
			break;
		case D0+1:
		case D0+2:
		case D0+3:
		case D0+4:
		case D0+5:
		case D0+6:
		case D0+7:
			// Handle individual data pins similarly
			if (reading && enabled && !writing) {
				int bit_pos = conn_id - D0;
				tmp[0] = ((this->addr < size ? this->data[this->addr] : 0) >> bit_pos) & 1;
				return dest.PutRaw(dest_conn_id, &tmp[0], 0, 1);  // Send 1 bit
			}
			break;
		default:
			// Ignore writes to unhandled connection IDs - this can happen when
			// other components try to write to pins that aren't directly handled
			// by this memory component
			break;
		}
	}
	else {
		// For non-WRITE operations, just return true (nothing to do)
		// Process operations from Machine are only WRITE, so this should
		// only happen if other component types send non-WRITE operations
		return true;
	}
	return true;
}

bool ICMem8Base::PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) {
	int off;
	uint16 mask;
	uint16 data16;
	
	switch (conn_id) {
		#include "D8A16.inl"
	case OE: // ~OE
		in_reading = !*data;
		break;
	case CS: // ~CS
		in_enabled = !*data;
		break;
	case WR:
		ASSERT(writable);
		ASSERT(data_bytes == 0 && data_bits == 1);
		in_writing = !*data;
		break;
		
	default:
		// Ignore writes to unhandled connection IDs - this can happen when
		// other components attempt to drive pins that aren't directly handled
		// by this memory component
		return true;
	}
	return true;
}






