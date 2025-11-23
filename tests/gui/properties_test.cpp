#include <gtest/gtest.h>
#include "../../src/ProtoVM/gui/ComponentPropertyPanel.h"
#include "../../src/ProtoVM/gui/CircuitComponent.h"
#include "../../src/ProtoVM/gui/CanvasComponent.h"

// Test fixture for property panel tests
class PropertiesTest : public ::testing::Test {
protected:
    void SetUp() override {
        nandGate = std::make_shared<NandGateComponent>(100, 100);
        propPanel = std::make_unique<ComponentPropertyPanel>();
    }

    void TearDown() override {
        nandGate.reset();
        propPanel.reset();
    }

    std::shared_ptr<NandGateComponent> nandGate;
    std::unique_ptr<ComponentPropertyPanel> propPanel;
};

// Test initial property panel state
TEST_F(PropertiesTest, InitialState) {
    EXPECT_EQ(propPanel->GetComponent(), nullptr);
    EXPECT_EQ(propPanel->GetPropertyCount(), 0);
}

// Test loading properties for a NAND gate
TEST_F(PropertiesTest, LoadNandGateProperties) {
    propPanel->LoadComponentProperties(nandGate);
    
    EXPECT_EQ(propPanel->GetComponent(), nandGate.get());
    // Basic properties should include position, name, etc.
    EXPECT_GT(propPanel->GetPropertyCount(), 0);
}

// Test property modification for name
TEST_F(PropertiesTest, ModifyComponentName) {
    propPanel->LoadComponentProperties(nandGate);
    
    std::string originalName = nandGate->GetName();
    std::string newName = "MyNAND";
    
    EXPECT_NE(originalName, newName);
    
    // Modify the name property
    propPanel->SetProperty("name", newName);
    
    EXPECT_EQ(nandGate->GetName(), newName);
}

// Test property modification for position
TEST_F(PropertiesTest, ModifyComponentPosition) {
    propPanel->LoadComponentProperties(nandGate);
    
    int originalX = nandGate->GetX();
    int originalY = nandGate->GetY();
    
    int newX = 200;
    int newY = 200;
    
    EXPECT_NE(originalX, newX);
    EXPECT_NE(originalY, newY);
    
    // Modify position properties
    propPanel->SetProperty("x", std::to_string(newX));
    propPanel->SetProperty("y", std::to_string(newY));
    
    EXPECT_EQ(nandGate->GetX(), newX);
    EXPECT_EQ(nandGate->GetY(), newY);
}

// Test loading properties for different component types
TEST_F(PropertiesTest, LoadDifferentComponentProperties) {
    auto norGate = std::make_shared<NorGateComponent>(150, 150);
    auto notGate = std::make_shared<NotGateComponent>(200, 200);
    
    // Load properties for NOR gate
    propPanel->LoadComponentProperties(norGate);
    EXPECT_EQ(propPanel->GetComponent(), norGate.get());
    EXPECT_GT(propPanel->GetPropertyCount(), 0);
    
    // Load properties for NOT gate
    propPanel->LoadComponentProperties(notGate);
    EXPECT_EQ(propPanel->GetComponent(), notGate.get());
    EXPECT_GT(propPanel->GetPropertyCount(), 0);
    
    // Load back to NAND gate
    propPanel->LoadComponentProperties(nandGate);
    EXPECT_EQ(propPanel->GetComponent(), nandGate.get());
    EXPECT_GT(propPanel->GetPropertyCount(), 0);
}

// Test property validation
TEST_F(PropertiesTest, PropertyValidation) {
    propPanel->LoadComponentProperties(nandGate);
    
    // Test setting valid position values
    bool result1 = propPanel->SetProperty("x", "300");
    EXPECT_TRUE(result1);
    EXPECT_EQ(nandGate->GetX(), 300);
    
    bool result2 = propPanel->SetProperty("y", "300");
    EXPECT_TRUE(result2);
    EXPECT_EQ(nandGate->GetY(), 300);
    
    // Test setting invalid (non-numeric) position values
    // This behavior would depend on the implementation
    // For now, just ensure it doesn't crash
    EXPECT_NO_THROW({
        propPanel->SetProperty("x", "invalid");
    });
}

// Test property retrieval
TEST_F(PropertiesTest, PropertyRetrieval) {
    nandGate->SetName("TestNAND");
    nandGate->SetPosition(120, 120);
    
    propPanel->LoadComponentProperties(nandGate);
    
    std::string nameValue = propPanel->GetProperty("name");
    std::string xValue = propPanel->GetProperty("x");
    std::string yValue = propPanel->GetProperty("y");
    
    EXPECT_EQ(nameValue, "TestNAND");
    EXPECT_EQ(xValue, "120");
    EXPECT_EQ(yValue, "120");
}

// Test updating properties after external changes
TEST_F(PropertiesTest, UpdateAfterExternalChange) {
    propPanel->LoadComponentProperties(nandGate);
    
    // Change component properties externally
    nandGate->SetName("ExternallyChanged");
    nandGate->SetPosition(250, 250);
    
    // The property panel should reflect these changes
    // This would depend on implementation - direct access or refresh required
    
    // If the property system directly accesses component properties:
    EXPECT_EQ(propPanel->GetProperty("name"), "ExternallyChanged");
    EXPECT_EQ(propPanel->GetProperty("x"), "250");
    EXPECT_EQ(propPanel->GetProperty("y"), "250");
}

// Test component type property
TEST_F(PropertiesTest, ComponentTypeProperty) {
    propPanel->LoadComponentProperties(nandGate);
    
    std::string typeValue = propPanel->GetProperty("type");
    
    // The type should reflect the actual component type
    EXPECT_FALSE(typeValue.empty());
    EXPECT_EQ(typeValue, "NAND");
}

// Test clearing property panel
TEST_F(PropertiesTest, ClearProperties) {
    propPanel->LoadComponentProperties(nandGate);
    
    EXPECT_EQ(propPanel->GetComponent(), nandGate.get());
    EXPECT_GT(propPanel->GetPropertyCount(), 0);
    
    // Clear the panel
    propPanel->Clear();
    
    EXPECT_EQ(propPanel->GetComponent(), nullptr);
    EXPECT_EQ(propPanel->GetPropertyCount(), 0);
}