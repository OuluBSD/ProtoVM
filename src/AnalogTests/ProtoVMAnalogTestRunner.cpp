#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "ProtoVM Analog Component Tests Suite" << std::endl;
    std::cout << "=====================================" << std::endl;

    // Run the resistor test
    std::cout << "\n1. Running Resistor Test..." << std::endl;
    int resistor_test_result = std::system("./ProtoVMAnalogResistorTest");
    if (resistor_test_result == 0) {
        std::cout << "   Resistor test PASSED" << std::endl;
    } else {
        std::cout << "   Resistor test FAILED" << std::endl;
    }

    // Run the capacitor test
    std::cout << "\n2. Running Capacitor Test..." << std::endl;
    int capacitor_test_result = std::system("./ProtoVMAnalogCapacitorTest");
    if (capacitor_test_result == 0) {
        std::cout << "   Capacitor test PASSED" << std::endl;
    } else {
        std::cout << "   Capacitor test FAILED" << std::endl;
    }

    // Run the RC circuit test
    std::cout << "\n3. Running RC Circuit Test..." << std::endl;
    int rc_test_result = std::system("./ProtoVMAnalogRCTest");
    if (rc_test_result == 0) {
        std::cout << "   RC circuit test PASSED" << std::endl;
    } else {
        std::cout << "   RC circuit test FAILED" << std::endl;
    }

    // Summary
    std::cout << "\nTest Suite Summary:" << std::endl;
    std::cout << "===================" << std::endl;
    std::cout << "Resistor Test: " << (resistor_test_result == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "Capacitor Test: " << (capacitor_test_result == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "RC Circuit Test: " << (rc_test_result == 0 ? "PASS" : "FAIL") << std::endl;

    if (resistor_test_result == 0 && capacitor_test_result == 0 && rc_test_result == 0) {
        std::cout << "\nAll tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\nSome tests FAILED!" << std::endl;
        return 1;
    }
}