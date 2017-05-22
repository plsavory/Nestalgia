#include <iostream>
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
  // This will be the function which calls the main loop of the emulator - handle timings for the hardware here.
  std::cout<<"CPU-Start"<<std::endl;

  while (mainCPU->myState == CPUState::Running)
    mainCPU->Execute();
}

void MainSystem::LoadROM(std::string ROMFile) {
  if (mainMemory->LoadFile(ROMFile) == 0)
  {
    Reset();
    Execute();
  }
  else
  {
    std::cout<<"Error loading ROM file - aborting..."<<std::endl;
  }
}

void MainSystem::Run()
{
  Execute();
}
