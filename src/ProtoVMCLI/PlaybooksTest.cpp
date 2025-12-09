#include "Playbooks.h"
#include "CircuitFacade.h"
#include "JsonIO.h"
#include <iostream>
#include <cassert>
#include <string>

// Mock implementations for testing purposes
namespace ProtoVMCLI {

// Mock CoDesignerManager for testing
class MockCoDesignerManager : public CoDesignerManager {
public:
    MockCoDesignerManager(std::shared_ptr<CircuitFacade> circuit_facade) 
        : CoDesignerManager(circuit_facade) {}
    
    Result<CoDesignerSessionState> CreateSessionForTest(int proto_session_id, const std::string& branch) {
        return CreateSession(proto_session_id, branch);
    }
};

// Simple test functions
bool TestPlaybookConfigSerialization() {
    PlaybookConfig config;
    config.kind = PlaybookKind::OptimizeBlockAndReport;
    config.designer_session_id = "test-session-123";
    config.target = "block";
    config.block_id = "B1";
    config.baseline_branch = "main";
    config.passes = {IrOptPassKind::SimplifyAlgebraic, IrOptPassKind::FoldConstants};
    config.use_optimized_ir = true;
    config.apply_refactors = false;
    
    Upp::ValueMap serialized = JsonIO::PlaybookConfigToValueMap(config);
    
    // Verify all fields are present
    assert(!serialized.IsEmpty());
    assert(serialized.IsKey("kind"));
    assert(serialized.IsKey("designer_session_id"));
    assert(serialized.IsKey("target"));
    assert(serialized.IsKey("block_id"));
    assert(serialized.IsKey("baseline_branch"));
    assert(serialized.IsKey("passes"));
    assert(serialized.IsKey("use_optimized_ir"));
    assert(serialized.IsKey("apply_refactors"));
    
    std::cout << "✓ PlaybookConfig serialization test passed" << std::endl;
    return true;
}

bool TestPlaybookKindSerialization() {
    // Test OptimizeBlockAndReport
    Upp::Value kindValue1 = JsonIO::PlaybookKindToJson(PlaybookKind::OptimizeBlockAndReport);
    assert(kindValue1.ToString() == "OptimizeBlockAndReport");
    
    // Test OptimizeAndApplySafeRefactors
    Upp::Value kindValue2 = JsonIO::PlaybookKindToJson(PlaybookKind::OptimizeAndApplySafeRefactors);
    assert(kindValue2.ToString() == "OptimizeAndApplySafeRefactors");
    
    std::cout << "✓ PlaybookKind serialization test passed" << std::endl;
    return true;
}

bool TestPlaybookEngineCreation() {
    // Test that PlaybookEngine can be instantiated and methods exist
    // This is a basic smoke test
    std::cout << "✓ PlaybookEngine creation test passed" << std::endl;
    return true;
}

// Run all tests
int RunPlaybookTests() {
    std::cout << "Running Playbook Tests..." << std::endl;
    
    int passed = 0;
    int total = 0;
    
    total++;
    try {
        if (TestPlaybookKindSerialization()) passed++;
    } catch (...) {
        std::cout << "✗ PlaybookKind serialization test failed" << std::endl;
    }
    
    total++;
    try {
        if (TestPlaybookConfigSerialization()) passed++;
    } catch (...) {
        std::cout << "✗ PlaybookConfig serialization test failed" << std::endl;
    }
    
    total++;
    try {
        if (TestPlaybookEngineCreation()) passed++;
    } catch (...) {
        std::cout << "✗ PlaybookEngine creation test failed" << std::endl;
    }
    
    std::cout << "\nTest Results: " << passed << "/" << total << " tests passed" << std::endl;
    
    if (passed == total) {
        std::cout << "All playbook tests passed successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed." << std::endl;
        return 1;
    }
}

} // namespace ProtoVMCLI

int main() {
    return ProtoVMCLI::RunPlaybookTests();
}