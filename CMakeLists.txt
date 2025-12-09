cmake_minimum_required(VERSION 3.10)
project(ProtoVM_Project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If we're building the CLI, we don't need wxWidgets
option(BUILD_PROTOVM_CLI "Build the ProtoVM CLI tool" ON)

# First, let's define the core ProtoVM library that both GUI and CLI will use
set(CORE_SOURCES
    src/ProtoVM/AdditionalSources.cpp
    src/ProtoVM/AddressDecoder4004.cpp
    src/ProtoVM/ADSR.cpp
    src/ProtoVM/ALU.cpp
    src/ProtoVM/AnalogCommon.cpp
    src/ProtoVM/AnalogComponents.cpp
    src/ProtoVM/AnalogSemiconductors.cpp
    src/ProtoVM/AnalogSimulation.cpp
    src/ProtoVM/Arithmetic.cpp
    src/ProtoVM/AudioOutputSystem.cpp
    src/ProtoVM/AudioProcessingModes.cpp
    src/ProtoVM/AudioSignalPath.cpp
    src/ProtoVM/Basic8BitCPU.cpp
    src/ProtoVM/Bus.cpp
    src/ProtoVM/BusController4004.cpp
    src/ProtoVM/BusInterface4004.cpp
    src/ProtoVM/ClockDivider.cpp
    src/ProtoVM/ClockGate.cpp
    src/ProtoVM/ClockGenerator4004.cpp
    src/ProtoVM/Common.cpp
    src/ProtoVM/CommonTubeTopologies.cpp
    src/ProtoVM/CompleteTubeComputerSystem.cpp
    src/ProtoVM/ComplexCPU.cpp
    src/ProtoVM/Component.cpp
    src/ProtoVM/ComponentHierarchy.cpp
    src/ProtoVM/CrossSwitch.cpp
    src/ProtoVM/CrystalOscillator.cpp
    src/ProtoVM/CustomTransformer.cpp
    src/ProtoVM/DemoCadc.cpp
    src/ProtoVM/EffectsProcessor.cpp
    src/ProtoVM/ExamplePatches.cpp
    src/ProtoVM/FaultInjection.cpp
    src/ProtoVM/FormalVerification.cpp
    src/ProtoVM/Fuse.cpp
    src/ProtoVM/Generic.cpp
    src/ProtoVM/Helper4004.cpp
    src/ProtoVM/IC4001.cpp
    src/ProtoVM/IC4002.cpp
    src/ProtoVM/IC4003.cpp
    src/ProtoVM/IC4004.cpp
    src/ProtoVM/IC6502.cpp
    src/ProtoVM/ICcadc.cpp
    src/ProtoVM/ICRamRom.cpp
    src/ProtoVM/ICs.cpp
    src/ProtoVM/Interak.cpp
    src/ProtoVM/LFO.cpp
    src/ProtoVM/Link.cpp
    src/ProtoVM/Machine.cpp
    src/ProtoVM/MDS1101SchematicTool.cpp
    src/ProtoVM/MDS1104SchematicTool.cpp
    src/ProtoVM/Mechanical.cpp
    src/ProtoVM/Memristor.cpp
    src/ProtoVM/MidiInput.cpp
    src/ProtoVM/MiniMax8085.cpp
    src/ProtoVM/MinimaxCADC.cpp
    src/ProtoVM/Oversampling.cpp
    src/ProtoVM/Pcb.cpp
    src/ProtoVM/ParameterAutomation.cpp
    src/ProtoVM/Photoresistor.cpp
    src/ProtoVM/PLL.cpp
    src/ProtoVM/PolyphonyManager.cpp
    src/ProtoVM/Potentiometer.cpp
    src/ProtoVM/PowerOnReset4004.cpp
    src/ProtoVM/PresetManager.cpp
    src/ProtoVM/ProtoVM.cpp
    src/ProtoVM/PslParser.cpp
    src/ProtoVM/PslTestRunner.cpp
    src/ProtoVM/RCOscillator.cpp
    src/ProtoVM/Relay.cpp
    src/ProtoVM/Router.cpp
    src/ProtoVM/SimpleComputerSystem.cpp
    src/ProtoVM/SimpleCPU.cpp
    src/ProtoVM/SparkGap.cpp
    src/ProtoVM/StandardLibrary.cpp
    src/ProtoVM/StateMachine.cpp
    src/ProtoVM/SynthesizerArchitectures.cpp
    src/ProtoVM/SynthUI.cpp
    src/ProtoVM/TappedTransformer.cpp
    src/ProtoVM/Test00_FlipFlop.cpp
    src/ProtoVM/Test01_ANDGate.cpp
    src/ProtoVM/Test02_Counter.cpp
    src/ProtoVM/Test03_BasicLogicGates.cpp
    src/ProtoVM/Test4004CPU.cpp
    src/ProtoVM/Test4004Dummy.cpp
    src/ProtoVM/Test4004Instructions.cpp
    src/ProtoVM/Test4004Output.cpp
    src/ProtoVM/Test4004Runner.cpp
    src/ProtoVM/Test4BitAdder.cpp
    src/ProtoVM/Test4BitMemory.cpp
    src/ProtoVM/Test4BitRegister.cpp
    src/ProtoVM/TestALU.cpp
    src/ProtoVM/TestAnalogSynthComponents.cpp
    src/ProtoVM/TestBasic8BitCPU.cpp
    src/ProtoVM/TestCadc.cpp
    src/ProtoVM/TestChipsUnit.cpp
    src/ProtoVM/TestClockDivider.cpp
    src/ProtoVM/TestClockGate.cpp
    src/ProtoVM/TestInspect.cpp
    src/ProtoVM/TestLogicGates.cpp
    src/ProtoVM/TestMotherboard.cpp
    src/ProtoVM/TestPLL.cpp
    src/ProtoVM/TestSignalTracing.cpp
    src/ProtoVM/TestStateMachine.cpp
    src/ProtoVM/TestTubeCountersRegisters.cpp
    src/ProtoVM/TestTubeFlipFlops.cpp
    src/ProtoVM/TestTubeLogicGates.cpp
    src/ProtoVM/TestVectorGenerator.cpp
    src/ProtoVM/Thermistor.cpp
    src/ProtoVM/TimingAnalysis.cpp
    src/ProtoVM/TransArrayProc.cpp
    src/ProtoVM/Transformer.cpp
    src/ProtoVM/TransmissionLine.cpp
    src/ProtoVM/TriodeTubeModel.cpp
    src/ProtoVM/TubeAmplifier.cpp
    src/ProtoVM/TubeAmplifierComponent.cpp
    src/ProtoVM/TubeAmpSimulation1950s.cpp
    src/ProtoVM/TubeAmpSimulation1960s.cpp
    src/ProtoVM/TubeAmpSimulation1970s.cpp
    src/ProtoVM/TubeAmpSimulation1980s.cpp
    src/ProtoVM/TubeAmpSimulation1990s.cpp
    src/ProtoVM/TubeAmpSimulation2000s.cpp
    src/ProtoVM/TubeArithmeticUnits.cpp
    src/ProtoVM/TubeAudioIO.cpp
    src/ProtoVM/TubeCircuits.cpp
    src/ProtoVM/TubeCircuitTopologies.cpp
    src/ProtoVM/TubeClockOscillators.cpp
    src/ProtoVM/TubeComponents.cpp
    src/ProtoVM/TubeComputerSimulator.cpp
    src/ProtoVM/TubeComputerSystems.cpp
    src/ProtoVM/TubeCountersRegisters.cpp
    src/ProtoVM/TubeDeEsser.cpp
    src/ProtoVM/TubeDistortion.cpp
    src/ProtoVM/TubeEffectCircuits.cpp
    src/ProtoVM/TubeEffects.cpp
    src/ProtoVM/TubeExciter.cpp
    src/ProtoVM/TubeFilter.cpp
    src/ProtoVM/TubeFiltersOscillators.cpp
    src/ProtoVM/TubeFlipFlops.cpp
    src/ProtoVM/TubeGateExpander.cpp
    src/ProtoVM/TubeLogic.cpp
    src/ProtoVM/TubeLogicGates.cpp
    src/ProtoVM/TubeLogicLibrary.cpp
    src/ProtoVM/TubeModels.cpp
    src/ProtoVM/TubeMultiBandCompressor.cpp
    src/ProtoVM/TubeMultiChorus.cpp
    src/ProtoVM/TubeMultiplexers.cpp
    src/ProtoVM/TubeMuxDemux.cpp
    src/ProtoVM/TubePhaser.cpp
    src/ProtoVM/TubePitchShifter.cpp
    src/ProtoVM/TubePowerSupply.cpp
    src/ProtoVM/TubeReverb.cpp
    src/ProtoVM/TubeReverbCircuits.cpp
    src/ProtoVM/TubeStandardLogicLibrary.cpp
    src/ProtoVM/TubeTapeEcho.cpp
    src/ProtoVM/TubeTremolo.cpp
    src/ProtoVM/UK101.cpp
    src/ProtoVM/VCA.cpp
    src/ProtoVM/VCF.cpp
    src/ProtoVM/VCO.cpp
    src/ProtoVM/VirtualAnalogModel.cpp
    src/ProtoVM/VoltageSources.cpp
)

# Create the core library
add_library(proto_vm_core STATIC ${CORE_SOURCES})
target_include_directories(proto_vm_core PUBLIC src/ProtoVM)

# GUI executable
set(GUI_SOURCES
    wxsrc/main.cpp
    wxsrc/ProtoVMApp.cpp
    wxsrc/MainFrame.cpp
    wxsrc/CircuitCanvas.cpp
    wxsrc/CircuitSerializer.cpp
    wxsrc/UndoRedo.cpp
    wxsrc/PropertiesPanel.cpp
    wxsrc/ComponentLibrary.cpp
    wxsrc/ComponentPalette.cpp
    wxsrc/SimulationController.cpp
    wxsrc/SimulationBridge.cpp
)

# Use wx-config to get compiler and linker flags for GUI
find_program(WX_CONFIG_PROGRAM NAMES wx-config)
if(NOT WX_CONFIG_PROGRAM)
    message(FATAL_ERROR "wx-config not found. Please install wxWidgets development libraries.")
endif()

execute_process(
    COMMAND ${WX_CONFIG_PROGRAM} --cxxflags
    OUTPUT_VARIABLE WX_CXX_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${WX_CONFIG_PROGRAM} --libs core,base,propgrid,adv
    OUTPUT_VARIABLE WX_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WX_CXX_FLAGS}")
separate_arguments(WX_LIBS_LIST NATIVE_COMMAND "${WX_LIBS}")

add_executable(protovm_gui ${GUI_SOURCES})
target_include_directories(protovm_gui PRIVATE wxsrc)
target_link_libraries(protovm_gui proto_vm_core ${WX_LIBS_LIST})

# CLI executable
if(BUILD_PROTOVM_CLI)
    set(CLI_SOURCES
        src/ProtoVMCLI/CliMain.cpp
        src/ProtoVMCLI/CommandDispatcher.cpp
        src/ProtoVMCLI/JsonIO.cpp
        src/ProtoVMCLI/JsonFilesystemSessionStore.cpp
        src/ProtoVMCLI/EngineFacade.cpp
        src/ProtoVMCLI/MachineSnapshot.cpp
        src/ProtoVMCLI/EventLogger.cpp
        src/ProtoVMCLI/CircuitFacade.cpp
        src/ProtoVMCLI/CircuitAnalysis.cpp
        src/ProtoVMCLI/CircuitMerge.cpp
        src/ProtoVMCLI/BranchOperations.cpp
        src/ProtoVMCLI/CircuitGraph.cpp
        src/ProtoVMCLI/CircuitGraphQueries.cpp
        src/ProtoVMCLI/HlsIr.cpp
        src/ProtoVMCLI/HlsIrInference.cpp
        src/ProtoVMCLI/DiffAnalysis.cpp
        src/ProtoVMCLI/CoDesigner.cpp
        src/ProtoVMCLI/ScheduledIr.cpp
        src/ProtoVMCLI/Scheduling.cpp
        src/ProtoVMCLI/CdcModel.cpp
        src/ProtoVMCLI/CdcAnalysis.cpp
        src/ProtoVMCLI/PipelineModel.cpp
        src/ProtoVMCLI/PipelineAnalysis.cpp
        src/ProtoVMCLI/RetimingModel.cpp
        src/ProtoVMCLI/RetimingAnalysis.cpp
    )

    add_executable(proto-vm-cli ${CLI_SOURCES})
    target_include_directories(proto-vm-cli PRIVATE
        src/ProtoVMCLI
        src/ProtoVM
    )
    target_link_libraries(proto-vm-cli proto_vm_core)

    # Daemon executable
    set(DAEMON_SOURCES
        src/ProtoVMCLI/DaemonMain.cpp
        src/ProtoVMCLI/SessionServer.cpp
        src/ProtoVMCLI/CommandDispatcher.cpp
        src/ProtoVMCLI/JsonIO.cpp
        src/ProtoVMCLI/JsonFilesystemSessionStore.cpp
        src/ProtoVMCLI/EngineFacade.cpp
        src/ProtoVMCLI/MachineSnapshot.cpp
        src/ProtoVMCLI/EventLogger.cpp
        src/ProtoVMCLI/CircuitFacade.cpp
        src/ProtoVMCLI/CircuitAnalysis.cpp
        src/ProtoVMCLI/CircuitMerge.cpp
        src/ProtoVMCLI/BranchOperations.cpp
        src/ProtoVMCLI/CircuitGraph.cpp
        src/ProtoVMCLI/CircuitGraphQueries.cpp
        src/ProtoVMCLI/HlsIr.cpp
        src/ProtoVMCLI/HlsIrInference.cpp
        src/ProtoVMCLI/DiffAnalysis.cpp
        src/ProtoVMCLI/CoDesigner.cpp
        src/ProtoVMCLI/ScheduledIr.cpp
        src/ProtoVMCLI/Scheduling.cpp
        src/ProtoVMCLI/CdcModel.cpp
        src/ProtoVMCLI/CdcAnalysis.cpp
        src/ProtoVMCLI/PipelineModel.cpp
        src/ProtoVMCLI/PipelineAnalysis.cpp
        src/ProtoVMCLI/RetimingModel.cpp
        src/ProtoVMCLI/RetimingAnalysis.cpp
    )

    add_executable(proto-vm-daemon ${DAEMON_SOURCES})
    target_include_directories(proto-vm-daemon PRIVATE
        src/ProtoVMCLI
        src/ProtoVM
    )
    target_link_libraries(proto-vm-daemon proto_vm_core)
endif()