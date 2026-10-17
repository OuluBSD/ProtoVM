#include "CustomTracks.h"

void PCBitsDecoder::Decode(Track& track, const CaptureModel& model) {
	track.decodedBlocks.Clear();
	
	if(track.transitions.GetCount() < 8) return;
	
	int64 bitPeriod = 10000;
	for(size_t i = 0; i + 7 < track.transitions.GetCount(); i += 8) {
		int64 startTime = track.transitions[i].time;
		int val = 0;
		
		for(int bit = 0; bit < 8; bit++) {
			if(i + bit < track.transitions.GetCount()) {
				if(track.transitions[i + bit].value) val |= (1 << bit);
			}
		}
		
		DecodedBlock block;
		block.startTime = startTime;
		block.endTime = startTime + 8 * bitPeriod;
		block.text = Format("0x%02X", val);
		block.color = GetColor();
		block.type = DT_CUSTOM;
		track.decodedBlocks.Add(block);
	}
}

void PCAddrDataDecoder::Decode(Track& track, const CaptureModel& model) {
	track.decodedBlocks.Clear();
	
	if(track.transitions.GetCount() < 16) return;
	
	int64 cyclePeriod = 20000;
	for(size_t i = 0; i + 15 < track.transitions.GetCount(); i += 16) {
		int64 startTime = track.transitions[i].time;
		int addr = 0;
		int data = 0;
		
		for(int bit = 0; bit < 8; bit++) {
			if(i + bit < track.transitions.GetCount()) {
				if(track.transitions[i + bit].value) addr |= (1 << bit);
			}
		}
		for(int bit = 0; bit < 8; bit++) {
			if(i + 8 + bit < track.transitions.GetCount()) {
				if(track.transitions[i + 8 + bit].value) data |= (1 << bit);
			}
		}
		
		DecodedBlock block;
		block.startTime = startTime;
		block.endTime = startTime + 16 * (cyclePeriod / 8);
		block.text = Format("A:0x%04X D:0x%02X", addr, data);
		block.color = GetColor();
		block.type = DT_CUSTOM;
		track.decodedBlocks.Add(block);
	}
}

Array<CustomTrackDecoder*> GetBuiltinCustomDecoders() {
	static PCBitsDecoder pcBits;
	static PCAddrDataDecoder pcAddrData;
	return {&pcBits, &pcAddrData};
}

Track CreateCustomTrack(const String& name, TrackKind kind, const CaptureModel& model) {
	Track track;
	track.name = name;
	track.kind = kind;
	
	if(kind == TK_CUSTOM_BITS) {
		track.color = Orange();
	} else if(kind == TK_CUSTOM_ADDR_DATA) {
		track.color = Color(128, 0, 128);
	}
	
	track.visible = true;
	return track;
}