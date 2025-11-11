#include "ProtoVM.h"
#include "Test4004Output.h"  // This header will be auto-generated or we'll call the function directly

int main(int argc, const char* argv[]) {
    // Initialize logging
    LOG("ProtoVM 4004 Output Test Runner\n");
    
    // Run the 4004 output tests
    int result = Run4004OutputTests();
    
    return result;
}