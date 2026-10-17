#include "DemoCapture.h"
#include "CaptureModel.h"

void GenerateDemoCapture(CaptureModel& model, int numChannels, int64 durationUs) {
	model.GenerateDemoCapture(numChannels, durationUs);
}

void GenerateUartDemo(CaptureModel& model, const String& rxData, const String& txData, int baudRate) {
	model.Clear();
	model.SetSampleRate(1000000);
	
	UartConfig uart;
	uart.name = "UART Demo";
	uart.color = LtBlue();
	uart.rxChannel = 0;
	uart.txChannel = 1;
	uart.baudRate = baudRate;
	uart.dataBits = 8;
	uart.parity = "none";
	uart.checkParity = true;
	uart.stopBits = 1.0;
	uart.bitOrder = "lsb-first";
	uart.dataFormat = "ascii";
	model.GetUartConfigs().Add(uart);
	
	model.SetTimeRange(0, 0);
	
	for(int i = 0; i < 2; i++) {
		Track& t = model.GetTracks().Add();
		t.name = Format("CH%d", i);
		t.kind = TK_RAW;
		t.color = Color((i * 37 + 100) % 255, (i * 73 + 50) % 255, (i * 11 + 150) % 255);
		t.visible = true;
	}
	
	Track& rxTrack = model.GetTracks().Add();
	rxTrack.name = "UART RX";
	rxTrack.kind = TK_UART_RX;
	rxTrack.color = LtBlue();
	rxTrack.uartChannel = 0;
	
	Track& txTrack = model.GetTracks().Add();
	txTrack.name = "UART TX";
	txTrack.kind = TK_UART_TX;
	txTrack.color = LtGreen();
	txTrack.uartChannel = 0;
	
	model.GenerateUartSignal(rxTrack, uart, rxData);
	model.GenerateUartSignal(txTrack, uart, txData);
	model.DecodeUart(model.GetTrackCount() - 2, uart);
	model.DecodeUart(model.GetTrackCount() - 1, uart);
}

void GenerateSpiDemo(CaptureModel& model, int numBytes) {
	model.Clear();
	model.SetSampleRate(1000000);
	model.SetTimeRange(0, numBytes * 8 * 1000);
	
	const char* names[] = {"SCLK", "MOSI", "MISO", "CS"};
	Color colors[] = {Red(), Green(), Blue(), Color(255, 165, 0)};
	
	for(int i = 0; i < 4; i++) {
		Track& t = model.GetTracks().Add();
		t.name = names[i];
		t.kind = TK_RAW;
		t.color = colors[i];
		t.visible = true;
		
		int64 period = 2000;
		for(int64 time = 0; time < model.GetTimeEnd(); time += period / 2) {
			bool val = (time / (period / 2)) % 2 == 0;
			if(i == 3) val = (time / (numBytes * 8 * period / 2)) % 2 == 0;
			t.samples.Add({time, val});
			if(time > 0) t.transitions.Add({time, val});
		}
		t.transitions.Add({model.GetTimeEnd(), !t.samples.Top().value});
	}
}

void GenerateI2cDemo(CaptureModel& model) {
	model.Clear();
	model.SetSampleRate(1000000);
	model.SetTimeRange(0, 100000);
	
	const char* names[] = {"SCL", "SDA"};
	Color colors[] = {Red(), Green()};
	
	for(int i = 0; i < 2; i++) {
		Track& t = model.GetTracks().Add();
		t.name = names[i];
		t.kind = TK_RAW;
		t.color = colors[i];
		t.visible = true;
		
		int64 period = 5000;
		for(int64 time = 0; time < model.GetTimeEnd(); time += period / 2) {
			bool val = (time / (period / 2)) % 2 == 0;
			if(i == 1) val = (time / (period * 2)) % 2 == 0;
			t.samples.Add({time, val});
			if(time > 0) t.transitions.Add({time, val});
		}
		t.transitions.Add({model.GetTimeEnd(), !t.samples.Top().value});
	}
}