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

## Engine audit findings (validated)

- `Machine::signal_traces`/`signal_transitions` are stubs — `LogSignalTransition` (`Machine.cpp:908`) has no callers, `last_value` is only ever set to 0, and `value_history`/`tick_history` are written only by snapshot deserialization (`ProtoVMCLI/MachineSnapshot.cpp:300-315`). `TestSignalTracing.cpp` exercises this and always records 0 transitions.
- `ElectricNodeBase` has no generic value getter, but one is not needed: every propagated value passes through the destination's `PutRaw`, so a passive **sink probe** observes values by definition.
- **Correction:** `Pcb::nodes` does *not* use value semantics — U++ `Array<T>` stores `T*` (`Core/Vcont.h:302-324`), so nodes are polymorphic and pointer-stable. The only ordering rule is to wire probes *before* `Machine::Init()` (which builds the `ProcessOp` table).
- `CircuitAnalysis` is static diagnostics only, not a live capture source.

## Engine build on Linux hosts

The full `ProtoVM` application package currently does **not** compile on this host (and not because of the distro alone):
- `AudioQa.h:36` declares `AudioQaReport` at global scope, but `Cli.cpp:669` uses `AudioQa::AudioQaReport` (from commit `7648c4b "Phase 29: Audio QA"`).
- `AudioQa.h:38` uses `uint64_t` without `#include <cstdint>`.
- jsoncpp's `::Json` clashes with U++'s `Json` (`Core/JSON.h:19`) under `using namespace Upp;`.
- `ProtoVM.upp` compiles `Cli.cpp` but omits `AudioQa.cpp`/`AudioQaAnalysis.cpp`.
- External deps: `sci-libs/fftw` and `dev-libs/jsoncpp` (headers land in `/usr/include/jsoncpp`, so `<json/json.h>` needs a symlink or `-I/usr/include/jsoncpp`).

### Headless engine core (`src/ProtoVMCore/`)

A library-only package was added so the simulator can be built and used without the
app main, CLI, AudioQa, FFTW or jsoncpp. It contains `Common.cpp`, `Component.cpp`,
`Link.cpp`, `Pcb.cpp`, `Machine.cpp` and a no-op `AnalogStub.cpp` (Machine only calls
`AnalogSimulation`'s ctor/`RegisterAnalogComponent`/`Tick`). Builds in ~25s.

`src/MachineCaptureTest/` is a console probe test: a source node wired into a passive
sink probe that records `PutRaw` values. It reports the captured waveform and a
non-zero transition count (`PROBE-SPIKE PASSED`) while `Machine`'s own tracing remains
0 — confirming the probe mechanism works and the built-in tracing is dead.

## Known limitations (documented)

1. **GUI untested headless** — requires GTK display
2. **Live capture** — `MachineCaptureAdapter` (`uppsrc/LogicAnalyzer/`) bridges a ProtoVMCore `Machine` to `CaptureModel`: it adds passive `TraceProbe` sinks to component pins (before `Init`), ticks the simulation sampling each tick, and emits raw tracks. Validated headlessly by `la-test` (`Live capture: clk.OUT -> N transitions`). GUI hookup (a "capture from ProtoVM" action) is not wired yet; the model still uses demo data at startup.
3. **No file I/O** — no save/load capture format
4. **Limited decoders** — UART + 2 custom only; no SPI/I2C/CAN protocol decoders
5. **No search/filter** — decoded data search and protocol filtering not implemented

## Recommended next steps

1. `git pull origin main` and run `./build.sh la-test` (prints `SELF-TEST PASSED` + a live-capture line)
2. Wire live capture into the GUI: add a menu action that builds/loads a `Machine`, uses `MachineCaptureAdapter`, and replaces the demo capture
3. Add save/load capture format
4. Add SPI/I2C/CAN decoders and search/filter
5. Optionally fix the `ProtoVM` app build (AudioQa/CLI issues above)



## Note

No `.agent-plans/` or `HANDOFF.md` existed before this file. There is no formal planning state to recover — just the committed code above.
