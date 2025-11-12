#!/bin/bash

# Test script to run the CADC component and functionality tests

echo "Running CADC Component Tests..."
echo ""

# Build the project if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "ProtoVM executable not found. Building project..."
    ./build.sh
fi

echo "Executing: ./build/ProtoVM cadc (CADC system test)"
echo ""

# Run the CADC system test
./build/ProtoVM cadc
cadc_exit_code=$?

echo ""
if [ $cadc_exit_code -eq 0 ]; then
    echo "CADC System Test completed successfully!"
    echo "Demonstrated: PMU, PDU, SLF pipeline operation"
else
    echo "CADC System Test failed with exit code $cadc_exit_code."
fi

echo ""
echo "Running general unit tests to check for CADC-specific results..."
echo ""

# Also run general unit tests to see if CADC components are included
./build/ProtoVM unittests -vv 2>&1 | grep -i "cadc\|pmu\|pdu\|slf\|polynomial" | head -10 || echo "No CADC-specific test results in general unit tests yet"

echo ""
echo "CADC Test Script completed."

# Exit with overall status
exit $cadc_exit_code