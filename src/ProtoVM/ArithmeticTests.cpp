#include "ProtoVM.h"

#include <Core/Core.h>
using namespace UPP;

// Input driver component to set initial values for testing
struct InputDriver : Chip {
    bool output_value = false;
    
    InputDriver() {
        AddSource("OUTPUT").SetRequired(false);
    }
    
    void SetValue(bool value) {
        output_value = value;
    }
    
    bool Tick() override {
        SetChanged(true);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        if (type == WRITE && conn_id == 0) { // OUTPUT pin
            byte output_data = output_value ? 1 : 0;
            return dest.PutRaw(dest_conn_id, &output_data, 0, 1);
        }
        return false;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        return true;  // InputDriver doesn't accept inputs
    }
};

// Output capture component to read values during testing
struct OutputCapture : Chip {
    bool captured_value = false;
    bool value_updated = false;
    
    OutputCapture() {
        AddSink("INPUT").SetRequired(false);
    }
    
    bool GetValue() const { return captured_value; }
    bool HasValueUpdated() const { return value_updated; }
    void ClearUpdateFlag() { value_updated = false; }
    
    bool Tick() override {
        SetChanged(true);
        return true;
    }
    
    bool Process(ProcessType type, int bytes, int bits, uint16 conn_id, ElectricNodeBase& dest, uint16 dest_conn_id) override {
        // The INPUT connector is handled by PutRaw
        return true;
    }
    
    bool PutRaw(uint16 conn_id, byte* data, int data_bytes, int data_bits) override {
        if (conn_id == 0) { // INPUT pin
            bool new_value = (data && *data != 0) ? true : false;
            if (captured_value != new_value) {
                value_updated = true;
            }
            captured_value = new_value;
        }
        return true;
    }
};

// Test the FullAdder component
bool TestFullAdder() {
    Machine mach;
    Pcb& b = mach.AddPcb();
    
    // Create a FullAdder component
    FullAdder& fa = b.Add<FullAdder>("FullAdder");
    
    // Create input drivers
    InputDriver& a_driver = b.Add<InputDriver>("A_driver");
    InputDriver& b_driver = b.Add<InputDriver>("B_driver");
    InputDriver& cin_driver = b.Add<InputDriver>("Cin_driver");
    
    // Create output capture components
    OutputCapture& sum_capture = b.Add<OutputCapture>("Sum_capture");
    OutputCapture& cout_capture = b.Add<OutputCapture>("Cout_capture");
    
    try {
        // Connect inputs
        a_driver["OUTPUT"] >> fa["A"];
        b_driver["OUTPUT"] >> fa["B"];
        cin_driver["OUTPUT"] >> fa["Cin"];
        
        // Connect outputs
        fa["Sum"] >> sum_capture["INPUT"];
        fa["Cout"] >> cout_capture["INPUT"];
        
        // Test all 8 possible input combinations
        bool test_passed = true;
        
        LOG("Testing FullAdder with all input combinations:");
        for (int a_val = 0; a_val <= 1; a_val++) {
            for (int b_val = 0; b_val <= 1; b_val++) {
                for (int cin_val = 0; cin_val <= 1; cin_val++) {
                    // Set input values
                    a_driver.SetValue(a_val != 0);
                    b_driver.SetValue(b_val != 0);
                    cin_driver.SetValue(cin_val != 0);
                    
                    // Process a tick to propagate values
                    mach.Tick();
                    
                    // Calculate expected values manually
                    int sum_exp = a_val ^ b_val ^ cin_val;
                    int cout_exp = (a_val && b_val) || (cin_val && (a_val || b_val));
                    
                    // Get actual values
                    bool sum_actual = sum_capture.GetValue();
                    bool cout_actual = cout_capture.GetValue();
                    
                    // Check if actual matches expected
                    if (sum_actual != (sum_exp != 0) || cout_actual != (cout_exp != 0)) {
                        String fail_msg = "  FAILED: A=" + AsString(a_val) + ", B=" + AsString(b_val) + ", Cin=" + AsString(cin_val) 
                              + " -> Sum: actual=" + AsString((int)sum_actual) + " vs expected=" + AsString(sum_exp)
                              + ", Cout: actual=" + AsString((int)cout_actual) + " vs expected=" + AsString(cout_exp);
                        LOG(fail_msg);
                        test_passed = false;
                    } else {
                        String pass_msg = "  PASSED: A=" + AsString(a_val) + ", B=" + AsString(b_val) + ", Cin=" + AsString(cin_val) 
                              + " -> Sum=" + AsString((int)sum_actual) + ", Cout=" + AsString((int)cout_actual);
                        LOG(pass_msg);
                    }
                }
            }
        }
        
        if (test_passed) {
            LOG("FullAdder test PASSED: All 8 combinations correct");
        } else {
            LOG("FullAdder test FAILED: Some combinations incorrect");
        }
        
        return test_passed;
    }
    catch (Exc e) {
        LOG("Error in FullAdder test: " + AsString(e));
        return false;
    }
}

// Test the AdderSubtractor4Bit component
bool TestAdderSubtractor4Bit() {
    Machine mach;
    Pcb& b = mach.AddPcb();
    
    // Create a 4-bit adder/subtractor component
    AdderSubtractor4Bit& adder = b.Add<AdderSubtractor4Bit>("Adder4Bit");
    
    // Create input drivers for A (4 bits)
    InputDriver& a3_driver = b.Add<InputDriver>("A3_driver");
    InputDriver& a2_driver = b.Add<InputDriver>("A2_driver");
    InputDriver& a1_driver = b.Add<InputDriver>("A1_driver");
    InputDriver& a0_driver = b.Add<InputDriver>("A0_driver");
    
    // Create input drivers for B (4 bits)
    InputDriver& b3_driver = b.Add<InputDriver>("B3_driver");
    InputDriver& b2_driver = b.Add<InputDriver>("B2_driver");
    InputDriver& b1_driver = b.Add<InputDriver>("B1_driver");
    InputDriver& b0_driver = b.Add<InputDriver>("B0_driver");
    
    // Create input drivers for control
    InputDriver& sub_driver = b.Add<InputDriver>("SUB_driver");
    InputDriver& cin_driver = b.Add<InputDriver>("CIN_driver");
    
    // Create output capture components
    OutputCapture& s3_capture = b.Add<OutputCapture>("S3_capture");
    OutputCapture& s2_capture = b.Add<OutputCapture>("S2_capture");
    OutputCapture& s1_capture = b.Add<OutputCapture>("S1_capture");
    OutputCapture& s0_capture = b.Add<OutputCapture>("S0_capture");
    OutputCapture& cout_capture = b.Add<OutputCapture>("COUT_capture");
    
    try {
        // Connect A inputs
        a3_driver["OUTPUT"] >> adder["A3"];
        a2_driver["OUTPUT"] >> adder["A2"];
        a1_driver["OUTPUT"] >> adder["A1"];
        a0_driver["OUTPUT"] >> adder["A0"];
        
        // Connect B inputs
        b3_driver["OUTPUT"] >> adder["B3"];
        b2_driver["OUTPUT"] >> adder["B2"];
        b1_driver["OUTPUT"] >> adder["B1"];
        b0_driver["OUTPUT"] >> adder["B0"];
        
        // Connect control inputs
        sub_driver["OUTPUT"] >> adder["Sub"];
        cin_driver["OUTPUT"] >> adder["Cin"];
        
        // Connect outputs
        adder["S3"] >> s3_capture["INPUT"];
        adder["S2"] >> s2_capture["INPUT"];
        adder["S1"] >> s1_capture["INPUT"];
        adder["S0"] >> s0_capture["INPUT"];
        adder["Cout"] >> cout_capture["INPUT"];
        
        bool test_passed = true;
        
        // Test addition (SUB = 0)
        LOG("Testing 4-bit Adder/Subtractor in addition mode:");
        sub_driver.SetValue(false);  // Addition mode
        cin_driver.SetValue(false);  // No carry in
        
        // Test: A = 5 (0101), B = 3 (0011) => Sum = 8 (1000), Carry = 0
        a3_driver.SetValue(false); a2_driver.SetValue(true); a1_driver.SetValue(false); a0_driver.SetValue(true);  // A = 5
        b3_driver.SetValue(false); b2_driver.SetValue(false); b1_driver.SetValue(true); b0_driver.SetValue(true);  // B = 3
        
        mach.Tick();  // Propagate values
        
        bool s3_val = s3_capture.GetValue();
        bool s2_val = s2_capture.GetValue();
        bool s1_val = s1_capture.GetValue();
        bool s0_val = s0_capture.GetValue();
        bool cout_val = cout_capture.GetValue();
        
        if (s3_val != true || s2_val != false || s1_val != false || s0_val != false || cout_val != false) {
            String result = String(s3_val ? "1":"0") + String(s2_val ? "1":"0") + String(s1_val ? "1":"0") + String(s0_val ? "1":"0");
            LOG("  FAILED: 5 + 3 != 8, got " + result);
            test_passed = false;
        } else {
            LOG("  PASSED: 5 + 3 = 8 (1000)");
        }
        
        // Test: A = 1 (0001), B = 1 (0001) => Sum = 2 (0010), Carry = 0
        a3_driver.SetValue(false); a2_driver.SetValue(false); a1_driver.SetValue(false); a0_driver.SetValue(true);  // A = 1
        b3_driver.SetValue(false); b2_driver.SetValue(false); b1_driver.SetValue(false); b0_driver.SetValue(true);  // B = 1
        
        mach.Tick();  // Propagate values
        
        s3_val = s3_capture.GetValue();
        s2_val = s2_capture.GetValue();
        s1_val = s1_capture.GetValue();
        s0_val = s0_capture.GetValue();
        cout_val = cout_capture.GetValue();
        
        if (s3_val != false || s2_val != false || s1_val != true || s0_val != false || cout_val != false) {
            String result = String(s3_val ? "1":"0") + String(s2_val ? "1":"0") + String(s1_val ? "1":"0") + String(s0_val ? "1":"0");
            LOG("  FAILED: 1 + 1 != 2, got " + result + " carry=" + AsString((int)cout_val));
            test_passed = false;
        } else {
            LOG("  PASSED: 1 + 1 = 2 (0010)");
        }
        
        // Test subtraction (SUB = 1)
        LOG("Testing 4-bit Adder/Subtractor in subtraction mode:");
        sub_driver.SetValue(true);  // Subtraction mode
        cin_driver.SetValue(false);  // No carry in (which effectively adds 1 for two's complement)
        
        // Test: A = 5 (0101), B = 3 (0011) => A - B = 2 (0010)
        a3_driver.SetValue(false); a2_driver.SetValue(true); a1_driver.SetValue(false); a0_driver.SetValue(true);  // A = 5
        b3_driver.SetValue(false); b2_driver.SetValue(false); b1_driver.SetValue(true); b0_driver.SetValue(true);  // B = 3
        
        mach.Tick();  // Propagate values
        
        s3_val = s3_capture.GetValue();
        s2_val = s2_capture.GetValue();
        s1_val = s1_capture.GetValue();
        s0_val = s0_capture.GetValue();
        cout_val = cout_capture.GetValue();
        
        if (s3_val != false || s2_val != false || s1_val != true || s0_val != false) {  // Expected: 0010 (2)
            String result = String(s3_val ? "1":"0") + String(s2_val ? "1":"0") + String(s1_val ? "1":"0") + String(s0_val ? "1":"0");
            LOG("  FAILED: 5 - 3 != 2, got " + result + " carry=" + AsString((int)cout_val));
            test_passed = false;
        } else {
            LOG("  PASSED: 5 - 3 = 2 (0010)");
        }
        
        if (test_passed) {
            LOG("AdderSubtractor4Bit test PASSED: Addition and subtraction working correctly");
        } else {
            LOG("AdderSubtractor4Bit test FAILED: Some operations incorrect");
        }
        
        return test_passed;
    }
    catch (Exc e) {
        LOG("Error in AdderSubtractor4Bit test: " + AsString(e));
        return false;
    }
}

// Run all unit tests for arithmetic components
void RunArithmeticUnitTests(Machine& mach) {
    LOG("Running Arithmetic Components Unit Tests...");
    
    bool full_adder_passed = TestFullAdder();
    bool adder_subtractor_passed = TestAdderSubtractor4Bit();
    
    if (full_adder_passed && adder_subtractor_passed) {
        LOG("ALL ARITHMETIC UNIT TESTS PASSED!");
    } else {
        LOG("SOME ARITHMETIC UNIT TESTS FAILED!");
    }
}