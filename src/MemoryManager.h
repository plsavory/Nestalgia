#pragma once
enum MemoryMapper {
    None, Test
};
enum HeaderData {
    Const0, Const1, Const2, Const3, PROMSize, CROMSize, Flags6, Flags7, PRAMSize, Flags9, Flags10, Null
};

class MemoryManager {
public:
    bool pageBoundaryPassed; // Holds true if the previous indirect memory operation passed a page boundary, reset otherwise.
    Cartridge *cartridge;
    bool writeDMA;

    MemoryManager(PPU &mPPU, InputManager &mInput);

    ~MemoryManager();

    int loadFile(std::string fileName);

    unsigned char readMemory(unsigned short location);

    void writeMemory(unsigned short location, unsigned char value);

    unsigned short IN(unsigned char lo, unsigned char hi);

    unsigned short INdX(unsigned char rX, unsigned char location);

    unsigned short INdY(unsigned char rY, unsigned char location);

    unsigned short AB(unsigned char lo, unsigned char hi);

    unsigned short AB(unsigned char offset, unsigned char lo, unsigned char hi);

    unsigned short ZP(unsigned char location);

    unsigned short ZP(unsigned char location, unsigned char registerValue);

    bool checkIRQ();

    bool checkNMI();

private:
    unsigned char memory[0xFFFF];
    bool IRQLine;
    bool NMILine;
    InputManager *inputManager;
    PPU *ppu;
    MemoryMapper mapper;

    void writeRAM(unsigned short location, unsigned char value);

    void writeCartridge(unsigned short location, unsigned char value);

    void OAMDMA(unsigned char location);

    unsigned short wrapMemory(unsigned short location, unsigned short wrapValue);

    unsigned char readCartridge(unsigned short location);

    unsigned char readPPU(unsigned short location);

    void writePPU(unsigned short location, unsigned char value);

    int checkCartridge(Cartridge &cartridge);

    bool getBit(int bit, unsigned char value);

    // mapper functions
    unsigned char readNROM(unsigned short location);
};
