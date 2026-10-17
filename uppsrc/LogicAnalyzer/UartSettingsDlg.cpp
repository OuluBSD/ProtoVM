#include "UartSettingsDlg.h"

UartSettingsDlg::UartSettingsDlg(UartConfig& cfg, const Array<String>& availableChannels)
: config(cfg) {
	for(const String& ch : availableChannels)
		channels.Add(ch);
	Title("UART Settings");
	Sizeable(false);
	
	SetupControls();
	LayoutControls();
	
	okBtn <<= THISBACK(OnOK);
	cancelBtn <<= THISBACK(OnCancel);
	
	CenterScreen();
}

void UartSettingsDlg::SetupControls() {
	name <<= config.name;
	color <<= config.color;
	
	for(int i = 0; i < channels.GetCount(); i++) {
		rxChannel.Add(i, channels[i]);
		txChannel.Add(i, channels[i]);
	}
	txChannel.Add(-1, "---");
	
	rxChannel <<= config.rxChannel >= 0 && config.rxChannel < channels.GetCount() ? config.rxChannel : 0;
	txChannel <<= config.txChannel >= 0 && config.txChannel < channels.GetCount() ? config.txChannel : -1;
	
	baudRate.MinMax(300, 4000000);
	baudRate.SetInc(100);
	baudRate <<= config.baudRate;
	
	dataBits.Add(5, "5");
	dataBits.Add(6, "6");
	dataBits.Add(7, "7");
	dataBits.Add(8, "8");
	dataBits.Add(9, "9");
	dataBits <<= config.dataBits;
	
	parity.Add("none", "none");
	parity.Add("even", "even");
	parity.Add("odd", "odd");
	parity.Add("mark", "mark");
	parity.Add("space", "space");
	parity <<= config.parity;
	
	checkParity.Add(true, "yes");
	checkParity.Add(false, "no");
	checkParity <<= config.checkParity;
	
	stopBits.Add(1.0, "1.0");
	stopBits.Add(1.5, "1.5");
	stopBits.Add(2.0, "2.0");
	stopBits <<= config.stopBits;
	
	bitOrder.Add("lsb-first", "lsb-first");
	bitOrder.Add("msb-first", "msb-first");
	bitOrder <<= config.bitOrder;
	
	dataFormat.Add("ascii", "ascii");
	dataFormat.Add("hex", "hex");
	dataFormat.Add("dec", "dec");
	dataFormat.Add("oct", "oct");
	dataFormat.Add("bin", "bin");
	dataFormat <<= config.dataFormat;
	
	invertRx <<= config.invertRx;
	invertTx <<= config.invertTx;
	
	requiredChannels <<= config.requiredChannels;
	
	decoderType.Add(DT_UART, "Stack Decoder");
	decoderType.Add(DT_CUSTOM, "Custom");
	decoderType <<= (int)config.decoderType;
	
	okBtn.SetLabel("OK");
	cancelBtn.SetLabel("Cancel");
}

void UartSettingsDlg::LayoutControls() {
	const int labelWidth = 120;
	const int controlWidth = 200;
	const int rowHeight = 30;
	const int startY = 10;
	const int startX = 10;
	
	int y = startY;
	
	auto AddLabel = [&](const char* text) {
		Label* lbl = new Label();
		lbl->SetLabel(text);
		Add(lbl->LeftPosZ(startX, labelWidth).TopPosZ(y, 20));
	};
	
	auto AddCtrl = [&](Ctrl& ctrl) {
		Add(ctrl.LeftPosZ(startX + labelWidth + 10, controlWidth).TopPosZ(y, 20));
		y += rowHeight;
	};
	
	AddLabel("Name:");
	AddCtrl(name);
	
	AddLabel("Color:");
	AddCtrl(color);
	
	AddLabel("RX Channel:");
	AddCtrl(rxChannel);
	
	AddLabel("TX Channel:");
	AddCtrl(txChannel);
	
	AddLabel("Baud Rate:");
	AddCtrl(baudRate);
	
	AddLabel("Data Bits:");
	AddCtrl(dataBits);
	
	AddLabel("Parity:");
	AddCtrl(parity);
	
	AddLabel("Check Parity:");
	AddCtrl(checkParity);
	
	AddLabel("Stop Bits:");
	AddCtrl(stopBits);
	
	AddLabel("Bit Order:");
	AddCtrl(bitOrder);
	
	AddLabel("Data Format:");
	AddCtrl(dataFormat);
	
	Add(invertRx.LeftPosZ(startX + labelWidth + 10, controlWidth).TopPosZ(y, 20));
	invertRx.SetLabel("Invert RX");
	invertRx <<= config.invertRx;
	y += rowHeight;
	
	Add(invertTx.LeftPosZ(startX + labelWidth + 10, controlWidth).TopPosZ(y, 20));
	invertTx.SetLabel("Invert TX");
	invertTx <<= config.invertTx;
	y += rowHeight;
	
	AddLabel("Required Channels:");
	AddCtrl(requiredChannels);
	
	AddLabel("Decoder Type:");
	AddCtrl(decoderType);
	
	Add(okBtn.RightPosZ(10, 80).BottomPosZ(10, 30));
	Add(cancelBtn.RightPosZ(100, 80).BottomPosZ(10, 30));
	
	SetRect(0, 0, 450, y + 60);
}

void UartSettingsDlg::OnOK() {
	config.name = ~name;
	config.color = ~color;
	config.rxChannel = ~rxChannel;
	config.txChannel = ~txChannel;
	config.baudRate = ~baudRate;
	config.dataBits = ~dataBits;
	config.parity = ~parity;
	config.checkParity = ~checkParity;
	config.stopBits = ~stopBits;
	config.bitOrder = ~bitOrder;
	config.dataFormat = ~dataFormat;
	config.invertRx = ~invertRx;
	config.invertTx = ~invertTx;
	config.decoderType = (DecoderType)(int)~decoderType;
	
	config.requiredChannels.Clear();
	String req = ~requiredChannels;
	Vector<String> parts = Split(req, ',');
	for(const String& p : parts) {
		String t = TrimBoth(p);
		if(!t.IsEmpty()) {
			if(!config.requiredChannels.IsEmpty()) config.requiredChannels << ',';
			config.requiredChannels << ScanInt(t);
		}
	}
	
	Break(IDOK);
}

void UartSettingsDlg::OnCancel() {
	Break(IDCANCEL);
}