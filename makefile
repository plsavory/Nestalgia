CC = g++
CFLAGS = -std=c++17 -g -Wall -O3

all:
	g++ -std=c++11 -I SFML\include src\CPU6502.cpp src\MemoryManager.cpp src\MemoryMappers.cpp src\MainSystem.cpp src\EntryPoint.cpp src\PPU.cpp src\InputManager.cpp src\APU.cpp -L SFML\lib -lsfml-graphics -lsfml-window -lsfml-system -o build\NESEmulator.exe -O3 -D_hypot=hypot
