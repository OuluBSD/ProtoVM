#!/bin/bash

# test_cadc_output.sh - Test CADC output functionality
# Similar to test_4004_output.sh but for the CADC system

# Check if we're in the right directory  
if [ ! -f "./build/ProtoVM" ]; then
    echo "Error: ProtoVM executable not found in ./build/"
    echo "Please run this script from the ProtoVM project root directory."
    exit 1
fi

echo "Testing CADC output functionality..."
echo "==================================="

# Set up temporary files
TEMP_OUTPUT=$(mktemp)
EXPECTED_OUTPUT=$(mktemp)

# Create expected output for basic CADC functionality
cat > "$EXPECTED_OUTPUT" << 'EOF'
Testing F-14 CADC System Implementation
=====================================
Created CADC system with:
- Multiply module (with PMU)
- Divide module (with PDU)
- Special Logic module (with SLF)
- System Executive Control

CADC Architecture Features:
- 20-bit word length (19 data bits + 1 sign bit)
- Two's complement representation
- 375 kHz clock frequency
- 9375 instructions per second
- Pipeline concurrency with 3 modules
- Serial data processing

Simulating air data computations...

Running simulation for 100 clock cycles...
Clock cycle 0 completed
Clock cycle 25 completed
Clock cycle 50 completed
Clock cycle 75 completed
CADC System Test Completed!
EOF

# Run the CADC system and capture output
./build/ProtoVM cadc > "$TEMP_OUTPUT" 2>&1

# Basic checks
echo "Checking for CADC-specific output..."
if grep -q "CADC\|Central Air Data Computer\|375 kHz" "$TEMP_OUTPUT"; then
    echo "✓ Found CADC-specific identifiers in output"
else
    echo "✗ Missing CADC-specific identifiers in output"
fi

if grep -q "PMU\|PDU\|SLF\|SLU\|RAS\|ROM" "$TEMP_OUTPUT"; then
    echo "✓ Found CADC component identifiers in output"
else
    echo "✗ Missing CADC component identifiers in output"
fi

if grep -q "20-bit\|pipeline\|polynomial" "$TEMP_OUTPUT"; then
    echo "✓ Found CADC architecture features in output"
else
    echo "✗ Missing CADC architecture features in output"
fi

echo ""
echo "CADC output test completed!"

# Clean up temporary files
rm -f "$TEMP_OUTPUT" "$EXPECTED_OUTPUT"