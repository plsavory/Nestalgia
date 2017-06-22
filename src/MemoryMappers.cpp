#include <iostream>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Cartridge.h"
#include "PPU.h"
#include "InputManager.h"
#include "MemoryManager.h"

// Functions for reading from specific memory mappers go here.
unsigned char MemoryManager::ReadNROM(unsigned short Location)
{
  // 0x8000 = Cartridge byte 0
  // 0xC000 = Cartridge byte 0

  // Handle the mirroring (Reading from 0xC000 is the same as reading from 0x8000) in NROM-128
  //if (Location > 0xBFFF)
    //Location -= 0x4000;
  if (mainCartridge->Header[4] == 1)
    Location&=0xBFFF;

  Location-=0x8000;
  // Return the actual data from the cartridge array
  return mainCartridge->pROM[Location];
}
