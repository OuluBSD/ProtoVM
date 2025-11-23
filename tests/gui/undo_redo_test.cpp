#include <gtest/gtest.h>
#include "../../src/ProtoVM/gui/UndoRedoManager.h"
#include "../../src/ProtoVM/gui/CircuitComponent.h"
#include "../../src/ProtoVM/gui/CanvasComponent.h"

// Test fixture for undo/redo tests
class UndoRedoTest : public ::testing::Test {
protected:
    void SetUp() override {
        canvas = std::make_shared<CircuitCanvas>();
        undoMgr = std::make_unique<UndoRedoManager>(canvas.get());
    }

    void TearDown() override {
        canvas.reset();
        undoMgr.reset();
    }

    std::shared_ptr<CircuitCanvas> canvas;
    std::unique_ptr<UndoRedoManager> undoMgr;
};

// Test initial state of undo/redo manager
TEST_F(UndoRedoTest, InitialState) {
    EXPECT_FALSE(undoMgr->CanUndo());
    EXPECT_FALSE(undoMgr->CanRedo());
    EXPECT_EQ(undoMgr->GetUndoStackSize(), 0);
    EXPECT_EQ(undoMgr->GetRedoStackSize(), 0);
}

// Test adding a component and undoing it
TEST_F(UndoRedoTest, AddComponentUndo) {
    auto nandGate = std::make_shared<NandGateComponent>(50, 50);
    
    // Add component with command
    auto addCmd = std::make_unique<AddComponentCommand>(canvas.get(), nandGate);
    addCmd->Execute();
    undoMgr->PushCommand(std::move(addCmd));
    
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    EXPECT_TRUE(undoMgr->CanUndo());
    EXPECT_FALSE(undoMgr->CanRedo());
    
    // Undo the addition
    undoMgr->Undo();
    
    EXPECT_EQ(canvas->GetComponents().size(), 0);
    EXPECT_FALSE(undoMgr->CanUndo());
    EXPECT_TRUE(undoMgr->CanRedo());
    
    // Redo the addition
    undoMgr->Redo();
    
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    EXPECT_TRUE(undoMgr->CanUndo());
    EXPECT_FALSE(undoMgr->CanRedo());
}

// Test deleting a component and undoing it
TEST_F(UndoRedoTest, DeleteComponentUndo) {
    auto nandGate = std::make_shared<NandGateComponent>(50, 50);
    canvas->AddComponent(nandGate);
    
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    
    // Delete component with command
    auto deleteCmd = std::make_unique<DeleteComponentCommand>(canvas.get(), nandGate);
    deleteCmd->Execute();
    undoMgr->PushCommand(std::move(deleteCmd));
    
    EXPECT_EQ(canvas->GetComponents().size(), 0);
    EXPECT_TRUE(undoMgr->CanUndo());
    
    // Undo the deletion
    undoMgr->Undo();
    
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    EXPECT_FALSE(undoMgr->CanUndo());
    EXPECT_TRUE(undoMgr->CanRedo());
}

// Test moving a component and undoing it
TEST_F(UndoRedoTest, MoveComponentUndo) {
    auto nandGate = std::make_shared<NandGateComponent>(50, 50);
    canvas->AddComponent(nandGate);
    
    int originalX = nandGate->GetX();
    int originalY = nandGate->GetY();
    
    EXPECT_EQ(originalX, 50);
    EXPECT_EQ(originalY, 50);
    
    // Move component with command
    auto moveCmd = std::make_unique<MoveComponentCommand>(nandGate, 100, 100);
    moveCmd->Execute();
    undoMgr->PushCommand(std::move(moveCmd));
    
    EXPECT_EQ(nandGate->GetX(), 100);
    EXPECT_EQ(nandGate->GetY(), 100);
    EXPECT_TRUE(undoMgr->CanUndo());
    
    // Undo the move
    undoMgr->Undo();
    
    EXPECT_EQ(nandGate->GetX(), originalX);
    EXPECT_EQ(nandGate->GetY(), originalY);
    EXPECT_FALSE(undoMgr->CanUndo());
    EXPECT_TRUE(undoMgr->CanRedo());
    
    // Redo the move
    undoMgr->Redo();
    
    EXPECT_EQ(nandGate->GetX(), 100);
    EXPECT_EQ(nandGate->GetY(), 100);
    EXPECT_TRUE(undoMgr->CanUndo());
    EXPECT_FALSE(undoMgr->CanRedo());
}

// Test connecting wires and undoing it
TEST_F(UndoRedoTest, ConnectWireUndo) {
    auto nandGate1 = std::make_shared<NandGateComponent>(50, 50);
    auto nandGate2 = std::make_shared<NandGateComponent>(150, 50);
    
    canvas->AddComponent(nandGate1);
    canvas->AddComponent(nandGate2);
    
    // Connect with a command
    auto connectCmd = std::make_unique<ConnectCommand>(canvas.get(), 
        nandGate1->GetOutputPin(0), nandGate2->GetInputPin(0));
    connectCmd->Execute();
    undoMgr->PushCommand(std::move(connectCmd));
    
    EXPECT_TRUE(undoMgr->CanUndo());
    
    // Verify connection exists
    // This would depend on the actual implementation of connection
    // For now, we just test the undo/redo mechanism itself
    
    // Undo the connection
    undoMgr->Undo();
    
    EXPECT_FALSE(undoMgr->CanUndo());
    EXPECT_TRUE(undoMgr->CanRedo());
    
    // Redo the connection
    undoMgr->Redo();
    
    EXPECT_TRUE(undoMgr->CanUndo());
    EXPECT_FALSE(undoMgr->CanRedo());
}

// Test multiple undo operations
TEST_F(UndoRedoTest, MultipleUndo) {
    auto nandGate1 = std::make_shared<NandGateComponent>(50, 50);
    auto nandGate2 = std::make_shared<NandGateComponent>(100, 100);
    auto nandGate3 = std::make_shared<NandGateComponent>(150, 150);
    
    // Add first component
    auto cmd1 = std::make_unique<AddComponentCommand>(canvas.get(), nandGate1);
    cmd1->Execute();
    undoMgr->PushCommand(std::move(cmd1));
    
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    
    // Add second component
    auto cmd2 = std::make_unique<AddComponentCommand>(canvas.get(), nandGate2);
    cmd2->Execute();
    undoMgr->PushCommand(std::move(cmd2));
    
    EXPECT_EQ(canvas->GetComponents().size(), 2);
    
    // Add third component
    auto cmd3 = std::make_unique<AddComponentCommand>(canvas.get(), nandGate3);
    cmd3->Execute();
    undoMgr->PushCommand(std::move(cmd3));
    
    EXPECT_EQ(canvas->GetComponents().size(), 3);
    
    // Undo all three additions
    undoMgr->Undo();  // Remove third
    EXPECT_EQ(canvas->GetComponents().size(), 2);
    
    undoMgr->Undo();  // Remove second
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    
    undoMgr->Undo();  // Remove first
    EXPECT_EQ(canvas->GetComponents().size(), 0);
    
    // Now redo them
    undoMgr->Redo();  // Add first
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    
    undoMgr->Redo();  // Add second
    EXPECT_EQ(canvas->GetComponents().size(), 2);
    
    undoMgr->Redo();  // Add third
    EXPECT_EQ(canvas->GetComponents().size(), 3);
}

// Test clear redo stack after new command
TEST_F(UndoRedoTest, ClearRedoStack) {
    auto nandGate1 = std::make_shared<NandGateComponent>(50, 50);
    auto nandGate2 = std::make_shared<NandGateComponent>(100, 100);
    
    // Add first component
    auto cmd1 = std::make_unique<AddComponentCommand>(canvas.get(), nandGate1);
    cmd1->Execute();
    undoMgr->PushCommand(std::move(cmd1));
    
    // Add second component
    auto cmd2 = std::make_unique<AddComponentCommand>(canvas.get(), nandGate2);
    cmd2->Execute();
    undoMgr->PushCommand(std::move(cmd2));
    
    EXPECT_EQ(canvas->GetComponents().size(), 2);
    EXPECT_EQ(undoMgr->GetUndoStackSize(), 2);
    EXPECT_EQ(undoMgr->GetRedoStackSize(), 0);
    
    // Undo once
    undoMgr->Undo();
    EXPECT_EQ(canvas->GetComponents().size(), 1);
    EXPECT_EQ(undoMgr->GetUndoStackSize(), 1);
    EXPECT_EQ(undoMgr->GetRedoStackSize(), 1);
    
    // Perform a new operation (not redo), which should clear the redo stack
    auto nandGate3 = std::make_shared<NandGateComponent>(150, 150);
    auto cmd3 = std::make_unique<AddComponentCommand>(canvas.get(), nandGate3);
    cmd3->Execute();
    undoMgr->PushCommand(std::move(cmd3));
    
    EXPECT_EQ(canvas->GetComponents().size(), 2);
    EXPECT_EQ(undoMgr->GetUndoStackSize(), 2);
    EXPECT_EQ(undoMgr->GetRedoStackSize(), 0);  // Redo stack should be cleared
}

// Test undo/redo limits
TEST_F(UndoRedoTest, UndoRedoLimits) {
    // Set a small limit to test
    undoMgr->SetMaxHistorySize(3);
    
    std::vector<std::shared_ptr<CircuitComponent>> gates;
    std::vector<std::unique_ptr<Command>> commands;
    
    // Add more components than the limit
    for (int i = 0; i < 5; i++) {
        auto gate = std::make_shared<NandGateComponent>(50 + i * 20, 50);
        auto cmd = std::make_unique<AddComponentCommand>(canvas.get(), gate);
        cmd->Execute();
        undoMgr->PushCommand(std::move(cmd));
        gates.push_back(gate);
    }
    
    // Only the last 3 operations should be in the history
    EXPECT_EQ(undoMgr->GetUndoStackSize(), 3);
    
    // Undo all possible operations
    for (int i = 0; i < 3; i++) {
        undoMgr->Undo();
    }
    
    // Now there should be no more operations to undo
    EXPECT_FALSE(undoMgr->CanUndo());
    
    // Redo all operations
    for (int i = 0; i < 3; i++) {
        undoMgr->Redo();
    }
    
    // Should have 3 components (those within the history limit)
    EXPECT_EQ(canvas->GetComponents().size(), 3);
}