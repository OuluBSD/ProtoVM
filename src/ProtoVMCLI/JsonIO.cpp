#include "JsonIO.h"
#include <iostream>
#include <sstream>

namespace ProtoVMCLI {

Upp::String JsonIO::SuccessResponse(const std::string& command, const Upp::ValueMap& data) {
    Upp::ValueMap response;
    response.Add("ok", true);
    response.Add("command", Upp::String(command.c_str()));
    response.Add("error_code", Upp::Value());
    response.Add("error", Upp::Value());
    if (!data.IsEmpty()) {
        response.Add("data", data);
    } else {
        response.Add("data", Upp::Value());
    }
    return ValueMapToJson(response);
}

Upp::String JsonIO::ErrorResponse(const std::string& command, const std::string& error_msg, const std::string& error_code) {
    Upp::ValueMap response;
    response.Add("ok", false);
    response.Add("command", Upp::String(command.c_str()));
    if (!error_code.empty()) {
        response.Add("error_code", Upp::String(error_code.c_str()));
    } else {
        response.Add("error_code", Upp::Value());
    }
    response.Add("error", Upp::String(error_msg.c_str()));
    response.Add("data", Upp::Value());
    return ValueMapToJson(response);
}

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

std::string JsonIO::ErrorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::None:
            return "NONE";
        case ErrorCode::WorkspaceNotFound:
            return "WORKSPACE_NOT_FOUND";
        case ErrorCode::InvalidWorkspace:
            return "INVALID_WORKSPACE";
        case ErrorCode::WorkspaceCorrupt:
            return "WORKSPACE_CORRUPT";
        case ErrorCode::SessionNotFound:
            return "SESSION_NOT_FOUND";
        case ErrorCode::SessionCorrupt:
            return "SESSION_CORRUPT";
        case ErrorCode::SessionDeleted:
            return "SESSION_DELETED";
        case ErrorCode::SessionIdConflict:
            return "SESSION_ID_CONFLICT";
        case ErrorCode::CircuitFileNotFound:
            return "CIRCUIT_FILE_NOT_FOUND";
        case ErrorCode::CircuitFileUnreadable:
            return "CIRCUIT_FILE_UNREADABLE";
        case ErrorCode::StorageIoError:
            return "STORAGE_IO_ERROR";
        case ErrorCode::StorageSchemaMismatch:
            return "STORAGE_SCHEMA_MISMATCH";
        case ErrorCode::CommandParseError:
            return "COMMAND_PARSE_ERROR";
        case ErrorCode::InternalError:
            return "INTERNAL_ERROR";
        default:
            return "UNKNOWN_ERROR";
    }
}

ErrorCode JsonIO::StringToErrorCode(const std::string& s) {
    if (s == "NONE") return ErrorCode::None;
    if (s == "WORKSPACE_NOT_FOUND") return ErrorCode::WorkspaceNotFound;
    if (s == "INVALID_WORKSPACE") return ErrorCode::InvalidWorkspace;
    if (s == "WORKSPACE_CORRUPT") return ErrorCode::WorkspaceCorrupt;
    if (s == "SESSION_NOT_FOUND") return ErrorCode::SessionNotFound;
    if (s == "SESSION_CORRUPT") return ErrorCode::SessionCorrupt;
    if (s == "SESSION_DELETED") return ErrorCode::SessionDeleted;
    if (s == "SESSION_ID_CONFLICT") return ErrorCode::SessionIdConflict;
    if (s == "CIRCUIT_FILE_NOT_FOUND") return ErrorCode::CircuitFileNotFound;
    if (s == "CIRCUIT_FILE_UNREADABLE") return ErrorCode::CircuitFileUnreadable;
    if (s == "STORAGE_IO_ERROR") return ErrorCode::StorageIoError;
    if (s == "STORAGE_SCHEMA_MISMATCH") return ErrorCode::StorageSchemaMismatch;
    if (s == "COMMAND_PARSE_ERROR") return ErrorCode::CommandParseError;
    if (s == "INTERNAL_ERROR") return ErrorCode::InternalError;
    return ErrorCode::InternalError;  // default for unknown error codes
}

Upp::ValueMap JsonIO::ParseArgs(int argc, char** argv) {
    Upp::ValueMap args;

    for (int i = 1; i < argc; i++) {
        Upp::String arg = argv[i];

        if (arg.StartsWith("--")) {
            Upp::String key = arg.Mid(2);
            Upp::String value = "";

            if (i + 1 < argc && argv[i + 1][0] != '-') {
                value = argv[++i];
            }

            args.Add(key, value);
        }
        else if (arg[0] == '-') {
            // Short form options
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                Upp::String key = arg.Mid(1);
                Upp::String value = argv[++i];
                args.Add(key, value);
            }
        }
        else {
            // Positional argument - handle command hierarchy
            if (!args.IsKey("command")) {
                args.Add("command", arg);
            } else if (args.Get("command", Upp::String("")) == "debug") {
                // Handle debug subcommands
                if (!args.IsKey("subcommand")) {
                    args.Add("subcommand", arg);
                } else if (args.Get("subcommand", Upp::String("")) == "process" ||
                          args.Get("subcommand", Upp::String("")) == "websocket" ||
                          args.Get("subcommand", Upp::String("")) == "poll") {
                    if (!args.IsKey("action")) {
                        args.Add("action", arg);
                    }
                }
            } else {
                // Store additional positional arguments if needed in the future
                args.Add("extra_" + Upp::AsString(i), arg);
            }
        }
    }

    return args;
}

Upp::String JsonIO::Serialize(const Upp::ValueMap& obj) {
    return ValueMapToJson(obj);
}

Upp::ValueMap JsonIO::Deserialize(const Upp::String& str) {
    // This is a simplified implementation that would need a full JSON parser in real use
    Upp::ValueMap empty_map;
    return empty_map; // Return empty for now
}

Upp::String JsonIO::ValueMapToJson(const Upp::ValueMap& vm) {
    Upp::String result = "{";
    bool first = true;
    
    for (int i = 0; i < vm.GetCount(); i++) {
        if (!first) {
            result += ",";
        }
        result += "\"" + vm.GetKey(i) + "\":" + ValueToJson(vm[i]);
        first = false;
    }
    
    result += "}";
    return result;
}

Upp::String JsonIO::ValueArrayToJson(const Upp::ValueArray& va) {
    Upp::String result = "[";
    bool first = true;
    
    for (int i = 0; i < va.GetCount(); i++) {
        if (!first) {
            result += ",";
        }
        result += ValueToJson(va[i]);
        first = false;
    }
    
    result += "]";
    return result;
}

Upp::String JsonIO::ValueToJson(const Upp::Value& val) {
    if (val.Is<int>()) {
        return Upp::AsString(val.Get<int>());
    } else if (val.Is<bool>()) {
        return val.Get<bool>() ? Upp::String("true") : Upp::String("false");
    } else if (val.Is<double>()) {
        return Upp::AsString(val.Get<double>());
    } else if (val.Is<Upp::String>()) {
        Upp::String str = val.Get<Upp::String>();
        // Escape quotes and other special characters
        str = Upp::String().Cat() << "\"" << str.Replace("\"", "\\\"") << "\"";
        return str;
    } else if (val.Is<Upp::ValueMap>()) {
        return ValueMapToJson(val.Get<Upp::ValueMap>());
    } else if (val.Is<Upp::ValueArray>()) {
        return ValueArrayToJson(val.Get<Upp::ValueArray>());
    } else {
        // Default to string representation
        return Upp::String().Cat() << "\"" << Upp::AsString(val) << "\"";
    }
}

} // namespace ProtoVMCLI