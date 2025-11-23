#include <gtest/gtest.h>
#include "../../src/ProtoVM/gui/CircuitComponent.h"
#include "../../src/ProtoVM/gui/CanvasComponent.h"
#include "../../src/ProtoVM/gui/SelectionManager.h"

// Test fixture for selection tests
class SelectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        canvas = std::make_shared<CircuitCanvas>();
        selMgr = std::make_unique<SelectionManager>(canvas.get());
    }

    void TearDown() override {
        canvas.reset();
        selMgr.reset();
    }

    std::shared_ptr<CircuitCanvas> canvas;
    std::unique_ptr<SelectionManager> selMgr;
};

// Test adding single component to selection
TEST_F(SelectionTest, SelectSingleComponent) {
    auto nandGate = std::make_shared<NandGateComponent>(100, 100);
    canvas->AddComponent(nandGate);
    
    selMgr->SelectComponent(nandGate);
    
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 1);
    EXPECT_TRUE(selMgr->IsSelected(nandGate));
}

// Test deselecting component
TEST_F(SelectionTest, DeselectComponent) {
    auto nandGate = std::make_shared<NandGateComponent>(100, 100);
    canvas->AddComponent(nandGate);
    
    selMgr->SelectComponent(nandGate);
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 1);
    
    selMgr->DeselectComponent(nandGate);
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 0);
    EXPECT_FALSE(selMgr->IsSelected(nandGate));
}

// Test selecting multiple components
TEST_F(SelectionTest, SelectMultipleComponents) {
    auto nandGate = std::make_shared<NandGateComponent>(100, 100);
    auto norGate = std::make_shared<NorGateComponent>(150, 150);
    auto notGate = std::make_shared<NotGateComponent>(200, 200);
    
    canvas->AddComponent(nandGate);
    canvas->AddComponent(norGate);
    canvas->AddComponent(notGate);
    
    selMgr->SelectComponent(nandGate);
    selMgr->SelectComponent(norGate);
    selMgr->SelectComponent(notGate);
    
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 3);
    EXPECT_TRUE(selMgr->IsSelected(nandGate));
    EXPECT_TRUE(selMgr->IsSelected(norGate));
    EXPECT_TRUE(selMgr->IsSelected(notGate));
}

// Test clearing all selections
TEST_F(SelectionTest, ClearSelections) {
    auto nandGate = std::make_shared<NandGateComponent>(100, 100);
    auto norGate = std::make_shared<NorGateComponent>(150, 150);
    
    canvas->AddComponent(nandGate);
    canvas->AddComponent(norGate);
    
    selMgr->SelectComponent(nandGate);
    selMgr->SelectComponent(norGate);
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 2);
    
    selMgr->ClearSelections();
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 0);
    EXPECT_FALSE(selMgr->IsSelected(nandGate));
    EXPECT_FALSE(selMgr->IsSelected(norGate));
}

// Test rectangle selection
TEST_F(SelectionTest, SelectInRectangle) {
    auto nandGate = std::make_shared<NandGateComponent>(50, 50);
    auto norGate = std::make_shared<NorGateComponent>(150, 150);
    auto notGate = std::make_shared<NotGateComponent>(250, 250);
    
    canvas->AddComponent(nandGate);
    canvas->AddComponent(norGate);
    canvas->AddComponent(notGate);
    
    // Select components within rectangle (0,0) to (100,100)
    // This should only select nandGate at (50,50)
    selMgr->SelectInRectangle(0, 0, 100, 100);
    
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 1);
    EXPECT_TRUE(selMgr->IsSelected(nandGate));
    EXPECT_FALSE(selMgr->IsSelected(norGate));
    EXPECT_FALSE(selMgr->IsSelected(notGate));
}

// Test selection of components by type
TEST_F(SelectionTest, SelectByType) {
    auto nandGate1 = std::make_shared<NandGateComponent>(50, 50);
    auto nandGate2 = std::make_shared<NandGateComponent>(100, 100);
    auto norGate = std::make_shared<NorGateComponent>(150, 150);
    
    canvas->AddComponent(nandGate1);
    canvas->AddComponent(nandGate2);
    canvas->AddComponent(norGate);
    
    // Select all NAND gates
    selMgr->SelectByType("NAND");
    
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 2);
    EXPECT_TRUE(selMgr->IsSelected(nandGate1));
    EXPECT_TRUE(selMgr->IsSelected(nandGate2));
    EXPECT_FALSE(selMgr->IsSelected(norGate));
}

// Test selection after movement operations
TEST_F(SelectionTest, SelectionAfterMove) {
    auto nandGate = std::make_shared<NandGateComponent>(100, 100);
    canvas->AddComponent(nandGate);
    
    selMgr->SelectComponent(nandGate);
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 1);
    
    // Simulate moving the component
    nandGate->SetPosition(200, 200);
    
    // Selection should still be valid after movement
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 1);
    EXPECT_TRUE(selMgr->IsSelected(nandGate));
}

// Test if selection works correctly with deletion
TEST_F(SelectionTest, SelectionAfterDeletion) {
    auto nandGate = std::make_shared<NandGateComponent>(100, 100);
    canvas->AddComponent(nandGate);
    
    selMgr->SelectComponent(nandGate);
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 1);
    
    // Remove component from canvas
    canvas->RemoveComponent(nandGate);
    
    // After component removal, it should no longer be selected
    EXPECT_EQ(selMgr->GetSelectedComponents().size(), 0);
}