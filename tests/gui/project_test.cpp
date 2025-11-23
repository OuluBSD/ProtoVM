#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "../../src/ProtoVM/gui/ProjectManager.h"
#include "../../src/ProtoVM/gui/CircuitComponent.h"
#include "../../src/ProtoVM/gui/CanvasComponent.h"

// Test fixture for ProjectManager tests
class ProjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code before each test
        pm = std::make_unique<ProjectManager>();
    }

    void TearDown() override {
        // Cleanup code after each test
        pm.reset();
    }

    std::unique_ptr<ProjectManager> pm;
};

// Test project creation
TEST_F(ProjectTest, CreateNewProject) {
    EXPECT_TRUE(pm->NewProject());
    EXPECT_EQ(pm->GetProjectName(), "Untitled");
    EXPECT_FALSE(pm->IsModified());
}

// Test project saving functionality
TEST_F(ProjectTest, SaveProject) {
    pm->NewProject();
    
    // Add a simple component to the project
    auto canvas = std::make_shared<CircuitCanvas>();
    auto nandGate = std::make_shared<NandGateComponent>(50, 50);
    canvas->AddComponent(nandGate);
    pm->SetCurrentCanvas(canvas);

    // Test saving project
    bool saveResult = pm->SaveProject("test_project.json");
    EXPECT_TRUE(saveResult);
    
    // Check that file was created and is not empty
    std::ifstream file("test_project.json");
    EXPECT_TRUE(file.is_open());
    
    nlohmann::json jsonData;
    file >> jsonData;
    file.close();
    
    EXPECT_FALSE(jsonData.empty());
    EXPECT_TRUE(jsonData.contains("components"));
    EXPECT_EQ(jsonData["components"].size(), 1);
    
    // Clean up test file
    std::remove("test_project.json");
}

// Test project loading functionality
TEST_F(ProjectTest, LoadProject) {
    // First, create a sample project file
    nlohmann::json sampleProject;
    sampleProject["project_name"] = "Sample Project";
    sampleProject["version"] = "1.0";
    
    auto& components = sampleProject["components"];
    nlohmann::json nandGate;
    nandGate["type"] = "NAND";
    nandGate["x"] = 100;
    nandGate["y"] = 100;
    nandGate["id"] = "nand1";
    components.push_back(nandGate);
    
    std::ofstream outFile("sample_project.json");
    outFile << sampleProject;
    outFile.close();
    
    // Load the project
    bool loadResult = pm->LoadProject("sample_project.json");
    EXPECT_TRUE(loadResult);
    
    // Verify project was loaded
    EXPECT_EQ(pm->GetProjectName(), "Sample Project");
    EXPECT_FALSE(pm->IsModified());
    
    // Clean up test file
    std::remove("sample_project.json");
}

// Test project save/load roundtrip
TEST_F(ProjectTest, SaveLoadRoundtrip) {
    pm->NewProject();
    pm->SetProjectName("Roundtrip Test");
    
    // Add components to the project
    auto canvas = std::make_shared<CircuitCanvas>();
    auto nandGate = std::make_shared<NandGateComponent>(50, 50);
    auto norGate = std::make_shared<NorGateComponent>(150, 50);
    canvas->AddComponent(nandGate);
    canvas->AddComponent(norGate);
    pm->SetCurrentCanvas(canvas);

    // Save project
    bool saveResult = pm->SaveProject("roundtrip_test.json");
    EXPECT_TRUE(saveResult);
    
    // Create a new project manager and load
    auto newPm = std::make_unique<ProjectManager>();
    bool loadResult = newPm->LoadProject("roundtrip_test.json");
    EXPECT_TRUE(loadResult);
    
    EXPECT_EQ(newPm->GetProjectName(), "Roundtrip Test");
    
    // Clean up test file
    std::remove("roundtrip_test.json");
}

// Test "Save As" functionality
TEST_F(ProjectTest, SaveAsProject) {
    pm->NewProject();
    
    // Add a component
    auto canvas = std::make_shared<CircuitCanvas>();
    auto notGate = std::make_shared<NotGateComponent>(75, 75);
    canvas->AddComponent(notGate);
    pm->SetCurrentCanvas(canvas);
    
    // Test Save As
    bool saveAsResult = pm->SaveProjectAs("save_as_test.json");
    EXPECT_TRUE(saveAsResult);
    
    // Verify file exists and has correct content
    std::ifstream file("save_as_test.json");
    EXPECT_TRUE(file.is_open());
    
    nlohmann::json jsonData;
    file >> jsonData;
    file.close();
    
    EXPECT_TRUE(jsonData.contains("components"));
    EXPECT_EQ(jsonData["components"].size(), 1);
    
    // Clean up test file
    std::remove("save_as_test.json");
}