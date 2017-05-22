#include <iostream>
#include "ProjectInfo.h"
#include "MainSystem.h"


int main(int argc, char* argv[]) {

// Print the version string to the console window
std::cout<<PROJECT_NAME<<" "<<PROJECT_VERSION<<PROJECT_OS<<PROJECT_ARCH<< " (Compiled: " << __DATE__ << " - " << __TIME__ <<  ") "<< " starting..." << std::endl;

MainSystem emulator;

// See if a ROM has been passed as an argument and attempt to load it if so. if not, just load a default test rom.
std::string ROMFile;

if (argc == 1)
  ROMFile = "nestest.nes";
  else
  ROMFile = std::string(argv[1]);


// Initialize the SFML system and pass the window handle on to the emulator object for further use.

emulator.LoadROM(ROMFile);

return EXIT_SUCCESS;
 }
