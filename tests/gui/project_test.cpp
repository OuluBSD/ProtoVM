#include "../../src/ProtoVM/CircuitSerializer.h"
#include "../../src/ProtoVM/CircuitCanvas.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <wx/wx.h>

void TestProjectSaveFunctionality() {
    std::cout << "Testing Project Save functionality..." << std::endl;

    // Create a test circuit with some components
    CircuitCanvas canvas(nullptr, wxID_ANY);

    // Add some components to the canvas
    NANDGateComponent* nand1 = new NANDGateComponent(50, 50);
    nand1->SetName("NAND1");
    canvas.AddComponent(nand1);

    NOTGateComponent* not1 = new NOTGateComponent(150, 100);
    not1->SetName("NOT1");
    canvas.AddComponent(not1);

    BufferComponent* buf1 = new BufferComponent(250, 150);
    buf1->SetName("BUF1");
    canvas.AddComponent(buf1);

    // Test serialization to data
    CircuitData circuitData;
    canvas.SerializeToData(circuitData);

    // Verify that the data contains the expected components
    assert(circuitData.components.size() == 3);
    std::cout << "✓ Circuit data contains expected number of components" << std::endl;

    // Test saving to file
    CircuitSerializer serializer;
    wxString testFilePath = "test_circuit.json";

    bool saveResult = serializer.SaveCircuit(circuitData, testFilePath);
    assert(saveResult == true);
    std::cout << "✓ Circuit saved to file successfully" << std::endl;

    // Verify the file exists
    std::ifstream fileCheck(testFilePath.ToStdString());
    assert(fileCheck.good());
    fileCheck.close();
    std::cout << "✓ Saved file exists and is readable" << std::endl;

    std::cout << "Project Save functionality test passed!" << std::endl;
}

void TestProjectLoadFunctionality() {
    std::cout << "Testing Project Load functionality..." << std::endl;

    // First, ensure we have a test file to load
    TestProjectSaveFunctionality();

    wxString testFilePath = "test_circuit.json";

    // Load the circuit from file
    CircuitSerializer serializer;
    CircuitData loadData;

    bool loadResult = serializer.LoadCircuit(testFilePath, loadData);
    assert(loadResult == true);
    std::cout << "✓ Circuit loaded from file successfully" << std::endl;

    // Verify the loaded data has the expected content
    assert(loadData.components.size() == 3);
    std::cout << "✓ Loaded circuit has expected number of components" << std::endl;

    // Create a new canvas and load the data
    CircuitCanvas newCanvas(nullptr, wxID_ANY);
    newCanvas.DeserializeFromData(loadData);

    assert(newCanvas.GetComponents().size() == 3);
    std::cout << "✓ Components correctly loaded into canvas" << std::endl;

    // Clean up test file
    wxRemoveFile(testFilePath);
    std::cout << "✓ Test file cleaned up" << std::endl;

    std::cout << "Project Load functionality test passed!" << std::endl;
}

void TestProjectSaveLoadRoundTrip() {
    std::cout << "Testing Project Save/Load Round Trip..." << std::endl;

    // Create an original circuit
    CircuitCanvas originalCanvas(nullptr, wxID_ANY);

    NANDGateComponent* nand1 = new NANDGateComponent(100, 100);
    nand1->SetName("OriginalNAND");
    originalCanvas.AddComponent(nand1);

    NOTGateComponent* not1 = new NOTGateComponent(200, 200);
    not1->SetName("OriginalNOT");
    originalCanvas.AddComponent(not1);

    // Serialize original
    CircuitData originalData;
    originalCanvas.SerializeToData(originalData);
    size_t originalComponentCount = originalData.components.size();
    
    // Save to file
    CircuitSerializer serializer;
    wxString filePath = "roundtrip_test.json";
    bool saveResult = serializer.SaveCircuit(originalData, filePath);
    assert(saveResult == true);
    std::cout << "✓ Original circuit saved for roundtrip test" << std::endl;

    // Load from file
    CircuitData loadedData;
    bool loadResult = serializer.LoadCircuit(filePath, loadedData);
    assert(loadResult == true);
    std::cout << "✓ Circuit loaded for roundtrip test" << std::endl;

    // Compare original and loaded data
    assert(loadedData.components.size() == originalComponentCount);
    std::cout << "✓ Component count preserved during roundtrip" << std::endl;

    // Create canvas from loaded data
    CircuitCanvas loadedCanvas(nullptr, wxID_ANY);
    loadedCanvas.DeserializeFromData(loadedData);
    
    assert(loadedCanvas.GetComponents().size() == originalComponentCount);
    std::cout << "✓ Component count preserved in loaded canvas" << std::endl;

    // Clean up
    wxRemoveFile(filePath);
    std::cout << "✓ Roundtrip test file cleaned up" << std::endl;

    std::cout << "Project Save/Load Round Trip test passed!" << std::endl;
}

int main() {
    std::cout << "Starting ProtoVM Project Save/Load Tests..." << std::endl;

    try {
        TestProjectSaveFunctionality();
        TestProjectLoadFunctionality(); 
        TestProjectSaveLoadRoundTrip();

        std::cout << "\nAll Project Save/Load tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}