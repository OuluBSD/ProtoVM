#ifndef _LogicAnalyzer_CustomTracks_h
#define _LogicAnalyzer_CustomTracks_h

#include <Core/Core.h>
#include "CaptureModel.h"

using namespace Upp;

class CustomTrackDecoder {
public:
	virtual ~CustomTrackDecoder() {}
	virtual void Decode(Track& track, const CaptureModel& model) = 0;
	virtual String GetName() const = 0;
	virtual Color GetColor() const = 0;
};

class PCBitsDecoder : public CustomTrackDecoder {
public:
	String GetName() const override { return "PC: Bits"; }
	Color GetColor() const override { return Orange(); }
	void Decode(Track& track, const CaptureModel& model) override;
};

class PCAddrDataDecoder : public CustomTrackDecoder {
public:
	String GetName() const override { return "PC: Address/Data"; }
	Color GetColor() const override { return Color(128, 0, 128); }
	void Decode(Track& track, const CaptureModel& model) override;
};

Array<CustomTrackDecoder*> GetBuiltinCustomDecoders();
Track CreateCustomTrack(const String& name, TrackKind kind, const CaptureModel& model);

#endif