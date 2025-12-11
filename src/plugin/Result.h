#ifndef _plugin_Result_h_
#define _plugin_Result_h_

#include <string>

namespace plugin {

// Error codes for the plugin system
enum class ErrorCode {
    None = 0,
    PluginNotFound,
    PluginLoadError,
    PluginInitializationError,
    PluginFunctionNotFound,
    PluginExecutionError,
    PluginConfigurationError
};

// Result template to encapsulate success/error responses
template<typename T>
struct Result {
    bool ok;
    ErrorCode error_code;
    std::string error_message;  // human-readable message
    T data;

    Result(T val) : ok(true), error_code(ErrorCode::None), error_message(""), data(val) {}
    Result(bool is_ok, ErrorCode code, const std::string& msg, T val)
        : ok(is_ok), error_code(code), error_message(msg), data(val) {}

    static Result<T> MakeOk(const T& data) {
        return Result<T>(true, ErrorCode::None, "", data);
    }

    static Result<T> MakeError(ErrorCode code, const std::string& message) {
        return Result<T>(false, code, message, T{});
    }
};

} // namespace plugin

#endif // _plugin_Result_h_