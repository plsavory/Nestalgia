#include "ProjectInfo.h"
#include <sstream>
#include <iostream>
#include "MainSystem.h"

MainSystem::MainSystem()
{
  mainPPU = new PPU();
  mainMemory = new MemoryManager(*mainPPU);
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
  // Only NTSC is supported right now - will add PAL timings in future.
  const double MasterClocksPerFrame = 21477272/60;
  int ClocksThisFrame;

  // Loop through all of the machine clicks that we need to
  while ((ClocksThisFrame < MasterClocksPerFrame) && mainCPU->myState==CPUState::Running)
  {
    int CPUCycles = mainCPU->Execute()*12; // CPU's clock speed is MasterClockSpeed/12


    mainPPU->Execute(CPUCycles*3); // PPU's clock is 3x the CPU's
    ClocksThisFrame+=CPUCycles;
  }

  if (mainCPU->myState!=CPUState::Running)
    std::cout<<"CPU Halted"<<std::endl;
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

  // Working in milliseconds
  const double oneframe = 1000/60;
  sf::Clock FrameTime;

  // Reset the CPU, PPU and APU in preparation to start
  std::cout<<"Emulator-Start"<<std::endl;
  mainCPU->Reset();
  mainPPU->Reset();


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

    // Update the emulator once per frame
    if (FrameTime.getElapsedTime().asMilliseconds() >= oneframe)
    {
      Execute();
      FrameTime.restart();
    }

    mainPPU->Draw(MainWindow);
    MainWindow.display();
  }

  Execute();
}
