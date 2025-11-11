#!/bin/bash

# Test script to run the 4004 CPU simulation for more ticks to ensure program execution

echo "Testing 4004 CPU character output with extended execution..."
echo "This will run the simulation with 100 ticks to ensure full program execution"

# Build the project if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "Building project..."
    ./build.sh
fi

# Run with more ticks to ensure full execution
echo "Running simulation for 100 ticks to ensure program completion..."
./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 100

exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo "Simulation completed successfully."
else
    echo "Simulation failed with exit code $exit_code."
fi

exit $exit_code