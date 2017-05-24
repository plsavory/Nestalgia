all:
	g++ -std=c++11 src/CPU6502.cpp src/MemoryManager.cpp src/MemoryMappers.cpp src/MainSystem.cpp src/EntryPoint.cpp -o build/NESEmulator

ftest:
	g++ -std=c++11 src/CPU6502.cpp src/MemoryManager.cpp src/MemoryMappers.cpp src/UnitTestClass.cpp src/TestEntryPoint.cpp -o build/NESEmulator-ftest
