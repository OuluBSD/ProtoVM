#ifndef _ProtoVM_TestVectorGenerator_h_
#define _ProtoVM_TestVectorGenerator_h_

#include "ProtoVM.h"

// Structure to represent a single test vector
struct TestVector : Moveable<TestVector> {
    Vector<byte> inputs;      // Input values for the test
    Vector<byte> expected;    // Expected output values
    String description;       // Description of what the test verifies
    int delay_cycles;         // Number of cycles to wait after applying inputs
    
    TestVector() : delay_cycles(1) {}
    
    // Helper methods
    void AddInput(byte value) { inputs.Add(value); }
    void AddExpected(byte value) { expected.Add(value); }
    int GetInputCount() const { return inputs.GetCount(); }
    int GetExpectedCount() const { return expected.GetCount(); }
};

// Structure to represent test results
struct TestResult : Moveable<TestResult> {
    String test_name;
    bool passed;
    String error_message;
    Vector<byte> actual_outputs;  // Actual outputs observed during test
    Vector<byte> expected_outputs; // Expected outputs for the test
    int test_vector_index;
    
    TestResult() : passed(false), test_vector_index(-1) {}
};

// Test vector generator and runner for comprehensive verification
class TestVectorGenerator {
private:
    Vector<TestVector> test_vectors;
    Vector<TestResult> test_results;
    String component_name;    // Name of the component being tested
    String test_suite_name;   // Name of the test suite
    
public:
    TestVectorGenerator(const String& comp_name = "", const String& suite_name = "");
    
    // Methods to create test vectors
    TestVector& AddTestVector(const String& description = "");
    TestVector& AddBasicFunctionalTest();
    TestVector& AddEdgeCaseTest();
    TestVector& AddTimingTest();
    TestVector& AddStressTest();
    
    // Methods to run tests
    void RunAllTests(Machine& mach);
    void RunTest(Machine& mach, int vector_index);
    bool VerifyTestResult(Machine& mach, int vector_index);
    
    // Results management
    void ReportResults() const;
    const Vector<TestResult>& GetTestResults() const { return test_results; }
    int GetPassCount() const;
    int GetFailCount() const;
    double GetPassRate() const;
    
    // Common test patterns
    void GenerateAllInputCombinations(int input_width);
    void GeneratePatternTests();
    void GenerateTimingTests();
    
    // Helper methods
    void ClearTests();
    void ClearResults();
    const Vector<TestVector>& GetTestVectors() const { return test_vectors; }
    
    // Setters
    void SetComponentName(const String& name) { component_name = name; }
    void SetTestSuiteName(const String& name) { test_suite_name = name; }
};

// Specialized test generator for ALU components
class ALUTestVectorGenerator : public TestVectorGenerator {
public:
    ALUTestVectorGenerator() : TestVectorGenerator("ALU", "ALU Verification Tests") {}
    
    // ALU-specific test creation methods
    TestVector& AddAdditionTest(byte a, byte b);
    TestVector& AddSubtractionTest(byte a, byte b);
    TestVector& AddLogicalTest(byte a, byte b, ALU::Operation op);
    TestVector& AddFlagTest(byte a, byte b, ALU::Operation op);
    TestVector& AddOverflowTest();
    TestVector& AddZeroTest();
    TestVector& AddNegativeTest();
    
    // Generate comprehensive test suite for ALU
    void GenerateComprehensiveTests();
};

// Specialized test generator for memory components
class MemoryTestVectorGenerator : public TestVectorGenerator {
public:
    MemoryTestVectorGenerator() : TestVectorGenerator("Memory", "Memory Verification Tests") {}
    
    // Memory-specific test creation methods
    TestVector& AddWriteReadTest(int addr, byte data);
    TestVector& AddAddressTest(int addr, byte data);
    TestVector& AddEnableTest();
    TestVector& AddMultipleLocationTest();
    TestVector& AddRefreshTest();
    
    // Generate comprehensive test suite for memory
    void GenerateComprehensiveTests();
};

// Specialized test generator for CPU components
class CPUTestVectorGenerator : public TestVectorGenerator {
public:
    CPUTestVectorGenerator() : TestVectorGenerator("CPU", "CPU Verification Tests") {}
    
    // CPU-specific test creation methods
    TestVector& AddInstructionTest(byte opcode, Vector<byte> operands, Vector<byte> expected_regs);
    TestVector& AddFlagTest(byte opcode, Vector<byte> operands, bool expected_carry, bool expected_zero, bool expected_negative);
    TestVector& AddRegisterTest();
    TestVector& AddBranchTest();
    
    // Generate comprehensive test suite for CPU
    void GenerateComprehensiveTests();
};

// Verification utility class
class VerificationUtils {
public:
    // Compare two values with optional tolerance for timing-related tests
    static bool ValuesEqual(byte actual, byte expected, int tolerance = 0);
    
    // Check if signals have settled within expected time
    static bool SignalSettledWithinTime(int actual_time, int expected_time, int tolerance = 2);
    
    // Generate random test vectors
    static TestVector GenerateRandomTestVector(int input_count, int output_count);
    
    // Create exhaustive test vectors for small input spaces
    static Vector<TestVector> GenerateExhaustiveTests(int input_width);
};

#endif