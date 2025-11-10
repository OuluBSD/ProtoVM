#include "ProtoVM.h"
#include "StateMachine.h"

#include <Core/Core.h>
using namespace UPP;

// Entry point for the test that works with Machine
void Test60_StateMachine() {
    LOG("Starting FSM Test...");
    
    // Create a 4-state state machine
    StateMachine fsm(4);  // 4 states: 0, 1, 2, 3
    fsm.SetName("TestStateMachine");

    // Define transitions:
    // From state 0: if IN0=1, go to state 1
    fsm.SetTransition(0, 1, 0x01);  // Condition: IN0=1
    
    // From state 1: if IN1=1, go to state 2
    fsm.SetTransition(1, 2, 0x02);  // Condition: IN1=1
    
    // From state 2: if IN2=1, go to state 3
    fsm.SetTransition(2, 3, 0x04);  // Condition: IN2=1
    
    // From state 3: if IN3=1, go back to state 0
    fsm.SetTransition(3, 0, 0x08);  // Condition: IN3=1
    
    // Set outputs for each state (Moore machine)
    fsm.SetOutputForState(0, 0x00);  // Output 0 in state 0
    fsm.SetOutputForState(1, 0x01);  // Output 1 in state 1
    fsm.SetOutputForState(2, 0x02);  // Output 2 in state 2
    fsm.SetOutputForState(3, 0x03);  // Output 3 in state 3

    String msg1 = "Initial state: State=" + AsString(fsm.GetCurrentState());
    LOG(msg1);
    
    // Test basic transitions manually by setting input and ticking
    // Initially in state 0
    String msg2 = "Current state: " + AsString(fsm.GetCurrentState());
    LOG(msg2);
    
    // Set input to transition from state 0 to 1 (IN0=1)
    fsm.PutRaw(2, (byte*)1, 0, 1);  // Set IN0 = 1
    fsm.Tick();
    String msg3 = "After setting IN0=1: State=" + AsString(fsm.GetCurrentState());
    LOG(msg3);
    
    // Reset input to 0
    fsm.PutRaw(2, (byte*)0, 0, 1);  // Set IN0 = 0
    fsm.Tick();
    String msg4 = "After setting IN0=0: State=" + AsString(fsm.GetCurrentState());
    LOG(msg4);
    
    // Set IN1 to transition to state 2
    fsm.PutRaw(3, (byte*)1, 0, 1);  // Set IN1 = 1
    fsm.Tick();
    String msg5 = "After setting IN1=1: State=" + AsString(fsm.GetCurrentState());
    LOG(msg5);
    
    // Reset IN1 to 0
    fsm.PutRaw(3, (byte*)0, 0, 1);  // Set IN1 = 0
    fsm.Tick();
    String msg6 = "After setting IN1=0: State=" + AsString(fsm.GetCurrentState());
    LOG(msg6);
    
    // Test a few more transitions manually
    fsm.PutRaw(4, (byte*)1, 0, 1);  // Set IN2 = 1
    fsm.Tick();
    String msg7 = "After setting IN2=1: State=" + AsString(fsm.GetCurrentState());
    LOG(msg7);
    
    fsm.PutRaw(4, (byte*)0, 0, 1);  // Set IN2 = 0
    fsm.Tick();
    String msg8 = "After setting IN2=0: State=" + AsString(fsm.GetCurrentState());
    LOG(msg8);
    
    fsm.PutRaw(5, (byte*)1, 0, 1);  // Set IN3 = 1
    fsm.Tick();
    String msg9 = "After setting IN3=1: State=" + AsString(fsm.GetCurrentState());
    LOG(msg9);
    
    LOG("State Machine test completed.");
}