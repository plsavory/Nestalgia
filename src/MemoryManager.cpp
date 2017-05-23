
#include <iostream>
#include "MemoryManager.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <iterator>

MemoryManager::MemoryManager()
{
	mapper = MemoryMapper::Test;

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

	// Make sure the ROM exists and check its size
	struct stat fileStat;
	int filestat = stat(FilePath.c_str(),&fileStat);

	if (filestat == 0)
	{
		int ROMSize = fileStat.st_size;

		if (ROMSize > 2630191)
		{
			// ROM file too large - abort
			std::cout << "Error - ROM file is too large (" << ROMSize << ") bytes" << std::endl;
			return -1;
		}
		else
		{
			std::cout << ROMSize << " byte ROM found..." << std::endl;
		}
	}
	else
	{
		// File not found - abort
		std::cout<<"Error - " << FilePath << " not found." << std::endl;
		return -1;
	}

	// If we've gotten this far, there are no problems with the rom (yet), so continue to load...
	typedef std::istream_iterator<unsigned char> istream_iterator;

	std::ifstream romStream(FilePath,std::ios::binary); // file stream for reading file contents
	std::vector<unsigned char> tempStorage; // temporary storage for data - will be copied into cartridge struct later

	romStream >> std::noskipws; // Do not skip white space, we want every single character of the file.
	std::copy(istream_iterator(romStream),istream_iterator(),std::back_inserter(tempStorage)); // Copy the contents of the file into the temporary storage vector

	// Figure out which kind of iNES file we're dealing with here
	if (tempStorage[7] == 0x08 && tempStorage[0x0C] == 0x08) {
		std::cout<<"iNES 2.0 format detected"<<std::endl;
		mainCartridge->FileFormat = FileType::iNES2;
	} else if (tempStorage[7] == 0x00 && tempStorage[0x0C] == 0x00 && (tempStorage[12] + tempStorage[13] + tempStorage[14] + tempStorage[15]) == 0) {
		std::cout<<"iNES 0.7 format detected"<<std::endl;
		mainCartridge->FileFormat = FileType::iNES;
	} else {
		std::cout<<"Original iNES format detected"<<std::endl;
		mainCartridge->FileFormat = FileType::iNESOriginal;
	}

	// Copy the 16-byte header into the appropriate section in the Cartridge
	for (int i = 0; i<=15;i++)
		mainCartridge->Header[i] = tempStorage[i];

	// Check the cartridge type before continuing
	if (CheckCartridge(*mainCartridge) != 0)
	{
		return -1;
	}

	std::cout<<"ROM File loaded successfully."<<std::endl;
	return 0;
}

int MemoryManager::CheckCartridge(Cartridge &cartridge)
{
	switch(cartridge.FileFormat)
	{
		case FileType::iNES2:
		{
			std::cout<<"Error: Support for iNES2.0 not yet implemented."<<std::endl;
			return -1;
		}
		case FileType::iNES:
		{
			// Print text if the appropriate header text is found at the start of the ROM - if not perhaps print out a warning later.
			if (cartridge.Header[0] == 0x4E && cartridge.Header[1] == 0x45 && cartridge.Header[2] == 0x53 && cartridge.Header[3] == 0x1A)
				std::cout<<"NES ROM Header Found"<<std::endl;

			// Find out if there is a trainer attached to the ROM or not
			cartridge.TrainerPresent = ((cartridge.Header[6] >> 1) & 1);
			if (cartridge.TrainerPresent)
				std::cout<<"ROM has a trainer attached."<<std::endl;
			else
				std::cout<<"ROM does not have a trainer attached."<<std::endl;

			// Store the mapper ID

			// Get the size of the PRG_ROM

			// Get the size of the CHRRom

			// Get the size of the PRG_RAM


			std::cout<<"Error: Support for iNES07 not yet implemented."<<std::endl;
			return -1;
		}
		case FileType::iNESOriginal:
		{
			std::cout<<"Error: Support for original iNES format not yet implemented."<<std::endl;
			return -1;
		}
		default:
		{
			std::cout<<"Error: Unsupported ROM format"<<std::endl;
			return -1;
		}
	}
	return 0;
}

void MemoryManager::WriteMemory(unsigned short Location, unsigned char Value)
{
	// There are no memory mappers emulated at the moment in this version of this emulator - this function is a lot simpler than it eventually will be.
	// Currently still working on the CPU for the most part.

	if (mapper == MemoryMapper::Test)
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
