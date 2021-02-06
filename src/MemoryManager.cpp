
#include <iostream>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Cartridge.h"
#include "PPU.h"
#include "InputManager.h"
#include "MemoryManager.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <iterator>
#include <assert.h>

MemoryManager::MemoryManager(PPU &mPPU, InputManager &mInput) {
    mapper = MemoryMapper::Test;
    ppu = &mPPU; // Store a reference to the passed-on PPU object
    inputManager = &mInput;
    cartridge = nullptr;

    for (unsigned char &i : memory) {
        i = 0x0;
    }

    pageBoundaryPassed = false;

    // Interrupt lines connected to CPU
    NMILine = false;
    IRQLine = false;

    writeDMA = false;
}

bool MemoryManager::checkIRQ() {
    return IRQLine;
}

bool MemoryManager::checkNMI() {
    return NMILine;
}

MemoryManager::~MemoryManager() {
    delete cartridge;
}

int MemoryManager::loadFile(std::string fileName) {
    // Delete the cartridge instance if it already exists to free RAM.
    if (cartridge) {
        delete cartridge;
    }

    cartridge = new Cartridge();

    // Attempt to load the file..
    std::cout << "Loading file: " << fileName << std::endl;

    // Make sure the ROM exists and check its size
    struct stat fileStat;
    int filestat = stat(fileName.c_str(), &fileStat);

    if (filestat == 0) {
        int ROMSize = fileStat.st_size;

        if (ROMSize > 2630191) {
            // ROM file too large - abort
            std::cout << "Error - ROM file is too large (" << ROMSize << ") bytes" << std::endl;
            return -1;
        } else {
            std::cout << ROMSize << " byte ROM found..." << std::endl;
        }
    } else {
        // File not found - abort
        std::cout << "Error - " << fileName << " not found." << std::endl;
        return -1;
    }

    // If we've gotten this far, there are no problems with the rom (yet), so continue to load...
    typedef std::istream_iterator<unsigned char> istream_iterator;

    std::ifstream romStream(fileName, std::ios::binary); // file stream for reading file contents
    std::vector<unsigned char> tempStorage; // temporary storage for data - will be copied into cartridge struct later

    romStream >> std::noskipws; // Do not skip white space, we want every single character of the file.
    std::copy(istream_iterator(romStream), istream_iterator(),
              std::back_inserter(tempStorage)); // Copy the contents of the file into the temporary storage vector

    // Figure out which kind of iNES file we're dealing with here
    if (tempStorage[7] == 0x08 && tempStorage[0x0C] == 0x08) {
        std::cout << "iNES 2.0 format detected" << std::endl;
        cartridge->fileFormat = FileType::iNES2;
    } else if (tempStorage[7] == 0x00 && tempStorage[0x0C] == 0x00 &&
               (tempStorage[12] + tempStorage[13] + tempStorage[14] + tempStorage[15]) == 0) {
        std::cout << "iNES 0.7 format detected" << std::endl;
        cartridge->fileFormat = FileType::iNES;
    } else {
        std::cout << "Original iNES format detected" << std::endl;
        cartridge->fileFormat = FileType::iNESOriginal;
    }

    // Copy the 16-byte header into the appropriate section in the Cartridge
    for (int i = 0; i <= 15; i++) {
        cartridge->header[i] = tempStorage[i];
    }

    // Check the cartridge type before continuing
    if (checkCartridge(*cartridge) != 0) {
        return -1;
    } else {

        // Trainers are currently ignored - will need to add support for them when the program is more functional.

        // Load the PRG_ROM into the correct location
        const int FileOffset = 16 + (512 * cartridge->trainerPresent);
        int PRGOffset = 0;

        while (PRGOffset < cartridge->PRGRomSize) {
            cartridge->PRGROM[PRGOffset] = tempStorage[FileOffset + PRGOffset];
            PRGOffset++;
        }

        // Load the CHR_ROM into the correct location
        const int CHRFileOffset = FileOffset + (cartridge->PRGRomSize);

        int CHROffset = 0;

        while (CHROffset < cartridge->CHRRomSize) {
            cartridge->CHRROM[CHROffset] = tempStorage[(CHRFileOffset + CHROffset)];
            ppu->writeCROM(CHROffset, cartridge->CHRROM[CHROffset]); // Copy the values into the PPU's character memory

            CHROffset++;
        }
    }

    std::cout << "ROM File loaded successfully." << std::endl;
    return 0;
}

bool MemoryManager::getBit(int bit, unsigned char value) {
    // Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
    return (value & (1 << bit));
}

int MemoryManager::checkCartridge(Cartridge &cartridge) {
    switch (cartridge.fileFormat) {
        case FileType::iNES2:
            std::cout << "Error: Support for iNES2.0 not yet implemented." << std::endl;
            return -1;
        case FileType::iNES: {
            // Print text if the appropriate header text is found at the start of the ROM - if not perhaps print out a warning later.
            if (cartridge.header[0] == 0x4E && cartridge.header[1] == 0x45 && cartridge.header[2] == 0x53 &&
                cartridge.header[3] == 0x1A) {
                std::cout << "NES ROM header Found" << std::endl;
            }

            // Find out if there is a trainer attached to the ROM or not
            cartridge.trainerPresent = ((cartridge.header[6] >> 1) & 1);

            if (cartridge.trainerPresent) {
                std::cout << "ROM has a trainer attached." << std::endl;
            } else {
                std::cout << "ROM does not have a trainer attached." << std::endl;
            }

            // Check which vertical mirroring mode the ROM uses
            ppu->nameTableMirrorMode = getBit(0, cartridge.header[6]);

            // Remove bits we don't care about from both...
            unsigned char upper = cartridge.header[6] >> 4;
            unsigned char lower = cartridge.header[6] >> 4;
            unsigned short mapper = (upper << 4) + lower;

            cartridge.mapper = mapper;

            std::cout << "mapper found: " << std::dec << (int) mapper << std::endl;
            // Get the size of the PRG_ROM
            cartridge.PRGRomSize = (16384 * cartridge.header[4]);
            std::cout << "PRG_ROM Size: " << std::dec << (int) cartridge.PRGRomSize << "bytes" << std::endl;

            // Get the size of the CHRRom
            cartridge.CHRRomSize = (8192 * cartridge.header[5]);
            std::cout << "CHR_ROM Size: " << std::dec << (int) cartridge.CHRRomSize << "bytes" << std::endl;

            if (cartridge.header[5] == 0) {
                ppu->CHRRAM = true;
                std::cout << "Cartridge uses CHR_RAM" << std::endl;
            } else {
                ppu->CHRRAM = false;
            }

            // Get the size of the PRG_RAM
            cartridge.PRGRamSize = (8192 * cartridge.header[8]);
            if (cartridge.PRGRamSize == 0)
                cartridge.PRGRamSize = 8192;

            std::cout << "PRG_RAM Size: " << std::dec << (int) cartridge.PRGRamSize << "bytes" << std::endl;

            // Detect cartridge region
            unsigned char tmpregion = cartridge.header[10] << 6;

            tmpregion = tmpregion >> 6;

            std::cout << "Cartridge region: ";

            if (tmpregion == 0) {
                cartridge.region = CartRegion::NTSC;
                std::cout << "NTSC" << std::endl;
            } else if (tmpregion == 2) {
                cartridge.region = CartRegion::PAL;
                std::cout << "PAL" << std::endl;
            } else {
                cartridge.region = CartRegion::Dual;
                std::cout << "Dual" << std::endl;
            }

            return 0; // Cartridge is processed and checked out - continue to load
        }
        case FileType::iNESOriginal:
            std::cout << "Error: Support for original iNES format not yet implemented." << std::endl;
            return -1;
        default:
            std::cout << "Error: Unsupported file format" << std::endl;
            return -1;
    }
}

void MemoryManager::writeMemory(unsigned short location, unsigned char value) {
    // There are no memory mappers emulated at the moment in this version of this emulator - this function is a lot simpler than it eventually will be
    // Currently still working on the CPU for the most part.

    /*
    NES Memory Map:
    0000-07FF: RAM (+ Mirrored RAM)
    2000-3FFF: PPU Registers (Mirrored every 8 bytes)
    4000-4017: APU + I/O registers
    4018-401F: APU + I/O Functionality (usually disabled)
    4020-FFFF: Cartridge (All ROM + RAM chips on cartridge as well as other hardware)
    */
#ifdef DISPLAYMEMACTIVITY
    if (location <= 0xFF)
        std::cout<<"      WRITE     $00"<<std::hex<<location<<" = $"<<(int)value<<std::endl;
    else
        std::cout<<"      WRITE     $"<<std::hex<<location<<" = $"<<(int)value<<std::endl;
#endif

    if ((location >= 0x2000) && (location <= 0x3FFF)) {
        writePPU(location, value);
    }

    if (location == 0x4014) {
        // PPU OAM DMA
        OAMDMA(value);
    }

    if (location == 0x4016) {
        inputManager->WriteStatus(location);
    }

    if (location >= 0x4020)
        writeCartridge(location, value);

    if (location <= 0x1FFF)
        writeRAM(location, value); // CPU is trying to write to RAM, so handle this here...
}

void MemoryManager::writeCartridge(unsigned short location, unsigned char value) {
    // Write to the cartridge - depending on the mapper being used this process will vary
    // Only available for cartridges which include RAM on them.

    switch (cartridge->mapper) {
        default:
            return; // Return if no mapper here matches the cartridge - you can't write to an NROM cart as they include no RAM.
    }
}

unsigned char MemoryManager::readCartridge(unsigned short location) {
    // Read from the cartridge - depending on the mapper being used this process will vary
    switch (cartridge->mapper) {
        case 0:
            return readNROM(location);
        default:
            std::cout << "CPU tried to read using an unsupported mapper (Why/how is the emulator even running?)"
                      << std::endl;
            return 0x0;
    }
}

void MemoryManager::writeRAM(unsigned short location, unsigned char value) {
    /* Handle RAM mirroring and RAM writes here.
         RAM Map:
         0000-07FF - 8Kb RAM
         0800-0FFF - Mirrored RAM 0
         1000-17FF - Mirrored RAM 1
         1800-1FFF - Mirrored RAM 2
    */

    // Reduce the location value to its "base" value, and then we can write them all at once to all mirrored locations.
    while (location > 0x07FF) {
        location -= 0x800; // This could be done in a nicer way with no loop - perhaps fix later.
    }

#ifdef TestingRAMWrapping
    std::cout<<"$"<<std::hex<<(int)location<<" = "<<(int) value<<std::endl;
    std::cout<<"$"<<std::hex<<(int)location+0x800<<" = "<<(int) value<<std::endl;
    std::cout<<"$"<<std::hex<<(int)location+0x1000<<" = "<<(int) value<<std::endl;
    std::cout<<"$"<<std::hex<<(int)location+0x1800<<" = "<<(int) value<<std::endl;
#endif

    // We have the base location, so write it.
    memory[location] = value;
    memory[0x800 + location] = value;
    memory[0x1000 + location] = value;
    memory[0x1800 + location] = value;
}

unsigned char MemoryManager::readMemory(unsigned short location) {
    /*
    NES Memory Map:
    0000-1FFF: RAM (+ Mirrored RAM)
    2000-3FFF: PPU registers (Mirrored every 8 bytes)
    4000-4017: APU + I/O registers
    4018-401F: APU + I/O Functionality (usually disabled)
    4020-FFFF: Cartridge (All ROM + RAM chips on cartridge as well as other hardware)
    */

#ifdef DISPLAYMEMACTIVITY
    if (location <= 0xFF)
        std::cout<<"      READ      $00"<<std::hex<<location<<std::endl;
    else
        std::cout<<"      READ      $"<<std::hex<<location<<std::endl;
#endif

    if (location <= 0x1FFF) {
        return memory[location];
    }

    if ((location >= 0x2000) && (location <= 0x3FFF)) {
        return readPPU(location);
    }

    // Joypad registers
    if (location == 0x4016 || location == 0x4017) {
        return inputManager->GetStatus(location);
    }

    if (location >= 0x4020) {
        return readCartridge(location);
    }

    return 0x0;
}

unsigned char MemoryManager::readPPU(unsigned short location) {
    location &= 0x2007;
    unsigned short dbglocation = location;
    location -= 0x2000; // There are 8 PPU registers to read/write, so we can easily find out which one here

    //std::cout<<"readPPU:$"<<dbglocation << ":" << location << " = "<<(int)ppu->registers[location]<<std::endl;

    return ppu->readRegister(location);
}

void MemoryManager::writePPU(unsigned short location, unsigned char value) {
    // Writes to the PPU's registers
    location &= 0x2007;

    location -= 0x2000; // There are 8 PPU registers to read/write, so we can easily find out which one here
    ppu->writeRegister(location, value);
}

// These need access to the current MemoryManager state, so delare them as class functions
unsigned short MemoryManager::IN(unsigned char lo, unsigned char hi) {
    // Returns the value stored within the given absolute address
    unsigned short TargetAddress1 = AB(lo, hi);
    unsigned short TargetAddress2 = AB(lo + 1, hi);
    return (readMemory(TargetAddress1)) + (readMemory(TargetAddress2) << 8);
}

unsigned short MemoryManager::INdX(unsigned char rX, unsigned char location) {
    return (readMemory((unsigned char) (rX + location)) + ((readMemory((unsigned char) (rX + location + 0x1))) << 8));
}

unsigned short MemoryManager::INdY(unsigned char rY, unsigned char location) {
    unsigned short wrapmask = (location & 0xFF00) | ((location + 1) & 0x00FF);
    unsigned short result = (unsigned short) readMemory(location) | ((unsigned short) readMemory(wrapmask) << 8);
    unsigned short oldresult = result;

    result += (unsigned char) rY;

    // Check for a page boundary cross
    if ((result >> 8) != oldresult >> 8) {
        pageBoundaryPassed = true;
    } else {
        pageBoundaryPassed = false; // reset it otherwise so that the following instructions don't take too long
    }

    return result;
}

unsigned short MemoryManager::AB(unsigned char lo, unsigned char hi) {
    // Used for Absolute writes/reads
    return ((unsigned short) lo + (unsigned short) (hi << 8));
}

unsigned short MemoryManager::AB(unsigned char offset, unsigned char lo, unsigned char hi) {
    // Used for Absolute X and Absolute Y writes/reads
    unsigned short result = ((unsigned short) lo + (unsigned short) (hi << 8));
    unsigned short oldresult = result;

    result += offset;

    if ((result >> 8) != oldresult >> 8) {
        pageBoundaryPassed = true;
    } else {
        pageBoundaryPassed = false; // reset it otherwise so that the following instructions don't take too long
    }

    return result;
}

// Having a separate function for zero paged addressing is not strictly necessary, but it does help for debugging and also allows for the overflowing of a ZP address to happen naturally.
unsigned short MemoryManager::ZP(unsigned char location) {
    return (unsigned char) location;
}

unsigned short MemoryManager::ZP(unsigned char location, unsigned char registerValue) {
    return (unsigned char) (location + registerValue);
}

void MemoryManager::OAMDMA(unsigned char location) {
    // Writes all 256 bytes within the given main memory page to the PPU's object attribute memory. The CPU should be halted for 513 CPU cycles.
    unsigned short MemLocation = location << 8; // location gives the high byte of a memory page

    for (int i = 0; i <= 0xFF; i++) {
        // Copy each one of the 256 bytes into the PPU's OAM array
        unsigned char writelocation = i + ppu->OAMAddress;
        ppu->writeOAM(writelocation, readMemory(MemLocation));
        MemLocation++;
    }

    // Cause the CPU to be halted for the required number of cycles
    writeDMA = true;

}
