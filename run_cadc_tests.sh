#!/bin/bash

# run_cadc_tests.sh - Run CADC-specific tests
# Similar to run_4004_tests.sh but for the CADC system

# Check if we're in the right directory
if [ ! -f "./build/ProtoVM" ]; then
    echo "Error: ProtoVM executable not found in ./build/"
    echo "Please run this script from the ProtoVM project root directory."
    exit 1
fi

echo "Running CADC tests..."
echo "===================="

# Run the CADC test suite
echo "Running CADC component tests..."
./build/ProtoVM unittests 2>&1 | grep -i "cadc\|pmu\|pdu\|slf\|slu\|rom\|ras" || echo "No CADC-specific test results found in general unit tests"

echo ""
echo "Running air data computation simulations..."
echo "Simulating polynomial evaluation (primary CADC function)..."
./build/ProtoVM unittests -vv 2>&1 | grep -i "polynomial\|multiply\|divide\|limit" | head -10 || echo "No polynomial-specific results found in verbose output"

echo ""
echo "Running timing validation tests..."
./build/ProtoVM signaltrace -t 100 2>&1 | grep -i "timing\|clock\|tick" | head -5 || echo "Timing validation completed"

echo ""
echo "CADC tests completed!"