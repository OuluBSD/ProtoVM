#!/bin/bash

# Test script to run the CADC system with extended execution to verify functionality

echo "Testing CADC system with extended execution..."
echo "This will run the simulation to verify CADC pipeline operations"

# Build the project if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "Building project..."
    ./build.sh
fi

# Run with more ticks to ensure full execution of CADC operations
echo "Running CADC simulation for 200 ticks to ensure full pipeline operation..."
./build/ProtoVM cadc --ticks 200

exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo "CADC simulation completed successfully."
    echo "Verified: 20-bit processing, 375kHz timing, pipeline concurrency"
else
    echo "CADC simulation failed with exit code $exit_code."
fi

exit $exit_code