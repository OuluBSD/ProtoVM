#!/bin/bash

# Script to run unittests with dummy putchar functions that write directly to stdout
# Executes three modes: dummy cpu, dummy motherboard, full demo
# All modes will write characters to stdout

# Change to the script's directory parent (project root) to run properly
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "ProtoVM 4004 Putchar Test Script"
echo "================================"

# Build the project if needed
if [ ! -f "build/ProtoVM" ]; then
    echo "ProtoVM executable not found. Building project..."
    ./build.sh
    if [ $? -ne 0 ]; then
        echo "Build failed. Exiting."
        exit 1
    fi
fi

echo ""
echo "Running 4004 Putchar Tests in Different Modes..."
echo ""

# Function to run a test and report status
run_test() {
    local test_name=$1
    local command=$2
    local expected_output=$3
    
    echo "Running: $test_name"
    echo "Command: $command"
    
    # Capture the output from the command
    output=$(eval $command 2>&1)
    exit_code=$?
    
    echo "$output"
    
    if [ $exit_code -eq 0 ]; then
        echo "✓ $test_name completed successfully"
    else
        echo "✗ $test_name failed with exit code $exit_code"
    fi
    
    # Check if expected output was produced
    if [ -n "$expected_output" ] && echo "$output" | grep -q "$expected_output"; then
        echo "✓ Expected output '$expected_output' found"
    elif [ -n "$expected_output" ]; then
        echo "✗ Expected output '$expected_output' not found"
    fi
    
    echo ""
}

# Function to run the 4004 demo with direct output capture
run_putchar_demo() {
    echo "Running: 4004 Putchar Demo (Direct Execution)"
    echo "Command: ./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 50"
    
    # Execute and capture both stdout and stderr
    ./build/ProtoVM minimax4004 --load-binary 4004_putchar.bin 0x0 --ticks 50
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo "✓ 4004 Putchar Demo completed successfully"
    else
        echo "✗ 4004 Putchar Demo failed with exit code $exit_code"
    fi
    
    echo ""
}

echo "----------------------------------------"
echo "1. Dummy CPU Test (Direct WR0 Output)..."
echo "----------------------------------------"
run_test "Dummy 4004 CPU Test" "./build/ProtoVM test4004dummy" "A"

echo "----------------------------------------"
echo "2. Motherboard Tests (with dummy chips)..."
echo "----------------------------------------"
run_test "Motherboard Tests with Dummy Chips" "./build/ProtoVM testmotherboard" "putchar"

echo "----------------------------------------"
echo "3. Full Demo Test (Real 4004 with 4004_putchar.bin)..."
echo "----------------------------------------"
run_putchar_demo

echo "----------------------------------------"
echo "4. 4004 Output Unit Tests (Additional Test)..."
echo "----------------------------------------"
run_test "4004 Output Unit Tests" "./build/ProtoVM test4004output" ""

echo "----------------------------------------"
echo "5. 4004 Instruction Unit Tests (Additional Test)..."
echo "----------------------------------------"
run_test "4004 Instruction Unit Tests" "./build/ProtoVM test4004instructions" ""

echo "All 4004 Putchar Tests Completed!"
echo "Check the outputs above to verify characters were written to stdout."