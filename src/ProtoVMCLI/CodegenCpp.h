#ifndef _ProtoVM_CodegenCpp_h_
#define _ProtoVM_CodegenCpp_h_

#include "CodegenIr.h"
#include "SessionTypes.h"
#include <string>
#include <vector>

namespace ProtoVMCLI {

struct CppClassOptions {
    std::string class_name;           // e.g. "OscBlock"
    std::string state_class_name;     // e.g. "OscState"
    std::string namespace_name;       // optional namespace, empty if none

    bool generate_render_method = false;  // if true, emit render() that loops step()
    std::string step_method_name = "Step";     // e.g. "Step"
    std::string render_method_name = "Render"; // e.g. "Render";

    double default_sample_rate = 48000.0; // used for demo code & audio DSL
};

} // namespace ProtoVMCLI

#endif // _ProtoVM_CodegenCpp_h_