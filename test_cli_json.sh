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
echo '{"command": "init-workspace", "workspace": "'"$TEST_DIR"'"}' | ./build/ProtoVM --cli

# Check if workspace was created
if [ -f "$TEST_DIR/workspace.json" ]; then
    echo "✓ init-workspace command succeeded"
else
    echo "✗ init-workspace command failed"
    exit 1
fi

# For the circuit file, we'll use a basic test circuit that should exist
# Let's create a minimal circuit file for testing
CIRCUIT_FILE="$TEST_DIR/test.circuit"
echo "# ProtoVM Circuit File
name=Test Circuit
description=Basic test circuit

# Components (0)
# Wires (0)
" > "$CIRCUIT_FILE"
echo "Created test circuit file at $CIRCUIT_FILE"

# Test create-session with user-id
echo "Testing create-session with circuit file: $CIRCUIT_FILE"
echo '{"command": "create-session", "workspace": "'"$TEST_DIR"'", "circuit-file": "'"$CIRCUIT_FILE"'", "user-id": "test_user"}' | ./build/ProtoVM --cli

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
echo '{"command": "run-ticks", "workspace": "'"$TEST_DIR"'", "session-id": '"$SESSION_ID"', "ticks": 10, "user-id": "test_user"}' | ./build/ProtoVM --cli

# Check if new snapshot was created
if [ -d "$SESSION_DIR/snapshots" ]; then
    SNAPSHOT_COUNT=$(ls $SESSION_DIR/snapshots/snapshot_*.bin 2>/dev/null | wc -l)
    if [ $SNAPSHOT_COUNT -ge 1 ]; then
        echo "✓ run-ticks command succeeded, $SNAPSHOT_COUNT snapshot(s) found"
    else
        echo "✗ run-ticks command failed, no snapshots found"
    fi
else
    echo "✗ snapshots directory does not exist"
fi

# Check if events log was updated
if [ -f "$SESSION_DIR/events.log" ]; then
    EVENT_COUNT=$(cat $SESSION_DIR/events.log | wc -l)
    if [ $EVENT_COUNT -ge 2 ]; then
        echo "✓ events.log updated, $EVENT_COUNT events found"
    else
        echo "✗ events.log not updated properly"
    fi
else
    echo "✗ events.log file not found"
fi

# Test get-state
echo "Testing get-state for session $SESSION_ID"
echo '{"command": "get-state", "workspace": "'"$TEST_DIR"'", "session-id": '"$SESSION_ID"'}' | ./build/ProtoVM --cli

# Test export-netlist
echo "Testing export-netlist for session $SESSION_ID"
echo '{"command": "export-netlist", "workspace": "'"$TEST_DIR"'", "session-id": '"$SESSION_ID"', "pcb-id": 0, "user-id": "test_user"}' | ./build/ProtoVM --cli

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

# Test destroy-session
echo "Testing destroy-session for session $SESSION_ID"
echo '{"command": "destroy-session", "workspace": "'"$TEST_DIR"'", "session-id": '"$SESSION_ID"', "user-id": "test_user"}' | ./build/ProtoVM --cli

# Check if session was deleted
if [ ! -d "$SESSION_DIR" ]; then
    echo "✓ destroy-session command succeeded"
else
    echo "✗ destroy-session command failed"
    exit 1
fi

echo "All tests completed!"