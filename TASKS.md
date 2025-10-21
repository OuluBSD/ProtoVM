# ProtoVM Digital Logic Simulation Tasks

## TODO

- [ ] Add propagation delay modeling system
- [ ] Model setup and hold time constraints
- [ ] Add topological ordering for component evaluation
- [ ] Add timing simulation for different clock domains

## IN PROGRESS

## DONE

- [x] Implement multi-tick convergence algorithm in Machine::Tick()
- [x] Add proper bus arbitration with tri-state buffer support
- [x] Modify Bus classes to support tri-state logic
- [x] Update IC6502 to handle bidirectional buses properly
- [x] Update ICRamRom to handle bidirectional buses properly
- [x] Implement oscillation detection to prevent infinite loops
- [x] Add feedback loop resolution within each tick
- [x] Implement state change detection to optimize processing