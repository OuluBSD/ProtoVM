#!/bin/bash

# Script to run the CADC demonstration simulation
# Similar to run_4004_demo.sh but for the CADC system

# Change to the script's directory parent (project root) to run properly
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "Running CADC demonstration simulation..."
echo "Demonstrating F-14 Central Air Data Computer architecture"

# Build the project if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "ProtoVM executable not found. Building project..."
    ./build.sh
fi

# Run the demonstration with appropriate parameters
echo "Executing: ./build/ProtoVM cadc --ticks 100"
./build/ProtoVM cadc --ticks 100

echo "CADC demonstration completed."
echo ""
echo "CADC features demonstrated:"
echo " - 20-bit word architecture with two's complement"
echo " - 375 kHz clock frequency operation"
echo " - Pipeline concurrency with 3 processing modules"
echo " - Polynomial evaluation capabilities"
echo " - Air data computation simulation"

# Return to original directory if needed (though script will exit anyway)
cd "$SCRIPT_DIR"