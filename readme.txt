LegacyNES readme.txt

LegacyNES is a Nintendo Entertainment System emulator which is developed purely for learning and experimental purposes, this is not intended to be used as a commercial product paid or otherwise.

As of now, the emulator is incapable of running any games or rom files fully, as the CPU emulation functionality is still under development while the graphics functionality is currently missing completely.

Compile instructions:

Windows:

To compile on Windows, the project assumes that MinGW is present on the system and has been added to the PATH variable. Once MinGW is set up and added to the PATH variable, simply run build.bat to build the code into an executable form. The executable will be saved in the build folder under the name "NESEmulator.exe".

To run the emulator component tests, use the buildtest.bat file. The resulting file will be saved in the build folder under the name "NESEmulator-ftest.exe". This version will not attempt to load or run any rom files, it will simply test its internal functions.

UNIX-like systems (Including Mac OSX & Linux):

Simply cd into the folder using a terminal and type "make" to build a normal build, the resulting file will be saved as /build/NESEmulator

Use the command "make ftest" to build a function test version, which will operate in the same way as described in the Windows section of the document.