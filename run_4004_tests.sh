#!/bin/bash

# Test script to run the 4004 CPU output functionality tests

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

exit_code=$?

echo ""
if [ $exit_code -eq 0 ]; then
    echo "4004 Output Tests completed successfully!"
else
    echo "4004 Output Tests failed with exit code $exit_code."
fi

echo ""
echo "Now running original 4004 character output program for comparison:"
echo ""

# Also run the original program for comparison
./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 50

echo ""
echo "4004 Test Script completed."
exit $exit_code