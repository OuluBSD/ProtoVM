#ifndef _ProtoVM_ProtoVM_h_
#define _ProtoVM_ProtoVM_h_

// Conditional inclusion: use real U++ if available, otherwise use mocks
#ifdef USE_UPP
    #ifdef USE_UPP
    #include <Core/Core.h>
#else
    #include "ProtoVMCommon/MockCore.h"
#endif
    #include <CtrlLib/CtrlLib.h>
    #include <plugin/bazaar/bazaar.h>
    #include <plugin/png/png.h>
    #include <plugin/fnt courier/Courier.h>
    #include <RichText/RichText.h>
    #include <Draw/Draw.h>
    #include <Painter/Painter.h>
    #include <plugin/swf/swf.h>
    #include <plugin/dds/dds.h>
    #include <plugin/wav/wav.h>
#else
    // Use mock types for standard C++ build
    #include "../ProtoVMCommon/MockCore.h"
    
    // Additional standard library includes
    #include <string>
    #include <vector>
    #include <memory>
    #include <iostream>
    #include <fstream>
    #include <map>
    #include <unordered_map>
    #include <set>
    #include <unordered_set>
    #include <algorithm>
    #include <functional>
    #include <chrono>
    #include <random>
    #include <thread>
    #include <mutex>
    #include <queue>
    #include <stack>
    #include <sstream>
    #include <iomanip>
    #include <cmath>
    #include <climits>
    #include <cfloat>
    #include <cstring>
    #include <cassert>
    
    // Define common macros used in the codebase
    #define Cout() std::cout
    #define min std::min
    #define max std::max
#endif

#include <stdint.h>

// Additional common includes for the project
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#endif