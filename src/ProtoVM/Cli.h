#ifndef _ProtoVM_Cli_h_
#define _ProtoVM_Cli_h_

#include "ProtoVM.h"

class Cli {
private:
	Machine* machine;
	bool running;
	
public:
	Cli();
	~Cli();
	
	void SetMachine(Machine* mach);
	void Start();
	void Stop();
	void ProcessCommand(const String& command);
	
private:
	void ShowHelp();
	void ProcessWriteCommand(const Vector<String>& tokens);
	void ProcessReadCommand(const Vector<String>& tokens);
	void ProcessRunCommand(const Vector<String>& tokens);
	void ProcessQuitCommand();
	void ProcessListCommand();
};

#endif