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
	void ProcessInspectCommand(const Vector<String>& tokens);
	void ProcessStateCommand(const Vector<String>& tokens);
	void ProcessVisualizeCommand(const Vector<String>& tokens);
	void ProcessNetlistCommand(const Vector<String>& tokens);
	void ProcessTraceCommand(const Vector<String>& tokens);
	void ProcessTraceLogCommand(const Vector<String>& tokens);
	void ProcessLoadCommand(const Vector<String>& tokens);
	void ProcessStepCommand(const Vector<String>& tokens);
	void ProcessContinueCommand(const Vector<String>& tokens);
	void ProcessBreakCommand(const Vector<String>& tokens);
	void ProcessMemoryDumpCommand(const Vector<String>& tokens);

public:
	// Public API for programmatic access to CLI functionality
	void AddSignalTrace(const String& componentName, const String& pinName, int pcbId = 0);
	void ShowSignalTraceLog();
	
private:
	// Additional members
	bool running_in_step_mode; // Track if in step mode
};

#endif