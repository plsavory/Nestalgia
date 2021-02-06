#pragma once

#include <fstream>
#include "CPUInstructions.h"

using namespace M6502;


class CPU6502 {
public:
    explicit CPU6502(MemoryManager &mManager);

    ~CPU6502();

    CPUState state;
    bool fireBRK;
    bool fireReset;
    bool fireNMI;
    bool interruptProcessed;

    void SetFlag(Flag flag, bool val);

    unsigned char SetBit(int bit, bool val, unsigned char value);

    bool GetBit(int bit, unsigned char value);

    unsigned char GetFlags();

    bool GetFlag(Flag flag);

    bool GetFlag(Flag flag, unsigned char value);

    bool SignBit(unsigned char value);

    unsigned char GetAcc();

    unsigned char GetX();

    unsigned char GetY();

    // Items below here should in future be private, but for unit testing purposes they are currently public.
    unsigned char rA, rX, rY; // CPU registers
    unsigned char AND(unsigned char value);

    unsigned char ASL(unsigned char value);

    unsigned char LSR(unsigned char value);

    unsigned char LD(unsigned char value);

    unsigned char ROL(unsigned char value);

    unsigned char ROR(unsigned char value);

    unsigned char ADC(unsigned char value);

    unsigned char SBC(unsigned char value);

    unsigned char ORA(unsigned char value);

    unsigned char EOR(unsigned char value);

    unsigned char IN(unsigned char value);

    unsigned char DE(unsigned char value);

    unsigned char SLO(unsigned char value);

    unsigned char SRE(unsigned char value);

    unsigned char DCP(unsigned char value);

    unsigned char ISB(unsigned char value);

    unsigned char RLA(unsigned char value);

    unsigned char RRA(unsigned char value);

    unsigned char SAX();

    void LAX(unsigned char value);

    void CMP(unsigned char registerValue, unsigned char value);

    unsigned short answer16;

    int Execute();

    void Reset();

    void HandleInterrupt(
            int type); // Forces the CPU to jump to an interrupt vector. may be called be a CPU instruction or piece of emulated hardware
    void FireInterrupt(int type);

private:
    unsigned char b1;
    unsigned char flagRegister;
    unsigned short programCounter;
    signed char dataOffset; // Causes the programCounter to be incremented at the end of an instruction
    unsigned short jumpOffset; // Used to tell the CPU where to jump next
    int cyclesTaken = 0;
    unsigned char stackPointer;
    bool pageBoundaryPassed;
    unsigned char location;
    unsigned short location16;
    unsigned char result;
    MemoryManager *memory;
    std::ofstream *redirectFile;
    unsigned char currentInstruction;
    int cpuCycles;

    unsigned char nextByte();

    void JMP(unsigned short location);

    void branch(bool value);

    void printCPUStatus(std::string instructionName);

    std::string getInstructionName(unsigned char opcode);

    void pushStack8(unsigned char value);

    void pushStack16(unsigned short value);

    void fPHP();

    unsigned char popStack();

    void fPLP(unsigned char value);

    void BIT(unsigned char value);

    void fRTS();

    void fRTI();

    void fBRK();

    std::string decToHex(int value);

    void checkInterrupts();
};
