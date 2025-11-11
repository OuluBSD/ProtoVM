#!/bin/bash

# Test script to run the 4004 CPU output and instruction functionality tests

echo "Running 4004 CPU Output Unit Tests..."
echo ""

# Build the project if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "ProtoVM executable not found. Building project..."
    ./build.sh
fi

echo "Executing: ./build/ProtoVM test4004output"
echo ""

# Run the 4004 output tests
./build/ProtoVM test4004output
output_exit_code=$?

echo ""
if [ $output_exit_code -eq 0 ]; then
    echo "4004 Output Tests completed successfully!"
else
    echo "4004 Output Tests failed with exit code $output_exit_code."
fi

echo ""
echo "Running 4004 CPU Instruction Unit Tests..."
echo ""

echo "Executing: ./build/ProtoVM test4004instructions"
echo ""

# Run the 4004 instruction tests
./build/ProtoVM test4004instructions
instruction_exit_code=$?

echo ""
if [ $instruction_exit_code -eq 0 ]; then
    echo "4004 Instruction Tests completed successfully!"
else
    echo "4004 Instruction Tests failed with exit code $instruction_exit_code."
fi

echo ""
echo "Now running original 4004 character output program for comparison:"
echo ""

# Also run the original program for comparison
./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 50

echo ""
echo "4004 Test Script completed."

# Exit with overall status (0 if all tests passed, non-zero if any failed)
if [ $output_exit_code -eq 0 ] && [ $instruction_exit_code -eq 0 ]; then
    exit 0
else
    exit 1
fi