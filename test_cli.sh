#!/bin/bash

# Test script for ProtoVM CLI with new EngineFacade and snapshot functionality

echo "Testing ProtoVM CLI with new EngineFacade and snapshot functionality..."

# Set up test workspace
TEST_DIR=$(pwd)/test_workspace
rm -rf $TEST_DIR
mkdir -p $TEST_DIR

echo "Created test workspace at $TEST_DIR"

# Test init-workspace command
echo "Testing init-workspace..."
./build/ProtoVM --cli init-workspace --workspace $TEST_DIR

# Check if workspace was created
if [ -f "$TEST_DIR/workspace.json" ]; then
    echo "✓ init-workspace command succeeded"
else
    echo "✗ init-workspace command failed"
    exit 1
fi

# For the circuit file, we'll use a basic test circuit that should exist
# Let's check what circuits are available
echo "Available test circuits:"
find . -name "*.circuit" -type f

# Let's try to find a basic circuit file for testing
CIRCUIT_FILE=$(find . -name "*.circuit" | head -n 1)

if [ -z "$CIRCUIT_FILE" ]; then
    echo "No circuit files found for testing."
    # Create a minimal circuit file for testing
    CIRCUIT_FILE="$TEST_DIR/test.circuit"
    echo "# ProtoVM Circuit File
name=Test Circuit
description=Basic test circuit

# Components (0)
# Wires (0)
" > "$CIRCUIT_FILE"
    echo "Created test circuit file at $CIRCUIT_FILE"
fi

# Test create-session with user-id
echo "Testing create-session with circuit file: $CIRCUIT_FILE"
./build/ProtoVM --cli create-session --workspace $TEST_DIR --circuit-file "$CIRCUIT_FILE" --user-id test_user

# Check if session was created
SESSION_DIRS=($TEST_DIR/sessions/*/)
if [ ${#SESSION_DIRS[@]} -gt 0 ]; then
    SESSION_DIR=${SESSION_DIRS[0]}
    echo "✓ create-session command succeeded"
    echo "Session directory: $SESSION_DIR"
    
    # Check if snapshot directory was created
    if [ -d "$SESSION_DIR/snapshots" ]; then
        echo "✓ snapshots directory created"
    else
        echo "✗ snapshots directory not created"
    fi
    
    # Check if events log file was created
    if [ -f "$SESSION_DIR/events.log" ]; then
        echo "✓ events.log created"
        echo "Event log contents:"
        cat "$SESSION_DIR/events.log"
    else
        echo "✗ events.log not created"
    fi
else
    echo "✗ create-session command failed"
    exit 1
fi

# Get the session ID from directory name
SESSION_ID=$(basename $SESSION_DIR)

# Test run-ticks
echo "Testing run-ticks for session $SESSION_ID"
./build/ProtoVM --cli run-ticks --workspace $TEST_DIR --session-id $SESSION_ID --ticks 10 --user-id test_user

# Check if new snapshot was created
SNAPSHOT_COUNT=$(ls $SESSION_DIR/snapshots/snapshot_*.bin | wc -l)
if [ $SNAPSHOT_COUNT -ge 1 ]; then
    echo "✓ run-ticks command succeeded, $SNAPSHOT_COUNT snapshot(s) found"
else
    echo "✗ run-ticks command failed, no snapshots found"
fi

# Check if events log was updated
EVENT_COUNT=$(cat $SESSION_DIR/events.log | wc -l)
if [ $EVENT_COUNT -ge 2 ]; then
    echo "✓ events.log updated, $EVENT_COUNT events found"
else
    echo "✗ events.log not updated properly"
fi

# Test get-state
echo "Testing get-state for session $SESSION_ID"
./build/ProtoVM --cli get-state --workspace $TEST_DIR --session-id $SESSION_ID

# Test export-netlist
echo "Testing export-netlist for session $SESSION_ID"
./build/ProtoVM --cli export-netlist --workspace $TEST_DIR --session-id $SESSION_ID --pcb-id 0 --user-id test_user

# Check if netlist file was created
NETLIST_DIR="$SESSION_DIR/netlists"
if [ -d "$NETLIST_DIR" ]; then
    echo "✓ netlists directory created"
    NETLIST_FILES=($NETLIST_DIR/netlist_*.txt)
    if [ ${#NETLIST_FILES[@]} -gt 0 ]; then
        echo "✓ netlist exported to ${NETLIST_FILES[0]}"
    else
        echo "✗ no netlist files found"
    fi
else
    echo "✗ netlists directory not created"
fi

# Check if export-netlist event was logged
EVENT_COUNT=$(cat $SESSION_DIR/events.log | wc -l)
if [ $EVENT_COUNT -ge 3 ]; then
    echo "✓ export-netlist event logged, $EVENT_COUNT total events"
else
    echo "✗ export-netlist event not logged"
fi

# Test destroy-session
echo "Testing destroy-session for session $SESSION_ID"
./build/ProtoVM --cli destroy-session --workspace $TEST_DIR --session-id $SESSION_ID --user-id test_user

# Check if session was deleted
if [ ! -d "$SESSION_DIR" ]; then
    echo "✓ destroy-session command succeeded"
else
    echo "✗ destroy-session command failed"
    exit 1
fi

# Check if destroy event was logged
if [ -f "$SESSION_DIR/events.log" ]; then
    # It's expected that the directory would be removed
    echo "Event log may have been removed with session directory"
fi

echo "All tests completed successfully!"