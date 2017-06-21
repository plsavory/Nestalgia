#include "ProjectInfo.h"
#include <sstream>
#include <iostream>
#include "Cartridge.h"
#include "MainSystem.h"

MainSystem::MainSystem()
{
  mainPPU = new PPU();
  mainMemory = new MemoryManager(*mainPPU);
  mainCPU = new CPU6502(*mainMemory);
  frameRate = new sf::Clock;
}

MainSystem::~MainSystem()
{

}

void MainSystem::Reset() {
  frameRate->restart();
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



    int CPUCycles = mainCPU->Execute(); // CPU's clock speed is MasterClockSpeed/12


    mainPPU->Execute(CPUCycles*3); // PPU's clock is 3x the CPU's

    int machineClocks = CPUCycles*12;

    if (mainPPU->NMI_Fired == true)
    {
      // If this is true, the PPU wants to send an NMI to the CPU
      mainCPU->FireInterrupt(CPUInterrupt::iNMI);
      //std::cout<<"PPU: "<<(mainPPU->NMI_Fired == true)<<std::endl;
      //std::cout<<"VBLANK_NMI Fired"<<std::endl;
    }

    if (mainCPU->InterruptProcessed == true && mainPPU->NMI_Fired == true ) {
      // an NMI has been triggered and dealt withm so reset the PPU's request
      mainPPU->NMI_Fired = false;
      mainCPU->InterruptProcessed = false;
    }


    ClocksThisFrame+=machineClocks;

  }


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
  sf::RenderWindow MainWindow(sf::VideoMode((256*3),(240*3)), "LegacyNES",sf::Style::Close);
  MainWindow.setFramerateLimit(60);
  MainWindow.setTitle(BuildString.str());

  // Working in milliseconds (Multiplier 2x makes it run at 60fps, will have to look into the timing issues when more roms run.)
  const double oneframe = 1000/120; // Will need to work on the timing when emulator is more complete. for some reason frame rate is showing 30fps unless I put 120 here.
  sf::Clock FrameTime;
  fps = 0;

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
      if (event.type == sf::Event::Closed) {
        mainCPU->myState = CPUState::Halt;
        MainWindow.close();
      }
    }

    // Update the emulator once per frame
    if (FrameTime.getElapsedTime().asMilliseconds() >= oneframe)
    {
      fps++;
      Execute();
      FrameTime.restart();
    }

    mainPPU->Draw(MainWindow); // Update the display output at the end of the frame
    MainWindow.display();

    // Set the window title to display the frame rate
    if (frameRate->getElapsedTime().asMilliseconds() >= 1000)
    {
      std::ostringstream windowtitle;
      windowtitle << BuildString.str() << " FPS: " << fps;
      MainWindow.setTitle(windowtitle.str());
      frameRate->restart();
      fps = 0;
    }

}
  //Execute();
}
