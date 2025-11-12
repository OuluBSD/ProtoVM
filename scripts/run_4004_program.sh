#!/bin/bash

# Script to run the 4004 CPU simulation with our character output program

# Change to the script's directory parent (project root) to run properly\nSCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"\ncd "$SCRIPT_DIR/.."
echo "Running 4004 CPU simulation with character output program..."
echo "Loading program that outputs 'A' character at address 0x0000"

# Build the project first if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "ProtoVM executable not found. Building project..."
    ./build.sh
fi

# Run the simulation with our binary
echo "Executing: ./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 20"
./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 20

# Capture the exit code
exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo "Simulation completed."
else
    echo "Simulation failed with exit code $exit_code."
fi

exit $exit_code