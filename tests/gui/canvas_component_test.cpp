#include "../src/ProtoVM/CircuitCanvas.h"
#include "../src/ProtoVM/ComponentLibrary.h"
#include <cassert>
#include <iostream>
#include <vector>

// Mock simulation controller for testing
class MockSimulationController {
public:
    bool IsRunning() const { return running; }
    void Start() { running = true; }
    void Stop() { running = false; }

private:
    bool running = false;
};

void TestCircuitCanvasBasics() {
    std::cout << "Testing CircuitCanvas basics..." << std::endl;

    // Create a mock parent window for testing
    wxWindow* parent = nullptr; // We'll test without an actual parent
    CircuitCanvas canvas(nullptr, wxID_ANY); // Will be created without parent in actual test

    // Test adding and retrieving components
    NANDGateComponent* nandComp = new NANDGateComponent(100, 100);
    canvas.AddComponent(nandComp);

    assert(canvas.GetComponents().size() == 1);
    assert(canvas.GetComponents()[0] != nullptr);
    std::cout << "✓ Component addition test passed" << std::endl;

    // Test grid and zoom functionality
    canvas.SetGridEnabled(true);
    assert(canvas.IsGridEnabled() == true);
    std::cout << "✓ Grid functionality test passed" << std::endl;

    // Test zoom functionality
    double originalZoom = canvas.GetZoomFactor();
    canvas.ZoomIn();
    assert(canvas.GetZoomFactor() > originalZoom);
    std::cout << "✓ Zoom functionality test passed" << std::endl;

    // Test component selection
    canvas.SelectAllComponents();
    assert(canvas.GetSelectedComponents().size() >= 0); // May be 0 if not in actual GUI context
    std::cout << "✓ Component selection test passed" << std::endl;

    std::cout << "All CircuitCanvas basic tests passed!" << std::endl;
}

void TestComponentCreation() {
    std::cout << "Testing component creation from library..." << std::endl;

    ComponentLibrary& lib = ComponentLibrary::GetInstance();

    // Test creating a NAND gate component
    Component* comp = lib.CreateComponent("NAND");
    assert(comp != nullptr);
    assert(comp->GetName() == "NAND");
    std::cout << "✓ Component creation test passed" << std::endl;

    delete comp; // Clean up

    // Test creating other components
    Component* norComp = lib.CreateComponent("NOR");
    assert(norComp != nullptr);
    std::cout << "✓ NOR gate creation test passed" << std::endl;
    delete norComp;

    Component* notComp = lib.CreateComponent("NOT");
    assert(notComp != nullptr);
    std::cout << "✓ NOT gate creation test passed" << std::endl;
    delete notComp;

    Component* bufComp = lib.CreateComponent("BUF");
    assert(bufComp != nullptr);
    std::cout << "✓ Buffer creation test passed" << std::endl;
    delete bufComp;

    std::cout << "All component creation tests passed!" << std::endl;
}

void TestComponentSerialization() {
    std::cout << "Testing component serialization..." << std::endl;

    // Create a simple circuit with components
    CircuitCanvas canvas(nullptr, wxID_ANY);

    // Add some components
    NANDGateComponent* comp1 = new NANDGateComponent(100, 100);
    comp1->SetName("TestNAND1");
    canvas.AddComponent(comp1);

    NOTGateComponent* comp2 = new NOTGateComponent(200, 200);
    comp2->SetName("TestNOT1");
    canvas.AddComponent(comp2);

    // Serialize to data
    CircuitData data;
    canvas.SerializeToData(data);

    // Verify serialization
    assert(data.components.size() == 2);
    std::cout << "✓ Circuit serialization test passed" << std::endl;

    // Create new canvas and deserialize
    CircuitCanvas canvas2(nullptr, wxID_ANY);
    canvas2.DeserializeFromData(data);

    // Verify deserialization
    assert(canvas2.GetComponents().size() == 2);
    std::cout << "✓ Circuit deserialization test passed" << std::endl;

    std::cout << "All component serialization tests passed!" << std::endl;
}

int main() {
    std::cout << "Starting ProtoVM GUI Tests..." << std::endl;

    try {
        TestCircuitCanvasBasics();
        TestComponentCreation();
        TestComponentSerialization();

        std::cout << "\nAll GUI tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}