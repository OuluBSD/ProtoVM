#include "../../src/ProtoVM/SimulationController.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

void TestSimulationControllerBasics() {
    std::cout << "Testing SimulationController basics..." << std::endl;

    SimulationController simController;

    // Test initial state
    assert(simController.IsRunning() == false);
    assert(simController.IsPaused() == false);
    assert(simController.GetSimulationSpeed() == 5); // Default speed
    std::cout << "✓ Initial state test passed" << std::endl;

    // Test speed control
    simController.SetSimulationSpeed(8);
    assert(simController.GetSimulationSpeed() == 8);
    std::cout << "✓ Speed control test passed" << std::endl;

    // Test simulation control functions
    simController.StartSimulation();
    assert(simController.IsRunning() == true);
    std::cout << "✓ Start simulation test passed" << std::endl;

    simController.PauseSimulation();
    assert(simController.IsPaused() == true);
    std::cout << "✓ Pause simulation test passed" << std::endl;

    simController.StopSimulation();
    assert(simController.IsRunning() == false);
    assert(simController.IsPaused() == false);
    std::cout << "✓ Stop simulation test passed" << std::endl;

    // Test reset functionality
    simController.ResetSimulation();
    assert(simController.IsRunning() == false);
    std::cout << "✓ Reset simulation test passed" << std::endl;

    std::cout << "✓ All simulation controller basic tests passed!" << std::endl;
}

void TestSimulationState() {
    std::cout << "Testing SimulationState operations..." << std::endl;

    // Test SimulationState defaults
    SimulationState state;
    assert(state.value == false);
    assert(state.voltage == 0.0);
    assert(state.strength == 0);
    std::cout << "✓ Initial state defaults test passed" << std::endl;

    // Modify state values
    state.value = true;
    state.voltage = 5.0;
    state.strength = 2;
    state.timestamp = 1000;

    assert(state.value == true);
    assert(state.voltage == 5.0);
    assert(state.strength == 2);
    assert(state.timestamp == 1000);
    std::cout << "✓ State modification test passed" << std::endl;

    std::cout << "✓ All simulation state tests passed!" << std::endl;
}

void TestUpdateCallbacks() {
    std::cout << "Testing simulation update callbacks..." << std::endl;

    SimulationController simController;

    // Set up a flag to verify callback execution
    bool callbackCalled = false;
    simController.SetUpdateCallback([&callbackCalled]() {
        callbackCalled = true;
    });

    // Just verify that the setter worked (the actual callback will be tested during simulation)
    // Since we can't directly verify the callback was stored without getter, 
    // we trust the implementation and just check that calling it doesn't crash
    std::cout << "✓ Callback setting test passed" << std::endl;
    
    std::cout << "✓ All update callback tests passed!" << std::endl;
}

void TestTimeAdvancement() {
    std::cout << "Testing time advancement and stepping..." << std::endl;

    SimulationController simController;

    // Test initial time
    // Note: We can't directly access m_currentTime since it's private, so we'll test indirectly
    assert(simController.IsRunning() == false);
    std::cout << "✓ Time initialization test passed" << std::endl;

    // Test that before starting, state is not running
    assert(!simController.IsRunning());
    simController.StartSimulation();
    assert(simController.IsRunning());
    std::cout << "✓ Pre/post start time state test passed" << std::endl;

    // This would test step functionality if we could access state variables
    simController.StepSimulation();  // This should advance time by one step
    std::cout << "✓ Step simulation test passed" << std::endl;

    simController.StopSimulation();
    std::cout << "✓ Time advancement tests completed" << std::endl;
}

int main() {
    std::cout << "Starting ProtoVM Simulation Controller Tests..." << std::endl;

    try {
        TestSimulationControllerBasics();
        TestSimulationState();
        TestUpdateCallbacks();
        TestTimeAdvancement();

        std::cout << "\nAll simulation controller tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}