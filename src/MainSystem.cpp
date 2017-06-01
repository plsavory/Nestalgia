#include "ProjectInfo.h"
#include <sstream>
#include <iostream>
#include <SFML/Window.hpp>
#include "MainSystem.h"

MainSystem::MainSystem()
{
  mainMemory = new MemoryManager();
  mainCPU = new CPU6502(*mainMemory);
}

MainSystem::~MainSystem()
{

}

void MainSystem::Reset() {

}

void MainSystem::Execute() {
  // This function executes 1 frame of emulation.

  // PAL Master clock: 21.48MHz, NTSC master clock: 26.60mhz.
  // CPU Clock: Master/12 (NTSC), Master/16 (PAL)
}

bool MainSystem::LoadROM(std::string ROMFile) {
  if (mainMemory->LoadFile(ROMFile) == 0)
  {
    Reset();
    Execute();
    return true;
  }
  else
  {
    std::cout<<"Error loading ROM file - aborting..."<<std::endl;
    return false;
  }
}

void MainSystem::Run()
{
  // Get the BuildString for the title bar
  std::ostringstream BuildString;
  BuildString<<PROJECT_NAME<<" "<<PROJECT_VERSION<<PROJECT_OS<<PROJECT_ARCH<<" ";

  // Initialize the SFML system and pass the window handle on to the emulator object for further use.
  sf::Window MainWindow(sf::VideoMode((256*3),(240*3)), "LegacyNES",sf::Style::Close);
  MainWindow.setFramerateLimit(60);
  MainWindow.setTitle(BuildString.str());

  // Reset the CPU, PPU and APU in preparation to start
  std::cout<<"Emulator-Start"<<std::endl;
  mainCPU->Reset();

  // This will be the emulator's main loop
  while (MainWindow.isOpen())
  {
    // Check window events
    sf::Event event;

    while (MainWindow.pollEvent(event))
    {
      // Close the window when the close button is clocked
      if (event.type == sf::Event::Closed)
        MainWindow.close();
    }

    MainWindow.display();
  }

  Execute();
}
