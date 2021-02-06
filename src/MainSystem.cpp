#include "ProjectInfo.h"
#include <sstream>
#include <iostream>
#include "Cartridge.h"
#include "MainSystem.h"

MainSystem::MainSystem() {
    mainInput = new InputManager();
    mainPPU = new PPU();
    mainMemory = new MemoryManager(*mainPPU, *mainInput);
    mainCPU = new CPU6502(*mainMemory);
    frameRate = new sf::Clock;
    hasFocus = true;
}

MainSystem::~MainSystem() {

}

void MainSystem::reset() {
    frameRate->restart();
}

void MainSystem::execute() {
    // This function executes 1 frame of emulation.

    // PAL Master clock: 21.48MHz, NTSC master clock: 26.60mhz.
    // CPU Clock: Master/12 (NTSC), Master/16 (PAL)
    // Only NTSC is supported right now - will add PAL timings in future.
    const double MasterClocksPerFrame = 21477272 / 60;
    int ClocksThisFrame = 0;

    // Update to current controller input
    if (hasFocus) {
        mainInput->Update();
    }

    // Loop through all of the machine clicks that we need to
    while ((ClocksThisFrame < MasterClocksPerFrame) && mainCPU->state == CPUState::Running) {


        int CPUCycles = mainCPU->Execute(); // CPU's clock speed is MasterClockSpeed/12


        mainPPU->execute(CPUCycles * 3); // PPU's clock is 3x the CPU's

        int machineClocks = CPUCycles * 12;

        if (mainPPU->NMIFired) {
            // If this is true, the PPU wants to send an NMI to the CPU
            mainCPU->FireInterrupt(CPUInterrupt::iNMI);
        }

        if (mainCPU->interruptProcessed && mainPPU->NMIFired) {
            // an NMI has been triggered and dealt withm so reset the PPU's request
            mainPPU->NMIFired = false;
            mainCPU->interruptProcessed = false;
        }


        ClocksThisFrame += machineClocks;

    }


}

bool MainSystem::loadROM(std::string fileName) {
    if (mainMemory->loadFile(fileName) == 0) {
        reset();
        execute();
        return true;
    } else {
        std::cout << "Error loading ROM file - aborting..." << std::endl;
        return false;
    }
}

void MainSystem::run() {
    // Get the buildString for the title bar
    std::ostringstream buildString;
    buildString << PROJECT_NAME << " " << PROJECT_VERSION << PROJECT_OS << PROJECT_ARCH << " ";

    // Initialize the SFML system and pass the window handle on to the emulator object for further use.
    sf::RenderWindow window(sf::VideoMode((256 * 3), (240 * 3)), "LegacyNES", sf::Style::Close);
    window.setFramerateLimit(60);
    window.setTitle(buildString.str());

    // Working in milliseconds (Multiplier 2x makes it run at 60fps, will have to look into the timing issues when more roms run.)
    // TODO: Will need to work on the timing when emulator is more complete. for some reason frame rate is showing 30fps unless I put 120 here.
    const double oneFrame = 1000/120;
    sf::Clock frameTime;
    fps = 0;

    // reset the CPU, PPU and APU in preparation to start
    std::cout << "Emulator-Start" << std::endl;
    mainCPU->Reset();
    mainPPU->reset();


    // This will be the emulator's main loop
    while (window.isOpen()) {

        // Check window events
        sf::Event event{};

        while (window.pollEvent(event)) {
            // Close the window when the close button is clocked
            if (event.type == sf::Event::Closed) {
                mainCPU->state = CPUState::Halt;
                window.close();
            }

            if (event.type == sf::Event::GainedFocus) {
                hasFocus = true;
            }

            if (event.type == sf::Event::LostFocus) {
                hasFocus = false;
            }

        }

        // Update the emulator once per frame
        if (frameTime.getElapsedTime().asMilliseconds() >= oneFrame) {
            fps++;
            execute();
            frameTime.restart();
        }

        mainPPU->draw(window); // Update the display output at the end of the frame
        window.display();

        // Set the window title to display the frame rate
        if (frameRate->getElapsedTime().asMilliseconds() >= 1000) {
            std::ostringstream windowTitle;
            windowTitle << buildString.str();

            if (false) { // TODO: Allow this to be configured in a config file or something.
                windowTitle << " FPS: " << fps;
            }

            window.setTitle(windowTitle.str());
            frameRate->restart();
            fps = 0;
        }

    }
}
