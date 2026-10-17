#include "CaptureModel.h"

#include <algorithm>

CaptureModel::CaptureModel() {
}

void CaptureModel::Clear() {
	tracks.Clear();
	uartConfigs.Clear();
	timeStart = 0;
	timeEnd = 1000000;
}

void CaptureModel::GenerateDemoCapture(int numChannels, int64 durationUs) {
	Clear();
	timeEnd = durationUs;
	
	for(int i = 0; i < numChannels; i++) {
		Track& t = tracks.Add();
		t.name = Format("CH%d", i);
		t.kind = TK_RAW;
		t.color = Color((i * 37 + 100) % 255, (i * 73 + 50) % 255, (i * 11 + 150) % 255);
		t.visible = true;
		
		int64 period = durationUs / (4 + i * 2);
		int64 halfPeriod = period / 2;
		
		for(int64 time = 0; time < durationUs; time += halfPeriod) {
			bool val = (time / halfPeriod) % 2 == 0;
			t.samples.Add(Sample{time, val});
			if(time > 0) {
				t.transitions.Add(Transition{time, val});
			}
		}
		t.transitions.Add(Transition{durationUs, !t.samples.Top().value});
	}
	
	UartConfig uart;
	uart.name = "UART1";
	uart.color = LtBlue();
	uart.rxChannel = 0;
	uart.txChannel = 1;
	uartConfigs.Add(uart);
	
	Track& rxTrack = tracks.Add();
	rxTrack.name = "UART RX";
	rxTrack.kind = TK_UART_RX;
	rxTrack.color = LtBlue();
	rxTrack.uartChannel = 0;
	GenerateUartSignal(rxTrack, uart, "Hello World!\r\n");
	
	Track& txTrack = tracks.Add();
	txTrack.name = "UART TX";
	txTrack.kind = TK_UART_TX;
	txTrack.color = LtGreen();
	txTrack.uartChannel = 0;
	GenerateUartSignal(txTrack, uart, "OK\r\n");
	
	AddCustomTrack("PC: Bits", TK_CUSTOM_BITS);
	AddCustomTrack("PC: Address/Data", TK_CUSTOM_ADDR_DATA);
}

void CaptureModel::AddCustomTrack(const String& name, TrackKind kind) {
	Track& t = tracks.Add();
	t.name = name;
	t.kind = kind;
	t.color = (kind == TK_CUSTOM_BITS) ? Color(255, 165, 0) : Color(138, 43, 226);
	t.visible = true;
	
	if(kind == TK_CUSTOM_BITS) {
		GenerateCustomBits(t);
	} else if(kind == TK_CUSTOM_ADDR_DATA) {
		GenerateCustomAddrData(t);
	}
}

void CaptureModel::RemoveTrack(int index) {
	if(index >= 0 && index < tracks.GetCount()) {
		tracks.Remove(index);
	}
}

void CaptureModel::DecodeUart(int trackIndex, const UartConfig& config) {
	if(trackIndex < 0 || trackIndex >= tracks.GetCount()) return;
	
	Track& track = tracks[trackIndex];
	track.decodedBlocks.Clear();
	
	if(track.kind != TK_UART_RX && track.kind != TK_UART_TX) return;
	
	const Array<Transition>& trans = track.transitions;
	if(trans.GetCount() < 2) return;
	
	int64 bitTime = 1000000 / config.baudRate;
	int64 samplePoint = bitTime / 2;
	
	int64 startTime = trans[0].time;
	bool idleState = !trans[0].value;
	
	for(size_t i = 0; i < trans.GetCount(); ) {
		if(trans[i].value == idleState) {
			int64 nextEdge = (i + 1 < trans.GetCount()) ? trans[i + 1].time : timeEnd;
			if(nextEdge - trans[i].time >= bitTime * 1.5) {
				int64 bitStart = trans[i].time;
				String byteStr;
				int bitsDecoded = 0;
				
				for(int bit = 0; bit < config.dataBits + 1; bit++) {
					int64 sampleTime = bitStart + samplePoint + bit * bitTime;
					if(sampleTime >= timeEnd) break;
					
					bool bitVal = false;
					for(size_t j = 0; j < trans.GetCount(); j++) {
						if(trans[j].time > sampleTime) {
							bitVal = (j > 0) ? trans[j - 1].value : trans[0].value;
							break;
						}
					}
					if(config.bitOrder == "lsb-first") {
						byteStr = (bitVal ? "1" : "0") + byteStr;
					} else {
						byteStr = byteStr + (bitVal ? "1" : "0");
					}
					bitsDecoded++;
				}
				
				if(bitsDecoded > 0) {
					int64 blockEnd = bitStart + (config.dataBits + 2) * bitTime;
					DecodedBlock block;
					block.startTime = bitStart;
					block.endTime = min(blockEnd, timeEnd);
					block.type = DT_UART;
					block.trackIndex = trackIndex;
					
					if(config.dataFormat == "hex") {
						int val = 0;
						for(int b = 0; b < byteStr.GetCount(); b++) {
							if(byteStr[b] == '1') val |= (1 << b);
						}
						block.text = Format("0x%02X", val);
					} else if(config.dataFormat == "bin") {
						block.text = byteStr;
					} else if(config.dataFormat == "dec") {
						int val = 0;
						for(int b = 0; b < byteStr.GetCount(); b++) {
							if(byteStr[b] == '1') val |= (1 << b);
						}
						block.text = AsString(val);
					} else if(config.dataFormat == "oct") {
						int val = 0;
						for(int b = 0; b < byteStr.GetCount(); b++) {
							if(byteStr[b] == '1') val |= (1 << b);
						}
						block.text = Format("0%o", val);
					} else {
						int val = 0;
						for(int b = 0; b < byteStr.GetCount(); b++) {
							if(byteStr[b] == '1') val |= (1 << b);
						}
						block.text = (val >= 32 && val < 127) ? String(val, 1) : Format("\\x%02X", val);
					}
					
					block.color = config.color;
					track.decodedBlocks.Add(block);
				}
				
				i += 2;
			} else {
				i++;
			}
		} else {
			i++;
		}
	}
}

void CaptureModel::DecodeAllUart() {
	for(int i = 0; i < tracks.GetCount(); i++) {
		if(tracks[i].kind == TK_UART_RX || tracks[i].kind == TK_UART_TX) {
			if(!uartConfigs.IsEmpty()) {
				DecodeUart(i, uartConfigs[0]);
			}
		}
	}
}

void CaptureModel::GenerateUartSignal(Track& track, const UartConfig& config, const String& data) {
	int64 bitTime = 1000000 / config.baudRate;
	int64 time = 0;
	
	bool idle = true;
	if(config.invertRx && track.kind == TK_UART_RX) idle = false;
	if(config.invertTx && track.kind == TK_UART_TX) idle = false;
	
	track.samples.Add(Sample{0, idle});
	
	for(int ci = 0; ci < data.GetCount(); ci++) {
		unsigned char c = data[ci];
		
		track.transitions.Add(Transition{time, !idle});
		track.samples.Add(Sample{time, !idle});
		time += bitTime;
		
		for(int bit = 0; bit < config.dataBits; bit++) {
			bool bitVal = (c >> bit) & 1;
			if(config.bitOrder == "msb-first") {
				bitVal = (c >> (config.dataBits - 1 - bit)) & 1;
			}
			if(config.invertRx && track.kind == TK_UART_RX) bitVal = !bitVal;
			if(config.invertTx && track.kind == TK_UART_TX) bitVal = !bitVal;
			
			track.transitions.Add(Transition{time, bitVal});
			track.samples.Add(Sample{time, bitVal});
			time += bitTime;
		}
		
		if(config.parity != "none") {
			int ones = 0;
			for(int bit = 0; bit < config.dataBits; bit++) {
				if((c >> bit) & 1) ones++;
			}
			bool parityBit = (config.parity == "even") ? (ones % 2 == 0) : (ones % 2 == 1);
			if(config.invertRx && track.kind == TK_UART_RX) parityBit = !parityBit;
			if(config.invertTx && track.kind == TK_UART_TX) parityBit = !parityBit;
			
			track.transitions.Add(Transition{time, parityBit});
			track.samples.Add(Sample{time, parityBit});
			time += bitTime;
		}
		
		int stopCount = (int)config.stopBits;
		for(int s = 0; s < stopCount; s++) {
			track.transitions.Add(Transition{time, idle});
			track.samples.Add(Sample{time, idle});
			time += bitTime;
		}
	}
	
	track.transitions.Add(Transition{time, idle});
	track.samples.Add(Sample{time, idle});
	
	if(time > this->timeEnd) this->timeEnd = time;
}

void CaptureModel::GenerateCustomBits(Track& track) {
	int64 durationUs = timeEnd;
	int64 time = 0;
	int64 period = 10000;
	
	for(int i = 0; i < 32; i++) {
		bool val = (i % 2 == 0);
		track.samples.Add(Sample{time, val});
		if(i > 0) track.transitions.Add(Transition{time, val});
		time += period;
		
		val = !val;
		track.samples.Add(Sample{time, val});
		track.transitions.Add(Transition{time, val});
		time += period;
	}
	track.transitions.Add(Transition{time, track.samples.Top().value});
	
	for(int i = 0; i < 8; i++) {
		DecodedBlock block;
		block.startTime = i * 4 * period;
		block.endTime = (i + 1) * 4 * period;
		block.text = Format("0x%02X", i * 17);
		block.color = track.color;
		block.type = DT_CUSTOM;
		block.trackIndex = tracks.GetCount() - 1;
		track.decodedBlocks.Add(block);
	}
}

void CaptureModel::GenerateCustomAddrData(Track& track) {
	int64 durationUs = timeEnd;
	int64 time = 0;
	int64 period = 20000;
	
	for(int i = 0; i < 16; i++) {
		bool addrPhase = (i % 2 == 0);
		for(int bit = 0; bit < 8; bit++) {
			bool val = addrPhase ? ((i / 2) >> bit) & 1 : (i * 13 + bit * 7) & 1;
			track.samples.Add(Sample{time, val});
			if(time > 0 || bit > 0) track.transitions.Add(Transition{time, val});
			time += period / 8;
		}
	}
	track.transitions.Add(Transition{time, track.samples.Top().value});
	
	for(int i = 0; i < 8; i++) {
		DecodedBlock block;
		block.startTime = i * 2 * period;
		block.endTime = (i + 1) * 2 * period;
		block.text = Format("A:0x%04X D:0x%02X", i * 256, i * 17);
		block.color = track.color;
		block.type = DT_CUSTOM;
		block.trackIndex = tracks.GetCount() - 1;
		track.decodedBlocks.Add(block);
	}
}

bool CaptureModel::RunSelfTest() {
	CaptureModel model;
	model.GenerateDemoCapture(4, 100000);
	model.DecodeAllUart();
	
	if(model.GetTrackCount() < 8) return false;
	
	const Track& ch0 = model.GetTrack(0);
	if(ch0.samples.IsEmpty() || ch0.transitions.IsEmpty()) return false;
	
	const Track& uartRx = model.GetTrack(4);
	if(uartRx.kind != TK_UART_RX) return false;
	if(uartRx.decodedBlocks.IsEmpty()) return false;
	
	const Track& customBits = model.GetTrack(6);
	if(customBits.kind != TK_CUSTOM_BITS) return false;
	
	const Track& customAddr = model.GetTrack(7);
	if(customAddr.kind != TK_CUSTOM_ADDR_DATA) return false;
	
	return true;
}