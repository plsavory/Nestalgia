#include <iostream>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Cartridge.h"
#include "PPU.h"
#include "InputManager.h"
#include "MemoryManager.h"

// Functions for reading from specific memory mappers go here.
unsigned char MemoryManager::readNROM(unsigned short location)
{
  // 0x8000 = Cartridge byte 0
  // 0xC000 = Cartridge byte 0

  // Handle the mirroring (Reading from 0xC000 is the same as reading from 0x8000) in NROM-128
  //if (location > 0xBFFF)
    //location -= 0x4000;
  if (cartridge->header[4] == 1)
      location&=0xBFFF;

    location-=0x8000;
  // Return the actual data from the cartridge array
  return cartridge->PRGROM[location];
}
