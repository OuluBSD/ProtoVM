#ifndef _ProtoVM_JsonIO_h_
#define _ProtoVM_JsonIO_h_

#include "ProtoVM.h"  // Include U++ types
#include "SessionTypes.h"
#include <string>

namespace ProtoVMCLI {

// Simple JSON utilities for the CLI using U++ types
class JsonIO {
public:
    // Create a success response with a standard envelope
    static Upp::String SuccessResponse(const std::string& command, const Upp::ValueMap& data = Upp::ValueMap());

    // Create an error response with a standard envelope
    static Upp::String ErrorResponse(const std::string& command, const std::string& error_msg, const std::string& error_code = "");

    // Create a response from a Result type with a standard envelope
    template<typename T>
    static Upp::String FromResult(const std::string& command, const Result<T>& result,
                                  std::function<Upp::ValueMap(const T&)> converter = nullptr);

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

    // Convert ErrorCode to string
    static std::string ErrorCodeToString(ErrorCode code);

    // Convert string to ErrorCode
    static ErrorCode StringToErrorCode(const std::string& s);
};

// Template implementation needs to be in header
template<typename T>
Upp::String JsonIO::FromResult(const std::string& command, const Result<T>& result,
                               std::function<Upp::ValueMap(const T&)> converter) {
    if (result.ok) {
        Upp::ValueMap data;
        if (converter) {
            data = converter(result.data);
        } else {
            // For simple types, just return an empty object or add a value field
            data.Add("value", Upp::Value(result.data));
        }
        return SuccessResponse(command, data);
    } else {
        std::string code_str = ErrorCodeToString(result.error_code);
        return ErrorResponse(command, result.error_message, code_str);
    }
}

} // namespace ProtoVMCLI

#endif