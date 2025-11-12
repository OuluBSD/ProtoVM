#!/bin/bash

# Test script for CADC system functionality (not character output like 4004)
# The F-14 CADC computes air data parameters (altitude, speed, etc.) not characters

# Change to the script's directory parent (project root) to run properly
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "Testing CADC system functionality..."
echo "Note: CADC computes air data parameters, not character output like 4004"

# Build the project if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "Building project..."
    ./build.sh
fi

# Run the CADC simulation to test pipeline operations
echo "Running CADC simulation for 100 ticks..."
./build/ProtoVM cadc --ticks 100

exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo "CADC simulation completed successfully."
    echo "Note: CADC computes air data (altitude, speed, etc.) not characters like 4004"
else
    echo "CADC simulation failed with exit code $exit_code."
fi

exit $exit_code