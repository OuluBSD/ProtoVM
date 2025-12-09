#include "SessionServer.h"
#include "JsonIO.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // Initialize the session server
    ProtoVMCLI::SessionServer server;
    
    std::cout << "ProtoVM Daemon starting..." << std::endl;
    
    // Process requests from stdin
    auto result = server.ProcessRequests();
    
    if (!result.ok) {
        // Output error as a JSON response
        Upp::ValueMap error_response;
        error_response.Add("ok", false);
        error_response.Add("command", Upp::String("daemon"));
        error_response.Add("error_code", Upp::String(ProtoVMCLI::JsonIO::ErrorCodeToString(result.error_code).c_str()));
        error_response.Add("error", Upp::String(result.error_message.c_str()));
        error_response.Add("data", Upp::Value());
        
        std::cout << ProtoVMCLI::JsonIO::ValueMapToJson(error_response).ToStd() << std::endl;
        return 1;
    }
    
    return 0;
}