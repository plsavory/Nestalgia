#include "ProjectInfo.h"
#include "MainSystem.h"
#include <iostream>

int main() {
std::cout<<PROJECT_NAME<<" "<<PROJECT_VERSION<<PROJECT_OS<<PROJECT_ARCH<< " (Compiled: " << __DATE__ << " - " << __TIME__ <<  ") "<< " starting..." << std::endl;

MainSystem emulator;

// Initialize the SFML system and pass the window handle on to the emulator object for further use.

emulator.LoadROM();
emulator.Reset();
emulator.Run();

return 0;
 }
