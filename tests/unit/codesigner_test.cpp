#include "ProtoVMCLI/CoDesigner.h"
#include "ProtoVMCLI/CircuitFacade.h"
#include "ProtoVMCLI/JsonIO.h"
#include <iostream>
#include <cassert>
#include <string>

using namespace ProtoVMCLI;

void TestCoDesignerSessionStateSerialization() {
    std::cout << "Testing CoDesignerSessionState serialization...\n";
    
    CoDesignerSessionState session;
    session.designer_session_id = "cd-test-session-123";
    session.proto_session_id = 42;
    session.branch = "main";
    session.current_block_id = "B1";
    session.current_node_id = "C10:OUT";
    session.current_node_kind = "Pin";
    session.use_optimized_ir = true;
    
    Upp::ValueMap serialized = JsonIO::CoDesignerSessionStateToValueMap(session);
    
    assert(serialized.Get("designer_session_id", Upp::String("")).ToString() == "cd-test-session-123");
    assert(serialized.Get("proto_session_id", -1) == 42);
    assert(serialized.Get("branch", Upp::String("")).ToString() == "main");
    assert(serialized.Get("current_block_id", Upp::String("")).ToString() == "B1");
    assert(serialized.Get("current_node_id", Upp::String("")).ToString() == "C10:OUT");
    assert(serialized.Get("current_node_kind", Upp::String("")).ToString() == "Pin");
    assert(serialized.Get("use_optimized_ir", false) == true);
    
    std::cout << "✓ CoDesignerSessionState serialization test passed\n";
}

void TestCoDesignerManagerLifecycle() {
    std::cout << "Testing CoDesignerManager lifecycle...\n";
    
    auto circuit_facade = std::make_shared<CircuitFacade>();
    CoDesignerManager manager(circuit_facade);
    
    // Test session creation
    auto create_result = manager.CreateSession(1, "main");
    assert(create_result.ok);
    assert(create_result.data.proto_session_id == 1);
    assert(create_result.data.branch == "main");
    assert(!create_result.data.designer_session_id.empty());
    
    std::string session_id = create_result.data.designer_session_id;
    
    // Test session retrieval
    auto get_result = manager.GetSession(session_id);
    assert(get_result.ok);
    assert(get_result.data.designer_session_id == session_id);
    
    // Test session update
    auto session_to_update = get_result.data;
    session_to_update.current_block_id = "B5";
    session_to_update.use_optimized_ir = true;
    
    auto update_result = manager.UpdateSession(session_to_update);
    assert(update_result.ok);
    
    // Verify the update
    auto verify_result = manager.GetSession(session_id);
    assert(verify_result.ok);
    assert(verify_result.data.current_block_id == "B5");
    assert(verify_result.data.use_optimized_ir == true);
    
    // Test session destruction
    auto destroy_result = manager.DestroySession(session_id);
    assert(destroy_result.ok);
    
    // Verify session is gone
    auto missing_result = manager.GetSession(session_id);
    assert(!missing_result.ok);
    
    std::cout << "✓ CoDesignerManager lifecycle test passed\n";
}

void TestCoDesignerManagerNegativeCases() {
    std::cout << "Testing CoDesignerManager negative cases...\n";
    
    auto circuit_facade = std::make_shared<CircuitFacade>();
    CoDesignerManager manager(circuit_facade);
    
    // Test getting non-existent session
    auto get_result = manager.GetSession("non-existent-session");
    assert(!get_result.ok);
    assert(get_result.error_code == ErrorCode::SessionNotFound);
    
    // Test updating non-existent session
    CoDesignerSessionState dummy_session;
    dummy_session.designer_session_id = "non-existent-session";
    auto update_result = manager.UpdateSession(dummy_session);
    assert(!update_result.ok);
    assert(update_result.error_code == ErrorCode::SessionNotFound);
    
    // Test destroying non-existent session
    auto destroy_result = manager.DestroySession("non-existent-session");
    assert(!destroy_result.ok);
    assert(destroy_result.error_code == ErrorCode::SessionNotFound);
    
    std::cout << "✓ CoDesignerManager negative cases test passed\n";
}

int main() {
    std::cout << "Running CoDesigner tests...\n\n";
    
    TestCoDesignerSessionStateSerialization();
    TestCoDesignerManagerLifecycle();
    TestCoDesignerManagerNegativeCases();
    
    std::cout << "\nAll CoDesigner tests passed!\n";
    return 0;
}