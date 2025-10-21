#include "ProtoVM.h"

/*
Links:
	- https://github.com/vygr/C-PCB
*/


NAMESPACE_UPP

void SetupTest1_Memory(Engine& mach);
void SetupTest2_6502(Engine& mach);
void SetupUK101(Engine& mach);
void SetupInterak(Engine& mach);
void SetupMiniMax8085(Engine& mach);

END_UPP_NAMESPACE

#ifdef flagMAIN

CONSOLE_APP_MAIN {
	using namespace Upp;
	
	Machine		mach;
	int			max_ticks = 100;
	
	switch (1) {
		case 0: SetupTest1_Memory(mach); break;
		case 1: SetupTest2_6502(mach); break;
		/*case 0: SetupUK101(mach); break;
		case 1: SetupInterak(mach); break;
		case 2: SetupMiniMax8085(mach); break;*/
		default: return;
	}
	
	if (mach.Init()) {
		
		for(int i = 0; i < max_ticks; i++) {
			LOG("Tick " << i);
			
			if (!mach.Tick())
				break;
			
		}
		
	}
}

#endif
