#include "JsonIO.h"
#include <iostream>
#include <sstream>

namespace ProtoVMCLI {

Upp::String JsonIO::SuccessResponse(const Upp::ValueMap& data) {
    Upp::ValueMap response;
    response.Add("ok", true);
    if (!data.IsEmpty()) {
        response.Add("data", data);
    }
    return ValueMapToJson(response);
}

Upp::String JsonIO::ErrorResponse(const std::string& error_msg, const std::string& error_code) {
    Upp::ValueMap response;
    response.Add("ok", false);
    response.Add("error", Upp::String(error_msg.c_str()));
    if (!error_code.empty()) {
        response.Add("error_code", Upp::String(error_code.c_str()));
    }
    return ValueMapToJson(response);
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