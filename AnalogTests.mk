# Makefile to run ProtoVM Analog Tests

all: resistor capacitor rc simulation

resistor:
	@echo "Running Analog Resistor Test..."
	@./build/ProtoVM analog-resistor

capacitor:
	@echo "Running Analog Capacitor Test..."
	@./build/ProtoVM analog-capacitor

rc:
	@echo "Running Analog RC Circuit Test..."
	@./build/ProtoVM analog-rc

simulation:
	@echo "Running Analog Simulation Test..."
	@./build/ProtoVM analog-sim

all-tests: resistor capacitor rc simulation
	@echo "All analog tests completed!"

help:
	@echo "ProtoVM Analog Test Suite"
	@echo "Available targets:"
	@echo "  all          - Run all analog tests (default)"
	@echo "  resistor     - Run analog resistor test"
	@echo "  capacitor    - Run analog capacitor test"
	@echo "  rc           - Run analog RC circuit test"
	@echo "  simulation   - Run analog simulation test"
	@echo "  all-tests    - Run all tests in sequence"
	@echo "  help         - Show this help message"

.PHONY: all resistor capacitor rc simulation all-tests help