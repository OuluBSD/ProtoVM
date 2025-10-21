#pragma once




class Machine {
public:
	Array<Pcb> pcbs;
	//Port power;
	LinkBaseMap l;
	
	bool Init();
	bool Tick();
	bool RunInitOps();
	bool RunRtOps();
	bool RunRtOpsWithChangeDetection(bool& changed);
	uint64 GetStateHash();
	bool IsStateInHistory(uint64 current_state, const Vector<uint64>& history);
	
	Pcb& AddPcb();
	//Port& GetPower() {return power;}
	
};




