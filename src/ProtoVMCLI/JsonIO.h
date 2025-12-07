#ifndef _ProtoVM_JsonIO_h_
#define _ProtoVM_JsonIO_h_

#include "ProtoVM.h"  // Include U++ types
#include <string>

namespace ProtoVMCLI {

// Simple JSON utilities for the CLI using U++ types
class JsonIO {
public:
    // Create a success response
    static Upp::String SuccessResponse(const Upp::ValueMap& data = Upp::ValueMap());
    
    // Create an error response
    static Upp::String ErrorResponse(const std::string& error_msg, const std::string& error_code = "");
    
    // Parse command line arguments to ValueMap
    static Upp::ValueMap ParseArgs(int argc, char** argv);
    
    // Serialize ValueMap to JSON string with consistent formatting
    static Upp::String Serialize(const Upp::ValueMap& obj);
    
    // Read ValueMap from JSON string
    static Upp::ValueMap Deserialize(const Upp::String& str);
    
    // Helper to convert ValueMap to JSON string (recursive)
    static Upp::String ValueMapToJson(const Upp::ValueMap& vm);
    
    // Helper to convert ValueArray to JSON string
    static Upp::String ValueArrayToJson(const Upp::ValueArray& va);
    
    // Helper to convert a single value to JSON
    static Upp::String ValueToJson(const Upp::Value& val);
};

} // namespace ProtoVMCLI

#endif