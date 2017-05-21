
#include <iostream>
#include "MemoryManager.h"


MemoryManager::MemoryManager()
{
	mapper = MemoryMapper::Test;

	std::cout<<"MEMORY_INIT"<<std::endl;
	for (int i = 0x0; i <= 0xFFFF; i++)
		MainMemory[i] = 0x0;

}


MemoryManager::~MemoryManager()
{
	delete mainCartridge;
}

int MemoryManager::LoadFile(std::string FilePath)
{
	// Delete the mainCartridge instance if it already exists to free RAM.
	if (mainCartridge)
		delete mainCartridge;

	mainCartridge = new Cartridge();

	// Attempt to load the file..
	std::cout<<"Loading file: "<<FilePath<<std::endl;
	return 0;
}

void MemoryManager::WriteMemory(unsigned short Location, unsigned char Value)
{
	// There are no memory mappers emulated at the moment in this version of this emulator - this function is a lot simpler than it eventually will be.
	// Currently still working on the CPU for the most part.

	if (mapper = MemoryMapper::Test)
	{
		MainMemory[Location] = Value;
		return;
	}
	// The NES has only 2Kb RAM (0x800) - Stop here if there is no mapper selected and we try to write above here
	if ((Location > 0x800) && (mapper == MemoryMapper::None))
		return;

	// $1000 - $1800 memory addresses mirror the RAM at $0000 $S0800 - so handle this here
	if (Location >= 0x0000 && Location <= 0x0800)
		MainMemory[Location + 0x1000] = Value;

	MainMemory[Location] = Value;
}

unsigned char MemoryManager::ReadMemory(unsigned short Location)
{
	return MainMemory[Location];
}

// These need access to the current MemoryManager state, so delare them as class functions
unsigned short MemoryManager::IN(unsigned char Lo, unsigned char Hi)
{
	// Returns the value stored within the given absolute address
	unsigned short TargetAddress = AB(Lo, Hi);
	return (ReadMemory(TargetAddress)) + (ReadMemory(TargetAddress+0x1) << 8);
}

unsigned short MemoryManager::INdX(unsigned char rX, unsigned char Loc)
{
	return (ReadMemory((unsigned char)(rX + Loc)) + ((ReadMemory((unsigned char)(rX + Loc + 0x1))) << 8));
}

unsigned short MemoryManager::INdY(unsigned char rY, unsigned char Loc)
{
	return (ReadMemory(Loc) + (ReadMemory(Loc+1) << 8)) + rY;
}

unsigned short MemoryManager::AB(unsigned char Lo, unsigned char Hi)
{
	// Used for Absolute writes/reads
	return ((unsigned short)Lo + (unsigned short)(Hi << 8));
}

unsigned short MemoryManager::AB(unsigned char Off, unsigned char Lo, unsigned char Hi)
{
	// Used for Absolute X and Absolute Y writes/reads
	return (unsigned short)Off + ((unsigned short)Lo + (unsigned short)(Hi << 8));
}

// Having a separate function for zero paged addressing is not strictly necessary, but it does help for debugging and also allows for the overflowing of a ZP address to happen naturally.
unsigned short MemoryManager::ZP(unsigned char Location)
{
	return (unsigned char)Location;
}

unsigned short MemoryManager::ZP(unsigned char Location, unsigned char Register)
{
	return (unsigned char)(Location + Register);
}
