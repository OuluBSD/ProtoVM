# Makefile for ProtoVM Analog Audio Test

CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -I../.. -I../../src -I$(HOME)/Topside/uppsrc
LDFLAGS = -lportaudio
LIB_PATHS = -L$(HOME)/.cache/upp.out/Core/CLANG.Debug.Debug_Full.Noblitz.Shared -L$(HOME)/.cache/upp.out/plugin/chips/CLANG.Debug.Debug_Full.Noblitz.Shared
LIBS = -lCore -lchips

# Find the ProtoVM library
PROTOVM_LIB = $(HOME)/.cache/upp.out/ProtoVM/CLANG.Debug.Debug_Full.Main.Noblitz.Shared/libProtoVM.a

all: ProtoVMAnalogAudioTest

ProtoVMAnalogAudioTest: ../../src/AnalogTests/ProtoVMAnalogAudioTest.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIB_PATHS) $(PROTOVM_LIB) $(LDFLAGS) $(LIBS)

clean:
	rm -f ProtoVMAnalogAudioTest

.PHONY: all clean