#pragma once

NAMESPACE_UPP


class Machine {
public:
	Array<Pcb> pcbs;
	//Port power;
	LinkMap l;
	
	bool Init();
	bool Tick();
	bool RunInitOps();
	bool RunRtOps();
	
	Pcb& AddPcb();
	//Port& GetPower() {return power;}
	
};



END_UPP_NAMESPACE
