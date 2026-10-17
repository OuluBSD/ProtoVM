#ifndef _LogicAnalyzer_UartSettingsDlg_h
#define _LogicAnalyzer_UartSettingsDlg_h

#include <CtrlLib/CtrlLib.h>
#include "CaptureModel.h"

using namespace Upp;

class UartSettingsDlg : public TopWindow {
public:
	typedef UartSettingsDlg CLASSNAME;
	
	UartSettingsDlg(UartConfig& config, const Array<String>& availableChannels);
	
	const UartConfig& GetConfig() const { return config; }

private:
	UartConfig& config;
	Array<String> channels;
	
	EditString name;
	ColorButton color;
	DropList rxChannel;
	DropList txChannel;
	EditIntSpin baudRate;
	DropList dataBits;
	DropList parity;
	DropList checkParity;
	DropList stopBits;
	DropList bitOrder;
	DropList dataFormat;
	Option invertRx;
	Option invertTx;
	EditString requiredChannels;
	DropList decoderType;
	Button okBtn;
	Button cancelBtn;
	
	void SetupControls();
	void LayoutControls();
	void OnOK();
	void OnCancel();
};

#endif