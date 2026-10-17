#ifndef _LogicAnalyzer_CaptureModel_h
#define _LogicAnalyzer_CaptureModel_h

#include <Core/Core.h>

using namespace Upp;

enum TrackKind {
	TK_RAW,
	TK_UART_RX,
	TK_UART_TX,
	TK_CUSTOM_BITS,
	TK_CUSTOM_ADDR_DATA,
};

enum DecoderType {
	DT_NONE,
	DT_UART,
	DT_CUSTOM,
};

struct Sample {
	int64 time;
	bool value;
};

struct Transition {
	int64 time;
	bool value;
};

struct DecodedBlock {
	int64 startTime;
	int64 endTime;
	String text;
	Color color;
	DecoderType type;
	int trackIndex;
};

struct Track {
	String name;
	TrackKind kind;
	Color color;
	bool visible = true;
	bool expanded = false;
	Array<Sample> samples;
	Array<Transition> transitions;
	Array<DecodedBlock> decodedBlocks;
	
	// UART specific
	int uartChannel = -1;
	
	// Custom track specific
	String customDecoder;
};

struct UartConfig {
	String name;
	Color color;
	int rxChannel = 1;
	int txChannel = -1;
	int baudRate = 9600;
	int dataBits = 8;
	String parity = "none";
	bool checkParity = true;
	double stopBits = 1.0;
	String bitOrder = "lsb-first";
	String dataFormat = "ascii";
	bool invertRx = false;
	bool invertTx = false;
	String requiredChannels;  // comma-separated list
	DecoderType decoderType = DT_UART;
};

class CaptureModel {
public:
	typedef CaptureModel CLASSNAME;
	
	CaptureModel();
	
	void Clear();
	void GenerateDemoCapture(int numChannels = 8, int64 durationUs = 1000000);
	void AddCustomTrack(const String& name, TrackKind kind);
	void RemoveTrack(int index);
	void DecodeUart(int trackIndex, const UartConfig& config);
	void DecodeAllUart();
	
	const Array<Track>& GetTracks() const { return tracks; }
	Array<Track>& GetTracks() { return tracks; }
	const Track& GetTrack(int index) const { return tracks[index]; }
	Track& GetTrack(int index) { return tracks[index]; }
	int GetTrackCount() const { return tracks.GetCount(); }
	
	const Array<UartConfig>& GetUartConfigs() const { return uartConfigs; }
	Array<UartConfig>& GetUartConfigs() { return uartConfigs; }
	
	void SetSampleRate(int rate) { sampleRate = rate; }
	int GetSampleRate() const { return sampleRate; }
	
	void SetTimeRange(int64 start, int64 end) { timeStart = start; timeEnd = end; }
	int64 GetTimeStart() const { return timeStart; }
	int64 GetTimeEnd() const { return timeEnd; }
	
	void GenerateUartSignal(Track& track, const UartConfig& config, const String& data);
	
	// Self-test
	static bool RunSelfTest();

private:
	Array<Track> tracks;
	Array<UartConfig> uartConfigs;
	int sampleRate = 1000000;
	int64 timeStart = 0;
	int64 timeEnd = 1000000;
	
	void GenerateCustomBits(Track& track);
	void GenerateCustomAddrData(Track& track);
};

#endif