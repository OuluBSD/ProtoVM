#include "../../src/ProtoVM/UndoRedo.h"
#include "../../src/ProtoVM/CircuitCanvas.h"
#include <cassert>
#include <iostream>

void TestUndoRedoManagerBasics() {
    std::cout << "Testing Undo/Redo Manager Basics..." << std::endl;

    UndoRedoManager manager;

    // Test initial state
    assert(manager.CanUndo() == false);
    assert(manager.CanRedo() == false);
    assert(manager.GetUndoActionName() == "None");
    assert(manager.GetRedoActionName() == "None");
    std::cout << "✓ Initial state test passed" << std::endl;

    // Create a simple command for testing
    NANDGateComponent* comp = new NANDGateComponent(100, 100);
    CircuitCanvas* canvas = new CircuitCanvas(nullptr, wxID_ANY);
    canvas->AddComponent(comp);

    // Create an add component command
    std::unique_ptr<UndoCommand> addCmd = std::make_unique<AddComponentCommand>(canvas, comp);

    // Push the command
    manager.PushCommand(std::move(addCmd));
    assert(manager.CanUndo() == true);
    assert(manager.CanRedo() == false);
    std::cout << "✓ Command push and state check test passed" << std::endl;

    // Test undo/redo capability
    assert(manager.GetUndoActionName() != "None");
    std::cout << "✓ Action name test passed" << std::endl;

    delete canvas; // Clean up for this test

    std::cout << "Undo/Redo Manager Basics tests passed!" << std::endl;
}

void TestAddComponentUndoRedo() {
    std::cout << "Testing Add Component Undo/Redo..." << std::endl;

    CircuitCanvas canvas(nullptr, wxID_ANY);
    UndoRedoManager manager;

    // Initially, canvas should be empty
    size_t initialCount = canvas.GetComponents().size();
    assert(initialCount == 0);
    std::cout << "✓ Initial canvas state test passed" << std::endl;

    // Create a component to add
    NANDGateComponent* comp = new NANDGateComponent(100, 100);
    comp->SetName("TestComponent");

    // Create and execute add command
    std::unique_ptr<UndoCommand> addCmd = std::make_unique<AddComponentCommand>(&canvas, comp);
    addCmd->Execute();

    // After adding, canvas should have one component
    assert(canvas.GetComponents().size() == 1);
    std::cout << "✓ Component addition test passed" << std::endl;

    // Push command to manager
    manager.PushCommand(std::move(addCmd));

    // Now undo the add
    manager.Undo();
    assert(canvas.GetComponents().size() == 0);
    std::cout << "✓ Undo add component test passed" << std::endl;

    // Now redo the add
    manager.Redo();
    assert(canvas.GetComponents().size() == 1);
    std::cout << "✓ Redo add component test passed" << std::endl;

    std::cout << "Add Component Undo/Redo tests passed!" << std::endl;
}

void TestDeleteComponentUndoRedo() {
    std::cout << "Testing Delete Component Undo/Redo..." << std::endl;

    CircuitCanvas canvas(nullptr, wxID_ANY);
    UndoRedoManager manager;

    // Add a component first
    NANDGateComponent* comp = new NANDGateComponent(100, 100);
    comp->SetName("ToDelete");
    canvas.AddComponent(comp);
    assert(canvas.GetComponents().size() == 1);
    std::cout << "✓ Component preparation test passed" << std::endl;

    // Create and execute delete command
    std::unique_ptr<UndoCommand> deleteCmd = std::make_unique<DeleteComponentCommand>(&canvas, comp);
    deleteCmd->Execute();

    // After deletion, canvas should be empty
    assert(canvas.GetComponents().size() == 0);
    std::cout << "✓ Component deletion test passed" << std::endl;

    // Push command to manager
    manager.PushCommand(std::move(deleteCmd));

    // Now undo the deletion
    manager.Undo();
    assert(canvas.GetComponents().size() == 1);
    std::cout << "✓ Undo delete component test passed" << std::endl;

    // Now redo the deletion
    manager.Redo();
    assert(canvas.GetComponents().size() == 0);
    std::cout << "✓ Redo delete component test passed" << std::endl;

    std::cout << "Delete Component Undo/Redo tests passed!" << std::endl;
}

void TestMultipleUndoRedo() {
    std::cout << "Testing Multiple Undo/Redo Operations..." << std::endl;

    CircuitCanvas canvas(nullptr, wxID_ANY);
    UndoRedoManager manager;

    // Add several components with undo/redo support
    std::vector<NANDGateComponent*> components;
    for (int i = 0; i < 3; i++) {
        NANDGateComponent* comp = new NANDGateComponent(50 + i*50, 50 + i*50);
        std::string name = "Comp" + std::to_string(i);
        comp->SetName(name);

        // Execute add command
        std::unique_ptr<UndoCommand> addCmd = std::make_unique<AddComponentCommand>(&canvas, comp);
        addCmd->Execute();
        manager.PushCommand(std::move(addCmd));
        components.push_back(comp);
    }

    // Verify all components were added
    assert(canvas.GetComponents().size() == 3);
    std::cout << "✓ Multiple additions test passed" << std::endl;

    // Undo all additions one by one
    for (int i = 0; i < 3; i++) {
        size_t expectedSize = 3 - i - 1;
        manager.Undo();
        assert(canvas.GetComponents().size() == expectedSize);
    }
    assert(canvas.GetComponents().size() == 0);
    std::cout << "✓ Multiple undos test passed" << std::endl;

    // Redo all additions one by one
    for (int i = 0; i < 3; i++) {
        manager.Redo();
        assert(canvas.GetComponents().size() == i + 1);
    }
    assert(canvas.GetComponents().size() == 3);
    std::cout << "✓ Multiple redos test passed" << std::endl;

    std::cout << "Multiple Undo/Redo tests passed!" << std::endl;
}

void TestUndoRedoStateManagement() {
    std::cout << "Testing Undo/Redo State Management..." << std::endl;

    CircuitCanvas canvas(nullptr, wxID_ANY);
    UndoRedoManager manager;

    // Initially no undo/redo possible
    assert(manager.CanUndo() == false);
    assert(manager.CanRedo() == false);
    std::cout << "✓ Initial state management test passed" << std::endl;

    // Add a component
    NANDGateComponent* comp = new NANDGateComponent(100, 100);
    std::unique_ptr<UndoCommand> addCmd = std::make_unique<AddComponentCommand>(&canvas, comp);
    addCmd->Execute();
    manager.PushCommand(std::move(addCmd));

    // Now we should be able to undo but not redo
    assert(manager.CanUndo() == true);
    assert(manager.CanRedo() == false);
    std::cout << "✓ Post-add state management test passed" << std::endl;

    // Undo the command
    manager.Undo();
    // Should be able to redo now, but not undo the original
    assert(manager.CanUndo() == false);
    assert(manager.CanRedo() == true);
    std::cout << "✓ Post-undo state management test passed" << std::endl;

    // Redo the command
    manager.Redo();
    // Should be able to undo again, but not redo the original
    assert(manager.CanUndo() == true);
    assert(manager.CanRedo() == false);
    std::cout << "✓ Post-redo state management test passed" << std::endl;

    std::cout << "Undo/Redo State Management tests passed!" << std::endl;
}

int main() {
    std::cout << "Starting ProtoVM Undo/Redo Tests..." << std::endl;

    try {
        TestUndoRedoManagerBasics();
        TestAddComponentUndoRedo();
        TestDeleteComponentUndoRedo();
        TestMultipleUndoRedo();
        TestUndoRedoStateManagement();

        std::cout << "\nAll Undo/Redo tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}