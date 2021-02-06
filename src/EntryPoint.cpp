#include <iostream>
#include "Cartridge.h"
#include "ProjectInfo.h"
#include "MainSystem.h"


int main(int argc, char* argv[]) {

    // Print the version string to the console window
    std::cout<<PROJECT_NAME<<" "<<PROJECT_VERSION<<PROJECT_OS<<PROJECT_ARCH<< " (Compiled: " << __DATE__ << " - " << __TIME__ <<  ") "<< " starting..." << std::endl;

    MainSystem emulator;

    // See if a ROM has been passed as an argument and attempt to load it if so. if not, just load a default test rom.
    std::string ROMFileName;

    if (argc == 1) {
        ROMFileName = "nestest.nes";
    } else {
        ROMFileName = std::string(argv[1]);
    }

    // Load the ROM file for the emulator to run, if there was no error start running it.
    if (emulator.loadROM(ROMFileName)) {
        emulator.run();
    }

    return EXIT_SUCCESS;
}
