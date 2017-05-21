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
  if (mainCPU->myState == CPUState::Running)
    mainCPU->Execute();
}

void MainSystem::LoadROM() {

}

void MainSystem::Run()
{
  Execute();
}
