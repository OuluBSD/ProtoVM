# Handoff: Logic Analyzer package (uppsrc/LogicAnalyzer/)

## Current state (committed bb2ca77)

All Logic Analyzer work is complete, committed, and pushed to both:
- origin: git@github.com:OuluBSD/ProtoVM.git
- 192.168.1.169: ~/Dev/ProtoVM

Working tree is clean. `git log --oneline -1` shows:
`bb2ca77 Add U++ Logic Analyzer package`

## What was built

New U++ package `uppsrc/LogicAnalyzer/` — a headless logic analyzer GUI with:

| File | Purpose |
|------|---------|
| `LogicAnalyzer.upp` | Package manifest |
| `LogicAnalyzer.h/.cpp` | Main DockWindow: MenuBar, ToolBar, StatusBar, Dockable windows |
| `CaptureModel.h/.cpp` | Headless capture model: Track/Sample/Transition/DecodedBlock/UartConfig, UART decoder, custom decoders, self-test |
| `WaveformView.h/.cpp` | Custom waveform canvas: headers, time ruler, zoom/pan/cursor, raw + decoded rendering |
| `UartSettingsDlg.h/.cpp/.lay` | UART configuration dialog |
| `CustomTracks.h/.cpp` | `PC: Bits` and `PC: Address/Data` decoders |
| `DemoCapture.h/.cpp` | Demo generators: UART, SPI, I2C patterns |
| `LogicAnalyzerTest.upp` | GUI-configured self-test package |
| `LogicAnalyzerModelTest.upp` | Console-configured model-test package |
| `SelfTest.cpp`, `TestModel.cpp` | Console self-test executables |
| `AGENTS.md` | Package instructions |

## Build commands (run from the ProtoVM repo root)

```bash
./build.sh la-test            # Build & run headless self-test (console)
./build.sh logic-analyzer     # Build GUI binary
```

`build.sh` auto-detects the U++ install (`UPP_DIR` env var, else an existing
`/yhteinen/active/sblo/Dev/ai-upp` or `$HOME/Dev/ai-upp`) and bootstraps `umk`
via `stdsrc/build/bootstrap.sh` if it is missing. Do not hardcode host paths.

Expected self-test output: 12 tracks, UART decoder produces blocks, custom decoders produce blocks, `SELF-TEST PASSED`.

GUI binary builds to `build/LogicAnalyzer` (~7.8 MB) but requires a GTK display (fails headless).

## Package layout (U++ requirement)

Each U++ package must live in its own directory with a matching `<name>.upp`:

- `uppsrc/LogicAnalyzer/LogicAnalyzer.upp` — GUI app (`uses CtrlLib, Docking, Painter`, GUI config)
- `uppsrc/LogicAnalyzerModelTest/LogicAnalyzerModelTest.upp` — console model test (`uses Core`), sources referenced as `../LogicAnalyzer/...`
- `uppsrc/LogicAnalyzerTest/LogicAnalyzerTest.upp` — console self-test (`uses Core`), sources referenced as `../LogicAnalyzer/...`

The test packages must not `uses CtrlLib` (the console config cannot include
CtrlCore).

## Build.sh changes

Added targets to `build.sh`:
- `logic-analyzer` / `la` — build GUI app
- `la-test` / `logic-analyzer-test` — build & run console model test

## Engine audit findings (not yet addressed)

- `Machine::signal_traces`/`signal_transitions` are stubs — no usable value history yet
- `ElectricNodeBase` has no generic value getter — live capture needs a probe sink overriding `PutRaw` or type-specific getters
- `Pcb::nodes` uses value semantics and relocates entries — probes must be added after netlist construction
- `CircuitAnalysis` is static diagnostics only, not a live capture source

## Known limitations (documented)

1. **GUI untested headless** — requires GTK display
2. **No live ProtoVM capture** — model uses generated demo data; no Machine/Pcb probe integration yet
3. **No file I/O** — no save/load capture format
4. **Limited decoders** — UART + 2 custom only; no SPI/I2C/CAN protocol decoders
5. **No search/filter** — decoded data search and protocol filtering not implemented

## Recommended next steps

1. `git pull origin main` (origin is at `8808ba9`, which adds this file on top of `bb2ca77`)
2. Run `./build.sh la-test` to confirm headless self-test passes (prints `SELF-TEST PASSED`)
3. Implement live capture adapter (`TraceProbe` connecting `Machine`/`Pcb` signals to `CaptureModel`)
4. Add save/load capture format
5. Add SPI/I2C/CAN decoders and search/filter

## Note

No `.agent-plans/` or `HANDOFF.md` existed before this file. There is no formal planning state to recover — just the committed code above.
