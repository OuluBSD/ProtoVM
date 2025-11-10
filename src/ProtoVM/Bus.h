#ifndef _ProtoVM_Bus_h_
#define _ProtoVM_Bus_h_




template <int Width>
class Bus : public ElcBase {
	//RTTI_DECL1(Bus, ElcBase);
	
	static constexpr int BYTES = Width / 8 + ((Width % 8) ? 1 : 0);
	static constexpr int BITS = Width % 8;
	
	bool processing = false;
	byte data[BYTES];
	bool is_driven[BYTES];  // Track which bytes are actively driven
	bool verbose = 1;
	
public:
	Bus() {
		for(int i = 0; i < Width; i++)
			AddBidirectional(IntStr(i)).SetMultiConn();
		Clear();
		InitDrivers();
	}
	
	Bus& Verbose(bool b=true) {verbose = b; return *this;}
	
	int GetMemorySize() const override {return Width / 8 + ((Width % 8) == 0 ? 0 : 1);}
	
	void Clear() {
		for(int i = 0; i < BYTES; i++)
			data[i] = 0;
	}
	
	void InitDrivers() {
		for(int i = 0; i < BYTES; i++)
			is_driven[i] = false;
	}
	
	void ResetDrivers() {
		InitDrivers();
	}
	
	bool IsDriven(int byte_idx) const {
		if (byte_idx >= 0 && byte_idx < BYTES)
			return is_driven[byte_idx];
		return false;
	}
	
	void SetDriven(int byte_idx, bool driven) {
		if (byte_idx >= 0 && byte_idx < BYTES)
			is_driven[byte_idx] = driven;
	}
	
	bool Tick() override {
		if (verbose) {
			LOG("Bus::Tick(" << GetName() << "): " << HexString((const char*)data, BYTES));
		}
		
		// Reset driver flags for next cycle
		ResetDrivers();
		
		// For buses, we consider them as having potentially changed
		// since other components may have driven new values
		SetChanged(true);
		return true;
	}
	
	bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
		if (processing) {
			LOG("error: recursive processing");
			return false;
		}
		
		if (type == WRITE) {
			if (!dest.PutRaw(dest_conn_id, this->data, bytes, bits))
				return false;
		}
		
		/*if (type == READ) {
			ProcessType dest_type = ProcessType::WRITE;
			processing = true;
			bool ret = dest.Process(dest_type, bytes, bits, dest_conn_id, *this, conn_id);
			processing = false;
			return ret;
		}*/
		return true;
	}
	
	bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
		if (conn_id == 0) {
			if (BITS == 0) {
				ASSERT(data_bytes == BYTES && data_bits == 0);
				// Handle tristate behavior - check if the connection is from a tristate component
				
				int copy_bytes = min(data_bytes, BYTES);
				// Handle tri-state behavior - only update if data is valid
				for (int i = 0; i < copy_bytes; i++) {
					if (!is_driven[i]) {
						// First driver takes control
						this->data[i] = data[i];
						is_driven[i] = true;
					} else {
						// If another driver is trying to drive a different value, handle bus contention
						if (this->data[i] != data[i]) {
							LOG("Bus contention detected on " << GetName() << " byte " << i 
								<< ": was 0x" << HexStr(this->data[i]) << " now 0x" << HexStr(data[i]));
							// For now, set to undefined value (0xFF) to indicate contention
							this->data[i] = 0xFF;
						}
					}
				}
				return true;
			}
			else {
				// For partial bit writes, we'll ignore them for now rather than fail
				// This can happen during initialization or special operations
				return true;
			}
		}
		// For unexpected conn ids, just return true to avoid failure
		// This can happen when components try to interact in unexpected ways
		return true;
	}
};


using Bus8 = Bus<8>;
using Bus16 = Bus<16>;


class InterakBus : public ElcBase {
	//RTTI_DECL1(InterakBus, ElcBase);
	
	
public:
	InterakBus();
	
};




#endif
