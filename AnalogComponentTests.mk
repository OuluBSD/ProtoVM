# Makefile for ProtoVM Analog Component Tests

CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -I../.. -I../../src -I$(HOME)/Topside/uppsrc
LDFLAGS = 
LIB_PATHS = -L$(HOME)/.cache/upp.out/Core/CLANG.Debug.Debug_Full.Noblitz.Shared -L$(HOME)/.cache/upp.out/plugin/chips/CLANG.Debug.Debug_Full.Noblitz.Shared
LIBS = -lCore -lchips

# Find the ProtoVM library
PROTOVM_LIB = $(HOME)/.cache/upp.out/ProtoVM/CLANG.Debug.Debug_Full.Main.Noblitz.Shared/libProtoVM.a

all: ProtoVMAnalogResistorTest ProtoVMAnalogCapacitorTest ProtoVMAnalogRCTest ProtoVMAnalogTestRunner

ProtoVMAnalogResistorTest: src/AnalogTests/ProtoVMAnalogResistorTest.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIB_PATHS) $(PROTOVM_LIB) $(LDFLAGS) $(LIBS)

ProtoVMAnalogCapacitorTest: src/AnalogTests/ProtoVMAnalogCapacitorTest.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIB_PATHS) $(PROTOVM_LIB) $(LDFLAGS) $(LIBS)

ProtoVMAnalogRCTest: src/AnalogTests/ProtoVMAnalogRCTest.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIB_PATHS) $(PROTOVM_LIB) $(LDFLAGS) $(LIBS)

ProtoVMAnalogTestRunner: src/AnalogTests/ProtoVMAnalogTestRunner.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f ProtoVMAnalogResistorTest ProtoVMAnalogCapacitorTest ProtoVMAnalogRCTest ProtoVMAnalogTestRunner

.PHONY: all clean