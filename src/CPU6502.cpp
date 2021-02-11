#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Cartridge.h"
#include "PPU.h"
#include "InputManager.h"
#include "MemoryManager.h"
#include "CPU6502.h"

#define LOGCPUTOFILE55
#define PRINTCPUSTATUS56

CPU6502::CPU6502(MemoryManager &mManager) {
    state = CPUState::Halt;
    memory = &mManager;
    flagRegister = 0x24; // Initialize the flags register
    rA = rX = rY = 0x0;
    programCounter = 0x0;
    state = CPUState::Halt;
    cpuCycles = 0;
    interruptProcessed = false;
}


CPU6502::~CPU6502() {
}


void CPU6502::Reset() {
    // Should always be called before starting emulation (Sets the program counter to the appropriate place)
    state = CPUState::Running;
    flagRegister = 0x24; // Initialize the flags register - unused and I should be set to 1.
    rX = 0x0;
    rY = 0x0;
    rA = 0x0;
    programCounter = (memory->readMemory(0xFFFD) * 256) + memory->readMemory(0xFFFC);

    stackPointer = 0xFD; // Set the stack pointer
    SetFlag(Flag::Unused, 1);
    SetFlag(Flag::EInterrupt, 1);
    state = CPUState::Running;
    cpuCycles = 0;

    // Set the interrupt lines all to false
    fireBRK = false;
    fireNMI = false;

    // Set up CPU logging (if it is enabled)
    // Set up the std::cout redirect for logging
    interruptProcessed = false;

#ifdef LOGCPUTOFILE
    std::cout<<"Logging CPU activity to CPULog.log"<<std::endl;
    redirectFile = new std::ofstream("CPULog.log");
    std::cout.rdbuf(redirectFile->rdbuf());
#endif
}

/**
 * Sets a specific flag to true or false depending on the value of "val"
 * @param flag
 * @param val
 */
void CPU6502::SetFlag(Flag flag, bool val) {
    val ? flagRegister |= flag : flagRegister &= ~(flag);
}

/**
 * Sets a specific flag to true or false depending on the value of "val"
 * @param bit
 * @param val
 * @param value
 * @return
 */
unsigned char CPU6502::SetBit(int bit, bool val,
                              unsigned char value) // Used for setting flags to a value which is not the flag register
{
    return val ? value | (1 << bit) : value & ~(1 << bit);
}

bool CPU6502::GetFlag(Flag flag) {
    // Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
    return (flagRegister & flag) != 0;
}

bool CPU6502::SignBit(unsigned char value) {
    return (value & 1 << 7) != 0;
}

bool CPU6502::GetBit(int bit, unsigned char value) {
    // Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
    return (value & (1 << bit)) != 0;
}

bool CPU6502::GetFlag(Flag flag, unsigned char value) {
    // Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
    return (value & flag) != 0;
}

unsigned char CPU6502::ORA(unsigned char value) {
    // Inclusive OR between A and value. Save result in A. Set Zero and Negative flags appropriately
    unsigned char result = value | rA;
    SetFlag(Flag::Zero, result == 0x0);
    SetFlag(Flag::Sign, result > 0x7F);
    return result;
}

unsigned char CPU6502::EOR(unsigned char value) {
    unsigned char result = value ^rA;
    SetFlag(Flag::Zero, result == 0x0);
    SetFlag(Flag::Sign, result > 0x7F);
    return result;
}

unsigned char CPU6502::AND(unsigned char value) {
    // AND the value with the accumulator, and then set the flags accordingly and return the result.
    unsigned char result = value & rA;
    SetFlag(Flag::Zero, result == 0x0);
    SetFlag(Flag::Sign, result > 0x7F);
    return result;
}

unsigned char CPU6502::ASL(unsigned char value) {
    unsigned char result = value << 1;
    SetFlag(Flag::Carry, (value & (1 << 7)));
    SetFlag(Flag::Sign, (result & (1 << 7)));
    SetFlag(Flag::Zero, result == 0x0);
    return result;
}

unsigned char CPU6502::LSR(unsigned char value) {
    unsigned char result = value >> 1;
    SetFlag(Flag::Carry, GetBit(0, value));
    result = SetBit(7, 0, result);
    SetFlag(Flag::Sign, (result > 0x7F));
    SetFlag(Flag::Zero, result == 0x0);
    return result;
}

unsigned char CPU6502::SLO(unsigned char value) {
    unsigned char result = ASL(value);
    rA = ORA(result);
    return result;
}

unsigned char CPU6502::SRE(unsigned char value) {
    unsigned char result = LSR(value);
    rA = EOR(result);
    return result;
}

unsigned char CPU6502::RLA(unsigned char value) {
    unsigned char result = ROL(value);
    rA = AND(result);
    return result;
}

unsigned char CPU6502::RRA(unsigned char value) {
    unsigned char result = ROR(value);
    rA = ADC(result);
    return result;
}

unsigned char CPU6502::ROL(unsigned char value) {
    // Shift carry onto bit 0 and shift the original bit 7 onto the carry
    unsigned char result = value << 1;
    result = SetBit(0, GetFlag(Flag::Carry),
                    result); // Put the current value of the carry flag onto bit 7 of the result
    SetFlag(Flag::Carry, GetBit(7, value)); // Shift bit 0 of the original value onto the carry flag.
    SetFlag(Flag::Zero, result == 0);
    SetFlag(Flag::Sign, result > 0x7F);
    return result;
}

unsigned char CPU6502::ROR(unsigned char value) {
    // Shift all bits right by 1
    unsigned char result = value >> 1;
    result = SetBit(7, GetFlag(Flag::Carry),
                    result); // Put the current value of the carry flag onto bit 7 of the result
    SetFlag(Flag::Carry, GetBit(0, value)); // Shift bit 0 of the original value onto the carry flag.
    SetFlag(Flag::Zero, result == 0);
    SetFlag(Flag::Sign, result > 0x7F);
    return result;
}

unsigned char CPU6502::LD(unsigned char value) {
    // Sets the flag register as an LD operation should and then simply returns the value.

    // If value == 0, set zero flag, otherwise clear it.
    SetFlag(Flag::Zero, value == 0x0);
    // If value > 127, set the sign flag so that the 6502 program knows that the number is negative, otherwise clear it.
    SetFlag(Flag::Sign, value > 0x7F);

    return value;
}

void CPU6502::LAX(unsigned char value) {
    rA = LD(value);
    rX = LD(value);
}

unsigned char CPU6502::ADC(unsigned char value) {
    /*
    ADC - Add "value" to the current value of the Accumulator register and the carry flag.
    Set the negative, overflow, zero and carry flags accordingly.
     If the decimal flag is set to 1, this operation will be performed in BCD mode, however the NES's CPU lacks this functionality so it does not need to be implemented at this time
    (perhaps make it a compile option for future use of this CPU emulator in other systems?)
    */

    // Perform the calculation and store it in a 16-bit value
    unsigned const OperationResult = value + rA + GetFlag(Flag::Carry);
    // Truncate this result down to an 8-bit value - this discards the most significant bit, but we extract it from the 16-bit value later.
    unsigned char RetVal = (unsigned char) OperationResult;

    // Debug reasons - make it possible to extract 16-bit version of the answer from the CPU
    answer16 = OperationResult;
    // Set the overflow flag if the sign of the addition values are the same, but differ from the sign of the sum result
    //SetFlag(Flag::Overflow, (~(rA ^ value) & (rA ^ OperationResult)) & 0x80);

    if ((SignBit(rA) == SignBit(value)) && (SignBit(rA) != SignBit(OperationResult)))
        SetFlag(Flag::Overflow, 1);
    else
        SetFlag(Flag::Overflow, 0);

    // Set the zero flag accordingly
    SetFlag(Flag::Zero, RetVal == 0);

    // Set the Sign flag accordingly (most significant bit of the 8-bit result)
    SetFlag(Flag::Sign, RetVal > 0x7F);

    /* Set the Carry flag accordingly - should be set if the result of the operation is > 255,
     in this case this becomes 511 but it is up to the program running inside the CPU how it represents this. */
    SetFlag(Flag::Carry, OperationResult > 0xFF);
    return RetVal;
}

unsigned char CPU6502::SBC(unsigned char value) {
    return ADC(~value); // Lazy lol - but it works.
}

unsigned char CPU6502::IN(unsigned char value) {
    unsigned char result = value + 1;
    SetFlag(Flag::Zero, result == 0);
    SetFlag(Flag::Sign, result > 0x7F);
    return result;
}

unsigned char CPU6502::DE(unsigned char value) {
    unsigned char result = value - 1;
    SetFlag(Flag::Zero, result == 0);
    SetFlag(Flag::Sign, result > 0x7F);
    return result;
}

unsigned char CPU6502::DCP(unsigned char value) {
    unsigned char result = value - 1;
    CMP(rA, result);
    return result;
}

unsigned char CPU6502::ISB(unsigned char value) {
    unsigned char result = value + 1;
    rA = SBC(result);
    return result;
}

unsigned char CPU6502::SAX() {
    unsigned char RetVal = (rX & rA);
    return RetVal;
}

unsigned char CPU6502::nextByte() {
    // Get the value of the next byte from Memory
    //return memory->readMemory(programCounter+(dataOffset++));
#ifdef PRINTCPUSTATUS
    if (currentInstruction) {
    std::cout<<" "<<ConvertHex(memory->readMemory(programCounter,1))<<" ";
    }
    dataOffset++;
#endif

    return memory->readMemory(programCounter++);
}

int CPU6502::Execute() {
    // Debug reasons
    cyclesTaken = 0; // Remove this later
    // Print the CPU's current status
#ifdef PRINTCPUSTATUS
    std::ostringstream cpustatestring;

    if (currentInstruction) // if not null
    {
        // Print out the current state of the PC
        std::cout<<std::uppercase<<std::hex<<(int)programCounter<<"   ";
        cpustatestring <<" "<< std::hex<<std::uppercase<<InstName(currentInstruction) <<"      A:"<<ConvertHex(rA)<<" X:"<<ConvertHex(rX)<<" Y:"<<ConvertHex(rY)<<" P:"<<(int)flagRegister<<" SP:"<<(int)stackPointer<<" CPUC:"<<std::dec<<cpuCycles<<std::hex<<" -"<<std::endl;
    }
        //printCPUStatus(getInstructionName(currentInstruction));
#endif

    // Check if the CPU needs to be halted for 513 cycles because of DMA writes
    if (memory->writeDMA) {
        cyclesTaken = 513;
        memory->writeDMA = false;
        return cyclesTaken;
    }

    // Handle interrupts if neccesary
    checkInterrupts();

    // Fetch the next opcode
    unsigned char opcode = nextByte();
#ifdef PRINTCPUSTATUS
    //dataOffset = 0; // This is used only to display the CPU output in the log file now, reset it to 0 here for text alignment.
#endif
    pageBoundaryPassed = 0;
    jumpOffset = 0;

    // Attempt to execute the opcode
    switch (opcode) {
        // BRK instructions
        case BRK:
            fBRK();
            cyclesTaken = 7;
            break;
            // LD_ZP instructions
        case LDA_ZP:
            rA = LD(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case LDX_ZP:
            rX = LD(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case LDY_ZP:
            rY = LD(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
            // LD_IMM instructions
        case LDA_IMM:
            rA = LD(nextByte());
            cyclesTaken = 2;
            break;
        case LDX_IMM:
            rX = LD(nextByte());
            cyclesTaken = 2;
            break;
        case LDY_IMM:
            rY = LD(nextByte());
            cyclesTaken = 2;
            break;
            // LD_AB instructions
        case LDA_AB:
            b1 = nextByte(); // Get first byte of next address
            rA = LD(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case LDX_AB:
            b1 = nextByte();
            rX = LD(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case LDY_AB:
            b1 = nextByte();
            rY = LD(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
            // LD_ABX/Y instructions
        case LDA_ABX:
            b1 = nextByte();
            rA = LD(memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case LDA_ABY:
            b1 = nextByte();
            rA = LD(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case LDX_ABY:
            b1 = nextByte();
            rX = LD(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case LDY_ABX:
            b1 = nextByte();
            rY = LD(memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
            // LD_ZPX instructions
        case LDA_ZPX:
            rA = LD(memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
        case LDX_ZPY:
            rX = LD(memory->readMemory(memory->ZP(nextByte(), rY)));
            cyclesTaken = 4;
            break;
        case LDY_ZPX:
            rY = LD(memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
            // LDA_IN instructions
        case LDA_INX:
            rA = LD(memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case LDA_INY:
            rA = LD(memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
            // AND instructions
        case AND_IMM:
            rA = AND(nextByte());
            cyclesTaken = 2;
            break;
        case AND_ZP:
            rA = AND(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case AND_ZPX:
            rA = AND(memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
        case AND_AB:
            b1 = nextByte();
            rA = AND(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case AND_ABX:
            b1 = nextByte();
            rA = AND(memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case AND_ABY:
            b1 = nextByte();
            rA = AND(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case AND_INX:
            rA = AND(memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case AND_INY:
            rA = AND(memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
            // ORA instructions
        case ORA_IMM:
            rA = ORA(nextByte());
            cyclesTaken = 2;
            break;
        case ORA_ZP:
            rA = ORA(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case ORA_ZPX:
            rA = ORA(memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
        case ORA_AB:
            b1 = nextByte();
            rA = ORA(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case ORA_ABX:
            b1 = nextByte();
            rA = ORA(memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case ORA_ABY:
            b1 = nextByte();
            rA = ORA(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case ORA_INX:
            rA = ORA(memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case ORA_INY:
            rA = ORA(memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
            // EOR instructions
        case EOR_IMM:
            rA = EOR(nextByte());
            cyclesTaken = 2;
            break;
        case EOR_ZP:
            rA = EOR(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case EOR_ZPX:
            rA = EOR(memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
        case EOR_AB:
            b1 = nextByte();
            rA = EOR(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case EOR_ABX:
            b1 = nextByte();
            rA = EOR(memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case EOR_ABY:
            b1 = nextByte();
            rA = EOR(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case EOR_INX:
            rA = EOR(memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case EOR_INY:
            rA = EOR(memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
            // ADC instructions
        case ADC_IMM:
            rA = ADC(nextByte());
            cyclesTaken = 2;
            break;
        case ADC_ZP:
            rA = ADC(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case ADC_ZPX:
            rA = ADC(memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
        case ADC_AB:
            b1 = nextByte();
            rA = ADC(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case ADC_ABX:
            b1 = nextByte();
            rA = ADC(memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case ADC_ABY:
            b1 = nextByte();
            rA = ADC(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case ADC_INX:
            rA = ADC(memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case ADC_INY:
            rA = ADC(memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
            // ABC instructions
        case SBC_IMM:
            rA = SBC(nextByte());
            cyclesTaken = 2;
            break;
        case SBC_ZP:
            rA = SBC(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case SBC_ZPX:
            rA = SBC(memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
        case SBC_AB:
            b1 = nextByte();
            rA = SBC(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case SBC_ABX:
            b1 = nextByte();
            rA = SBC(memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case SBC_ABY:
            b1 = nextByte();
            rA = SBC(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case SBC_INX:
            rA = SBC(memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case SBC_INY:
            rA = SBC(memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
            // Increment instructions
        case INX:
            rX = IN(rX);
            cyclesTaken = 2;
            break;
        case INY:
            rY = IN(rY);
            cyclesTaken = 2;
            break;
        case DEX:
            rX = DE(rX);
            cyclesTaken = 2;
            break;
        case DEY:
            rY = DE(rY);
            cyclesTaken = 2;
            break;
            // ASL instructions
        case ASL_ACC:
            rA = ASL(rA);
            cyclesTaken = 2;
            break;
        case ASL_ZP:
            location = memory->ZP(nextByte());
            result = ASL(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 5;
            break;
        case ASL_ZPX:
            location = memory->ZP(nextByte(), rX);
            result = ASL(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 6;
            break;
        case ASL_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            result = ASL(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 6;
            break;
        case ASL_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            result = ASL(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 7;
            break;
            // LSR functions
        case LSR_ACC:
            rA = LSR(rA);
            cyclesTaken = 2;
            break;
        case LSR_ZP:
            location = memory->ZP(nextByte());
            result = LSR(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 5;
            break;
        case LSR_ZPX:
            location = memory->ZP(nextByte(), rX);
            result = LSR(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 6;
            break;
        case LSR_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            result = LSR(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 6;
            break;
        case LSR_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            result = LSR(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 7;
            break;
            // ROL operations
        case ROL_ACC:
            rA = ROL(rA);
            cyclesTaken = 2;
            break;
        case ROL_ZP:
            location = memory->ZP(nextByte());
            result = ROL(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 5;
            break;
        case ROL_ZPX:
            location = memory->ZP(nextByte(), rX);
            result = ROL(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 6;
            break;
        case ROL_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            result = ROL(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 6;
            break;
        case ROL_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            result = ROL(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 7;
            break;
        case ROR_ACC:
            rA = ROR(rA);
            cyclesTaken = 2;
            break;
        case ROR_ZP:
            location = memory->ZP(nextByte());
            result = ROR(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 5;
            break;
        case ROR_ZPX:
            location = memory->ZP(nextByte(), rX);
            result = ROR(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 6;
            break;
        case ROR_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            result = ROR(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 6;
            break;
        case ROR_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            result = ROR(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 7;
            break;
            // INC & DEC Instructions
        case INC_ZP:
            location = memory->ZP(nextByte());
            result = IN(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 5;
            break;
        case INC_ZPX:
            location = memory->ZP(nextByte(), rX);
            result = IN(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 6;
            break;
        case INC_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            result = IN(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 6;
            break;
        case INC_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            result = IN(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 7;
            break;
        case DEC_ZP:
            location = memory->ZP(nextByte());
            result = DE(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 5;
            break;
        case DEC_ZPX:
            location = memory->ZP(nextByte(), rX);
            result = DE(memory->readMemory(location));
            memory->writeMemory(location, result);
            cyclesTaken = 6;
            break;
        case DEC_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            result = DE(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 6;
            break;
        case DEC_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            result = DE(memory->readMemory(location16));
            memory->writeMemory(location16, result);
            cyclesTaken = 7;
            break;
            // Store operations
        case STA_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, rA);
            cyclesTaken = 3;
            break;
        case STA_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, rA);
            cyclesTaken = 4;
            break;
        case STA_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, rA);
            cyclesTaken = 4;
            break;
        case STA_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            memory->writeMemory(location16, rA);
            cyclesTaken = 5;
            break;
        case STA_ABY:
            b1 = nextByte();
            location16 = memory->AB(rY, b1, nextByte());
            memory->writeMemory(location16, rA);
            cyclesTaken = 5;
            break;
        case STA_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, rA);
            cyclesTaken = 6;
            break;
        case STA_INY:
            location16 = memory->INdY(rY, nextByte());
            memory->writeMemory(location16, rA);
            cyclesTaken = 6;
            break;
        case STX_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, rX);
            cyclesTaken = 3;
            break;
        case STX_ZPY:
            location = memory->ZP(nextByte(), rY);
            memory->writeMemory(location, rX);
            cyclesTaken = 4;
            break;
        case STX_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, rX);
            cyclesTaken = 4;
            break;
        case STY_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, rY);
            cyclesTaken = 3;
            break;
        case STY_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, rY);
            cyclesTaken = 4;
            break;
        case STY_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, rY);
            cyclesTaken = 4;
            break;
            // Branch instructions
        case BCC:
            branch(!GetFlag(Flag::Carry));
            break;
        case BCS:
            branch(GetFlag(Flag::Carry));
            break;
        case BEQ:
            branch(GetFlag(Flag::Zero));
            break;
        case BMI:
            branch(GetFlag(Flag::Sign));
            break;
        case BNE:
            //std::cout<<"BNE "<<std::hex<<(int)memory->readMemory(programCounter,true)<<" "<<(int)memory->readMemory(programCounter+1);
            branch(!GetFlag(Flag::Zero));
            break;
        case BPL:
            branch(!GetFlag(Flag::Sign));
            break;
        case BVC:
            branch(!GetFlag(Flag::Overflow));
            break;
        case BVS:
            branch(GetFlag(Flag::Overflow));
            break;
            // Transfer instructions
        case TAX:
            rX = LD(rA);
            cyclesTaken = 2;
            break;
        case TAY:
            rY = LD(rA);
            cyclesTaken = 2;
            break;
        case TXA:
            rA = LD(rX);
            cyclesTaken = 2;
            break;
        case TYA:
            rA = LD(rY);
            cyclesTaken = 2;
            break;
        case TSX:
            rX = LD(stackPointer);
            cyclesTaken = 2;
            break;
        case TXS:
            stackPointer = rX; // Does not effect flags
            cyclesTaken = 2;
            break;
            // Clear Flag instructions
        case CLC:
            SetFlag(Flag::Carry, 0);
            cyclesTaken = 2;
            break;
        case CLV:
            SetFlag(Flag::Overflow, 0);
            cyclesTaken = 2;
            break;
        case CLD:
            SetFlag(Flag::BCDMode, 0);
            cyclesTaken = 2;
            break;
        case CLI:
            SetFlag(Flag::EInterrupt, 0);
            cyclesTaken = 2;
            break;
            // Set Flag instructions
        case SEC:
            SetFlag(Flag::Carry, 1);
            cyclesTaken = 2;
            break;
        case SED:
            SetFlag(Flag::BCDMode, 1);
            cyclesTaken = 2;
            break;
        case SEI:
            SetFlag(Flag::EInterrupt, 1);
            cyclesTaken = 2;
            break;
            // JMP instructions
        case JMP_AB:
            b1 = nextByte();
            JMP(memory->AB(b1, nextByte()));
            cyclesTaken = 3;
            break;
        case JMP_IN:
            // Jump to the target address which is contained in the memory address after the byte.
            b1 = nextByte();
            JMP(memory->IN(b1, nextByte()));
            cyclesTaken = 5;
            break;
        case JSR:
            b1 = nextByte();
            pushStack16(programCounter); // Push the location of the next instruction -1 to the stack
            JMP(memory->AB(b1, nextByte()));
            cyclesTaken = 6;
            break;
        case RTS:
            fRTS();
            cyclesTaken = 6;
            break;
        case RTI:
            fRTI();
            cyclesTaken = 6;
            break;
        case BIT_ZP:
            BIT(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case BIT_AB:
            b1 = nextByte();
            BIT(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
            // Stack operations
        case PHA:
            pushStack8(rA);
            cyclesTaken = 3;
            break;
        case PHP:
            fPHP();
            cyclesTaken = 3;
            break;
        case PLA:
            rA = LD(popStack());
            cyclesTaken = 4;
            break;
        case PLP:
            fPLP(popStack());
            cyclesTaken = 4;
            break;
            // CMP Instructions
        case CMP_IMM:
            CMP(rA, nextByte());
            cyclesTaken = 2;
            break;
        case CMP_ZP:
            CMP(rA, memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case CMP_ZPX:
            CMP(rA, memory->readMemory(memory->ZP(nextByte(), rX)));
            cyclesTaken = 4;
            break;
        case CMP_AB:
            b1 = nextByte();
            CMP(rA, memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case CMP_ABX:
            b1 = nextByte();
            CMP(rA, memory->readMemory(memory->AB(rX, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case CMP_ABY:
            b1 = nextByte();
            CMP(rA, memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case CMP_INX:
            CMP(rA, memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case CMP_INY:
            CMP(rA, memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
        case CPX_IMM:
            CMP(rX, nextByte());
            cyclesTaken = 2;
            break;
        case CPX_ZP:
            CMP(rX, memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case CPX_AB:
            b1 = nextByte();
            CMP(rX, memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case CPY_IMM:
            CMP(rY, nextByte());
            cyclesTaken = 2;
            break;
        case CPY_ZP:
            CMP(rY, memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case CPY_AB:
            b1 = nextByte();
            CMP(rY, memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case NOP:
            // Do nothing
            cyclesTaken = 2;
            break;
            // Undocumented opcodes from here on outside
            // NOP Varients
        case NOP1:
        case NOP2:
        case NOP3:
        case NOP4:
        case NOP5:
        case NOP6:
            cyclesTaken = 2;
            break;
            // DOP (Double NOP) (All of these read from the memory address following it, and do nothing with the result)
            // Actually reading the values to keep consistency with log files
        case DOP1:
        case DOP4:
        case DOP6:
            memory->readMemory(memory->ZP(nextByte()));
            cyclesTaken = 3;
            break;
        case DOP2:
        case DOP3:
        case DOP5:
        case DOP7:
        case DOP12:
        case DOP14:
            memory->readMemory(memory->ZP(nextByte(), rX));
            cyclesTaken = 4;
            break;
        case DOP8:
        case DOP9:
        case DOP10:
        case DOP11:
        case DOP13:
            nextByte();
            cyclesTaken = 2;
            break;
        case DCP_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, DCP(memory->readMemory(location)));
            cyclesTaken = 5;
            break;
        case DCP_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, DCP(memory->readMemory(location)));
            cyclesTaken = 6;
            break;
        case DCP_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, DCP(memory->readMemory(location16)));
            cyclesTaken = 6;
            break;
        case DCP_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            memory->writeMemory(location16, DCP(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case DCP_ABY:
            b1 = nextByte();
            location16 = memory->AB(rY, b1, nextByte());
            memory->writeMemory(location16, DCP(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case DCP_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, DCP(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
        case DCP_INY:
            location16 = memory->INdY(rY, nextByte());
            memory->writeMemory(location16, DCP(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
            // ISB
        case ISB_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, ISB(memory->readMemory(location)));
            cyclesTaken = 5;
            break;
        case ISB_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, ISB(memory->readMemory(location)));
            cyclesTaken = 6;
            break;
        case ISB_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, ISB(memory->readMemory(location16)));
            cyclesTaken = 6;
            break;
        case ISB_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            memory->writeMemory(location16, ISB(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case ISB_ABY:
            b1 = nextByte();
            location16 = memory->AB(rY, b1, nextByte());
            memory->writeMemory(location16, ISB(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case ISB_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, ISB(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
        case ISB_INY:
            location16 = memory->INdY(rY, nextByte());
            memory->writeMemory(location16, ISB(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
            // RLA
        case RLA_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, RLA(memory->readMemory(location)));
            cyclesTaken = 5;
            break;
        case RLA_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, RLA(memory->readMemory(location)));
            cyclesTaken = 6;
            break;
        case RLA_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, RLA(memory->readMemory(location16)));
            cyclesTaken = 6;
            break;
        case RLA_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            memory->writeMemory(location16, RLA(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case RLA_ABY:
            b1 = nextByte();
            location16 = memory->AB(rY, b1, nextByte());
            memory->writeMemory(location16, RLA(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case RLA_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, RLA(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
        case RLA_INY:
            location16 = memory->INdY(rY, nextByte());
            memory->writeMemory(location16, RLA(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
            // RRA
        case RRA_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, RRA(memory->readMemory(location)));
            cyclesTaken = 5;
            break;
        case RRA_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, RRA(memory->readMemory(location)));
            cyclesTaken = 6;
            break;
        case RRA_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, RRA(memory->readMemory(location16)));
            cyclesTaken = 6;
            break;
        case RRA_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            memory->writeMemory(location16, RRA(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case RRA_ABY:
            b1 = nextByte();
            location16 = memory->AB(rY, b1, nextByte());
            memory->writeMemory(location16, RRA(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case RRA_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, RRA(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
        case RRA_INY:
            location16 = memory->INdY(rY, nextByte());
            memory->writeMemory(location16, RRA(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
            // SLO
        case SLO_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, SLO(memory->readMemory(location)));
            cyclesTaken = 5;
            break;
        case SLO_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, SLO(memory->readMemory(location)));
            cyclesTaken = 6;
            break;
        case SLO_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, SLO(memory->readMemory(location16)));
            cyclesTaken = 6;
            break;
        case SLO_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            memory->writeMemory(location16, SLO(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case SLO_ABY:
            b1 = nextByte();
            location16 = memory->AB(rY, b1, nextByte());
            memory->writeMemory(location16, SLO(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case SLO_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, SLO(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
        case SLO_INY:
            location16 = memory->INdY(rY, nextByte());
            memory->writeMemory(location16, SLO(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
        case SRE_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, SRE(memory->readMemory(location)));
            cyclesTaken = 5;
            break;
        case SRE_ZPX:
            location = memory->ZP(nextByte(), rX);
            memory->writeMemory(location, SRE(memory->readMemory(location)));
            cyclesTaken = 6;
            break;
        case SRE_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, SRE(memory->readMemory(location16)));
            cyclesTaken = 6;
            break;
        case SRE_ABX:
            b1 = nextByte();
            location16 = memory->AB(rX, b1, nextByte());
            memory->writeMemory(location16, SRE(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case SRE_ABY:
            b1 = nextByte();
            location16 = memory->AB(rY, b1, nextByte());
            memory->writeMemory(location16, SRE(memory->readMemory(location16)));
            cyclesTaken = 7;
            break;
        case SRE_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, SRE(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
        case SRE_INY:
            location16 = memory->INdY(rY, nextByte());
            memory->writeMemory(location16, SRE(memory->readMemory(location16)));
            cyclesTaken = 8;
            break;
            // TOP (Triple NOP)
        case TOP1:
            b1 = nextByte();
            memory->readMemory(memory->AB(b1, nextByte()));
            cyclesTaken = 4;
            break;
        case TOP2:
        case TOP3:
        case TOP4:
        case TOP5:
        case TOP6:
        case TOP7:
            b1 = nextByte();
            memory->readMemory(memory->AB(rX, b1, nextByte()));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
            // SAX
        case SAX_ZP:
            location = memory->ZP(nextByte());
            memory->writeMemory(location, SAX());
            cyclesTaken = 3;
            break;
        case SAX_ZPY:
            location = memory->ZP(nextByte(), rY);
            memory->writeMemory(location, SAX());
            cyclesTaken = 4;
            break;
        case SAX_INX:
            location16 = memory->INdX(rX, nextByte());
            memory->writeMemory(location16, SAX());
            cyclesTaken = 6;
            break;
        case SAX_AB:
            b1 = nextByte();
            location16 = memory->AB(b1, nextByte());
            memory->writeMemory(location16, SAX());
            cyclesTaken = 4;
            break;
            // LAX
        case LAX_ZP:
            LAX(memory->readMemory(memory->ZP(nextByte())));
            cyclesTaken = 3;
            break;
        case LAX_ZPY:
            LAX(memory->readMemory(memory->ZP(nextByte(), rY)));
            cyclesTaken = 4;
            break;
        case LAX_AB:
            b1 = nextByte();
            LAX(memory->readMemory(memory->AB(b1, nextByte())));
            cyclesTaken = 4;
            break;
        case LAX_ABY:
            b1 = nextByte();
            LAX(memory->readMemory(memory->AB(rY, b1, nextByte())));
            cyclesTaken = 4 + memory->pageBoundaryPassed;
            break;
        case LAX_INX:
            LAX(memory->readMemory(memory->INdX(rX, nextByte())));
            cyclesTaken = 6;
            break;
        case LAX_INY:
            LAX(memory->readMemory(memory->INdY(rY, nextByte())));
            cyclesTaken = 5 + memory->pageBoundaryPassed;
            break;
            // SBC
        case SBC_IMM1:
            rA = SBC(nextByte());
            cyclesTaken = 2;
            break;
        default:
            if (getInstructionName(opcode) != "UNKNOWN-OPCODE")
                std::cout << "CPU-Error: Handler not yet implemented: " << getInstructionName(opcode) << " at: " << (int) programCounter
                          << std::endl;
            else
                std::cout << "CPU-Error: Unknown opcode: $" << std::hex << (int) opcode << " at: " << (int) programCounter
                          << std::endl;

            state = CPUState::Error;
            break;
    }

    // Print the flag values of the CPU before the instructions were executed
#ifdef PRINTCPUSTATUS

    // Align the text because I get irritated when it isn't done.
/*	if (dataOffset == 1)
        std::cout<<"    "<<"    "<<"    ";
    else if (dataOffset == 2)
        std::cout<<"        ";
*/
if (currentInstruction)
{
    if (dataOffset == 2)
        std::cout<<"    ";
    if (dataOffset == 1)
        std::cout<<"        ";
    if (dataOffset == 0)
        std::cout<<"            ";

    std::cout<<cpustatestring.str();
    }
#endif

    // If we have not jumped or branched, increment the programCounter
    if (jumpOffset != 0) {
        programCounter = jumpOffset; // If jumpOffset is not 0, the programCounter will automatically move there for the next cycle - use this for jmp and branch operations.
    }

    // reset the dataOffset
    dataOffset = 0;

    // Reduce the remaining cycles variable as we've just done one (put this outside an if statement later to enable cycle accuracy when it is implemented).
    cpuCycles += cyclesTaken;

    // Return the number of cycles the CPU has gone through to the main emulator object
    return cyclesTaken;
}

void CPU6502::fPLP(unsigned char value) {
// Bits 4 and 5 should be ignored, so we need to set the status register manually
    SetFlag(Flag::Carry, GetFlag(Flag::Carry, value));
    SetFlag(Flag::Zero, GetFlag(Flag::Zero, value));
    SetFlag(Flag::EInterrupt, GetFlag(Flag::EInterrupt, value));
    SetFlag(Flag::BCDMode, GetFlag(Flag::BCDMode, value));
    SetFlag(Flag::Overflow, GetFlag(Flag::Overflow, value));
    SetFlag(Flag::Sign, GetFlag(Flag::Sign, value));
}

void CPU6502::fPHP() {
    // Push the status register to the stack. Bit 4 should be set (only in the value pushed to the stack) if pushed by PHP or BRK.
    // If an interrupt, it should be clear.
    unsigned char pushflags = flagRegister;
    pushflags = SetBit(4, 1, pushflags);
    pushStack8(pushflags);
}

void CPU6502::fRTS() {
    // Return from a subroutine
    unsigned char lo = popStack();
    unsigned char hi = popStack();
    unsigned short location = (hi << 8) + lo;
    JMP(location + 1); // Add 1 as what should have been pushed to the stack when JSR was called was address-1
}

void CPU6502::fRTI() {
    // Return from an interrupt handler
    unsigned char flags = popStack();
    fPLP(flags);
    unsigned char lo = popStack();
    unsigned char hi = popStack();
    unsigned short location = (hi << 8) + lo;
    JMP(location); // Unlike with RTS, the pushed address contains the actual address we need to jump back to.
}

void CPU6502::fBRK() {
    // Push the current PC + 2 to the stack, and then JMP to tbe BRK vector ($FFFE)
    nextByte(); // Fetch garbage byte to increment the PC
    // Immediately handle BRK interrupt
    HandleInterrupt(CPUInterrupt::iBRK);
}

void CPU6502::FireInterrupt(int type) {
    // Fire an interrupt to be picked up by the CPU
    if (type == CPUInterrupt::iNMI)
        fireNMI = true;
}

void CPU6502::checkInterrupts() {
    // Check for interrupts and react as neccesary
    if (fireNMI) {// NMI has highest priority and cannot be ignored
        HandleInterrupt(CPUInterrupt::iNMI);
        //std::cout<<"CPU:VBLANK Interrupt"<<std::endl;
    } else {
        // Handle everything else
        if (GetFlag(Flag::EInterrupt)) { // Only check when this flag is set
            if (memory->checkIRQ()) {
                HandleInterrupt(CPUInterrupt::iIRQ);
            }
        } // Don't bother handling reset in this function, will just code this directly later
    }
}

void CPU6502::BIT(unsigned char value) {
    SetFlag(Flag::Sign, (value >> 7)); // Set S flag to bit 7
    SetFlag(Flag::Overflow, (unsigned char) (value << 1) >> 7); // Set V flag to bit 6
    SetFlag(Flag::Zero, !(rA & value));
}

void CPU6502::CMP(unsigned char registerValue, unsigned char value) {
    SetFlag(Flag::Carry, registerValue >= value);
    SetFlag(Flag::Zero, registerValue == value);
    unsigned char result = (registerValue - value);
    SetFlag(Flag::Sign, result > 0x7F);
}

void CPU6502::JMP(unsigned short location) {
    // Jump to the specified address
    jumpOffset = location;
}

void CPU6502::branch(bool value) {
    if (value) {
        unsigned char branchloc = nextByte();
        unsigned char branchloc1 = branchloc;
        unsigned short oldpc = programCounter;

        //Get the sign of the value
        bool sign = (branchloc > 0x7F);

        if (sign) {
            branchloc = ~branchloc;
            branchloc++;
            unsigned short bloc1 = branchloc;
            programCounter -= (bloc1);
        } else {
            programCounter += branchloc;
        }

        // Dataoffset should = the byte after the instruction
        if ((programCounter >> 8) != (oldpc >> 8)) {
            pageBoundaryPassed = true;
        } else {
            pageBoundaryPassed = false; // reset it otherwise so that the following instructions don't take too long
        }

        cyclesTaken = 3 + pageBoundaryPassed;
    } else {
        cyclesTaken = 2;
        nextByte(); // Skip the next byte as it is data for the branch instruction.
    }
}

void CPU6502::printCPUStatus(std::string instructionName) {
    std::cout << std::uppercase << std::hex << (int) programCounter << "            " << instructionName;


    std::cout << "      A:" << decToHex(rA) << " X:" << decToHex(rX) << " Y:" << decToHex(rY) << " P:"
              << (int) flagRegister << " SP:" << (int) stackPointer << " CPUC:" << std::dec << cpuCycles << std::hex << std::endl;
}

std::string CPU6502::decToHex(int value) {
    // Displays a hex number formatted to match the other emulator's log better
    std::stringstream s;

    if (value < 0xF) {
        s << std::uppercase << std::hex << "0" << value;
    } else {
        s << std::uppercase << std::hex << value;
    }

    return s.str();
}

// Used for unit testing purposes...
unsigned char CPU6502::GetFlags() {
    return flagRegister;
}

unsigned char CPU6502::GetAcc() {
    return rA;
}

unsigned char CPU6502::GetX() {
    return rX;
}

unsigned char CPU6502::GetY() {
    return rY;
}

std::string CPU6502::getInstructionName(unsigned char opcode) {
    // Used for debugging purposes, just spits out the name of the current opcode - makes it easier to compare CPU logs with other emulators for debugging.
    std::string RetVal;
    switch (opcode) {
        case BRK:
            RetVal = "BRK    ";
            break;
        case ADC_IMM:
            RetVal = "ADC_IMM";
            break;
        case ADC_ZP:
            RetVal = "ADC_ZP ";
            break;
        case ADC_ZPX:
            RetVal = "ADC_ZPX";
            break;
        case ADC_AB:
            RetVal = "ADC_AB ";
            break;
        case ADC_ABX:
            RetVal = "ADC_ABX";
            break;
        case ADC_ABY:
            RetVal = "ADC_ABY";
            break;
        case ADC_INX:
            RetVal = "ADC_INX";
            break;
        case ADC_INY:
            RetVal = "ADC_INY";
            break;
        case AND_IMM:
            RetVal = "AND_IMM";
            break;
        case AND_ZP:
            RetVal = "AND_ZP ";
            break;
        case AND_ZPX:
            RetVal = "AND_ZPX";
            break;
        case AND_AB:
            RetVal = "AND_AB ";
            break;
        case AND_ABX:
            RetVal = "AND_ABX";
            break;
        case AND_ABY:
            RetVal = "AND_ABY";
            break;
        case AND_INX:
            RetVal = "AND_INX";
            break;
        case AND_INY:
            RetVal = "AND_INY";
            break;
        case ASL_ACC:
            RetVal = "ASL_ACC";
            break;
        case ASL_ZP:
            RetVal = "ASL_ZP ";
            break;
        case ASL_ZPX:
            RetVal = "ASL_ZPX";
            break;
        case ASL_AB:
            RetVal = "ASL_AB ";
            break;
        case ASL_ABX:
            RetVal = "ASL_ABX";
            break;
        case BCC:
            RetVal = "BCC    ";
            break;
        case BCS:
            RetVal = "BCS    ";
            break;
        case BEQ:
            RetVal = "BEQ    ";
            break;
        case BMI:
            RetVal = "BMI    ";
            break;
        case BNE:
            RetVal = "BNE    ";
            break;
        case BPL:
            RetVal = "BPL    ";
            break;
        case BVC:
            RetVal = "BVC    ";
            break;
        case BVS:
            RetVal = "BVS    ";
            break;
        case BIT_ZP:
            RetVal = "BIT_ZP ";
            break;
        case BIT_AB:
            RetVal = "BIT_AB ";
            break;
        case CLC:
            RetVal = "CLC    ";
            break;
        case CLD:
            RetVal = "CLD    ";
            break;
        case CLI:
            RetVal = "CLI    ";
            break;
        case CLV:
            RetVal = "CLV    ";
            break;
        case CMP_IMM:
            RetVal = "CMP_IMM";
            break;
        case CMP_ZP:
            RetVal = "CMP_ZP ";
            break;
        case CMP_ZPX:
            RetVal = "CMP_ZPX";
            break;
        case CMP_AB:
            RetVal = "CMP_AB ";
            break;
        case CMP_ABX:
            RetVal = "CMP_ABX";
            break;
        case CMP_ABY:
            RetVal = "CMP_ABY";
            break;
        case CMP_INX:
            RetVal = "CMP_INX";
            break;
        case CMP_INY:
            RetVal = "CMP_INY";
            break;
        case CPX_IMM:
            RetVal = "CPX_IMM";
            break;
        case CPX_ZP:
            RetVal = "CPX_ZP ";
            break;
        case CPX_AB:
            RetVal = "CPX_AB ";
            break;
        case CPY_IMM:
            RetVal = "CPY_IMM";
            break;
        case CPY_ZP:
            RetVal = "CPY_ZP ";
            break;
        case CPY_AB:
            RetVal = "CPY_AB ";
            break;
        case DEC_ZP:
            RetVal = "DEC_ZP ";
            break;
        case DEC_ZPX:
            RetVal = "DEC_ZPX";
            break;
        case DEC_AB:
            RetVal = "DEC_AB ";
            break;
        case DEC_ABX:
            RetVal = "DEC_ABX";
            break;
        case DEX:
            RetVal = "DEX    ";
            break;
        case DEY:
            RetVal = "DEY    ";
            break;
        case EOR_IMM:
            RetVal = "EOR_IMM";
            break;
        case EOR_ZP:
            RetVal = "EOR_ZP ";
            break;
        case EOR_ZPX:
            RetVal = "EOR_ZPX";
            break;
        case EOR_AB:
            RetVal = "EOR_AB ";
            break;
        case EOR_ABX:
            RetVal = "EOR_ABX";
            break;
        case EOR_ABY:
            RetVal = "EOR_ABY";
            break;
        case EOR_INX:
            RetVal = "EOR_INX";
            break;
        case EOR_INY:
            RetVal = "EOR_INY";
            break;
        case INC_ZP:
            RetVal = "INC_ZP ";
            break;
        case INC_ZPX:
            RetVal = "INC_ZPX";
            break;
        case INC_AB:
            RetVal = "INC_AB ";
            break;
        case INC_ABX:
            RetVal = "INC_ABX";
            break;
        case INX:
            RetVal = "INX    ";
            break;
        case INY:
            RetVal = "INY    ";
            break;
        case JMP_AB:
            RetVal = "JMP_AB ";
            break;
        case JMP_IN:
            RetVal = "JMP_IN ";
            break;
        case JSR:
            RetVal = "JSR    ";
            break;
        case LDA_IMM:
            RetVal = "LDA_IMM";
            break;
        case LDA_ZP:
            RetVal = "LDA_ZP ";
            break;
        case LDA_ZPX:
            RetVal = "LDA_ZPX";
            break;
        case LDA_AB:
            RetVal = "LDA_AB ";
            break;
        case LDA_ABX:
            RetVal = "LDA_ABX";
            break;
        case LDA_ABY:
            RetVal = "LDA_ABY";
            break;
        case LDA_INX:
            RetVal = "LDA_INX";
            break;
        case LDA_INY:
            RetVal = "LDA_INY";
            break;
        case LDX_IMM:
            RetVal = "LDX_IMM";
            break;
        case LDX_ZP:
            RetVal = "LDX_ZP ";
            break;
        case LDX_ZPY:
            RetVal = "LDX_ZPY";
            break;
        case LDX_AB:
            RetVal = "LDX_AB ";
            break;
        case LDX_ABY:
            RetVal = "LDX_ABY";
            break;
        case LDY_IMM:
            RetVal = "LDY_IMM";
            break;
        case LDY_ZP:
            RetVal = "LDY_ZP ";
            break;
        case LDY_ZPX:
            RetVal = "LDY_ZPX";
            break;
        case LDY_AB:
            RetVal = "LDY_AB ";
            break;
        case LDY_ABX:
            RetVal = "LDY_ABX";
            break;
        case LSR_ACC:
            RetVal = "LSR_A  ";
            break;
        case LSR_ZP:
            RetVal = "LSR_ZP ";
            break;
        case LSR_ZPX:
            RetVal = "LSR_ZPX";
            break;
        case LSR_AB:
            RetVal = "LSR_AB ";
            break;
        case LSR_ABX:
            RetVal = "LSR_ABX";
            break;
        case NOP:
            RetVal = "NOP    ";
            break;
        case ORA_IMM:
            RetVal = "ORA_IMM";
            break;
        case ORA_ZP:
            RetVal = "ORA_ZP ";
            break;
        case ORA_ZPX:
            RetVal = "ORA_ZPX";
            break;
        case ORA_AB:
            RetVal = "ORA_AB ";
            break;
        case ORA_ABX:
            RetVal = "ORA_ABX";
        case ORA_ABY:
            RetVal = "ORA_ABY";
            break;
        case ORA_INX:
            RetVal = "ORA_INX";
            break;
        case ORA_INY:
            RetVal = "ORA_INY";
            break;
        case PHA:
            RetVal = "PHA    ";
            break;
        case PHP:
            RetVal = "PHP    ";
            break;
        case PLA:
            RetVal = "PLA    ";
            break;
        case PLP:
            RetVal = "PLP    ";
            break;
        case ROL_ACC:
            RetVal = "ROL_ACC";
            break;
        case ROL_ZP:
            RetVal = "ROL_ZP ";
            break;
        case ROL_ZPX:
            RetVal = "ROL_ZPX";
            break;
        case ROL_AB:
            RetVal = "ROL_AB ";
            break;
        case ROL_ABX:
            RetVal = "ROL_ABX";
            break;
        case ROR_ACC:
            RetVal = "ROR_ACC";
            break;
        case ROR_ZP:
            RetVal = "ROR_ZP ";
            break;
        case ROR_ZPX:
            RetVal = "ROR_ZPX";
            break;
        case ROR_AB:
            RetVal = "ROR_AB ";
            break;
        case ROR_ABX:
            RetVal = "ROR_ABX";
            break;
        case RTI:
            RetVal = "RTI    ";
            break;
        case RTS:
            RetVal = "RTS    ";
            break;
        case SBC_IMM:
            RetVal = "SBC_IMM";
            break;
        case SBC_ZP:
            RetVal = "SBC_ZP ";
            break;
        case SBC_ZPX:
            RetVal = "SBC_ZPX";
            break;
        case SBC_AB:
            RetVal = "SBC_AB ";
            break;
        case SBC_ABX:
            RetVal = "SBC_ABX";
            break;
        case SBC_ABY:
            RetVal = "SBC_ABY";
            break;
        case SBC_INX:
            RetVal = "SBC_INX";
            break;
        case SBC_INY:
            RetVal = "SBC_INY";
            break;
        case SEC:
            RetVal = "SEC    ";
            break;
        case SED:
            RetVal = "SED    ";
            break;
        case SEI:
            RetVal = "SEI    ";
            break;
        case STA_ZP:
            RetVal = "STA_ZP ";
            break;
        case STA_ZPX:
            RetVal = "STA_ZPX";
            break;
        case STA_AB:
            RetVal = "STA_AB ";
            break;
        case STA_ABX:
            RetVal = "STA_ABX";
            break;
        case STA_ABY:
            RetVal = "STA_ABY";
            break;
        case STA_INX:
            RetVal = "STA_INX";
            break;
        case STA_INY:
            RetVal = "STA_INY";
            break;
        case STX_ZP:
            RetVal = "STX_ZP ";
            break;
        case STX_ZPY:
            RetVal = "STX_ZPY";
            break;
        case STX_AB:
            RetVal = "STX_AB ";
            break;
        case STY_ZP:
            RetVal = "STY_ZP ";
            break;
        case STY_ZPX:
            RetVal = "STY_ZPX";
            break;
        case STY_AB:
            RetVal = "STY_AB ";
            break;
        case TAX:
            RetVal = "TAX    ";
            break;
        case TAY:
            RetVal = "TAY    ";
            break;
        case TSX:
            RetVal = "TSX    ";
            break;
        case TXA:
            RetVal = "TXA    ";
            break;
        case TXS:
            RetVal = "TXS    ";
            break;
        case TYA:
            RetVal = "TYA    ";
            break;
            // Undocumented opcodes from here on out
        case DCP_ZP:
            RetVal = "DCP_ZP";
            break;
        case DCP_ZPX:
            RetVal = "DCP_ZPX";
            break;
        case DCP_AB:
            RetVal = "DCP_AB";
            break;
        case DCP_ABX:
            RetVal = "DCP_ABX";
            break;
        case DCP_ABY:
            RetVal = "DCP_ABY";
            break;
        case DCP_INX:
            RetVal = "DCP_INX";
            break;
        case DCP_INY:
            RetVal = "DCP_INY";
            break;
        case ISB_ZP:
            RetVal = "ISB_ZP ";
            break;
        case ISB_ZPX:
            RetVal = "ISB_ZPX";
            break;
        case ISB_AB:
            RetVal = "ISB_AB ";
            break;
        case ISB_ABX:
            RetVal = "ISB_ABX";
            break;
        case ISB_ABY:
            RetVal = "ISB_ABY";
            break;
        case ISB_INX:
            RetVal = "ISB_INX";
            break;
        case ISB_INY:
            RetVal = "ISB_INY";
            break;
        case NOP1:
        case NOP2:
        case NOP3:
        case NOP4:
        case NOP5:
        case NOP6:
            RetVal = "*NOP   ";
            break;
        case DOP1:
        case DOP2:
        case DOP3:
        case DOP4:
        case DOP5:
        case DOP6:
        case DOP7:
        case DOP8:
        case DOP9:
        case DOP10:
        case DOP11:
        case DOP12:
        case DOP13:
        case DOP14:
            RetVal = "DOP    ";
            break;
        case TOP1:
        case TOP2:
        case TOP3:
        case TOP4:
        case TOP5:
        case TOP6:
        case TOP7:
            RetVal = "TOP    ";
            break;
        case SBC_IMM1:
            RetVal = "SBC_IMM";
            break;
        case SAX_ZP:
            RetVal = "SAX_ZP ";
            break;
        case SAX_ZPY:
            RetVal = "SAX_ZPY";
            break;
        case SAX_INX:
            RetVal = "SAX_INX";
            break;
        case SAX_AB:
            RetVal = "SAX_AB";
            break;
        case LAX_ZP:
            RetVal = "LAX_ZP ";
            break;
        case LAX_ZPY:
            RetVal = "LAX_ZPY";
            break;
        case LAX_AB:
            RetVal = "LAX_AB ";
            break;
        case LAX_ABY:
            RetVal = "LAX_ABY";
            break;
        case LAX_INX:
            RetVal = "LAX_INX";
            break;
        case LAX_INY:
            RetVal = "LAX_INY";
            break;
        case RLA_ZP:
            RetVal = "RLA_ZP ";
            break;
        case RLA_ZPX:
            RetVal = "RLA_ZPX";
            break;
        case RLA_AB:
            RetVal = "RLA_AB ";
            break;
        case RLA_ABX:
            RetVal = "RLA_ABX";
            break;
        case RLA_ABY:
            RetVal = "RLA_ABY";
            break;
        case RLA_INX:
            RetVal = "RLA_INX";
            break;
        case RLA_INY:
            RetVal = "RLA_INY";
            break;
        case RRA_ZP:
            RetVal = "RRA_ZP ";
            break;
        case RRA_ZPX:
            RetVal = "RRA_ZPX";
            break;
        case RRA_AB:
            RetVal = "RRA_AB ";
            break;
        case RRA_ABX:
            RetVal = "RRA_ABX";
            break;
        case RRA_ABY:
            RetVal = "RRA_ABY";
            break;
        case RRA_INX:
            RetVal = "RRA_INX";
            break;
        case RRA_INY:
            RetVal = "RRA_INY";
            break;
        case SLO_ZP:
            RetVal = "SLO_ZP ";
            break;
        case SLO_ZPX:
            RetVal = "SLO_ZPX";
            break;
        case SLO_AB:
            RetVal = "SLO_AB ";
            break;
        case SLO_ABX:
            RetVal = "SLO_ABX";
            break;
        case SLO_ABY:
            RetVal = "SLO_ABY";
            break;
        case SLO_INX:
            RetVal = "SLO_INX";
            break;
        case SLO_INY:
            RetVal = "SLO_INY";
            break;
        case SRE_ZP:
            RetVal = "SRE_ZP ";
            break;
        case SRE_ZPX:
            RetVal = "SRE_ZPX";
            break;
        case SRE_AB:
            RetVal = "SRE_AB ";
            break;
        case SRE_ABX:
            RetVal = "SRE_ABX";
            break;
        case SRE_ABY:
            RetVal = "SRE_ABY";
            break;
        case SRE_INX:
            RetVal = "SRE_INX";
            break;
        case SRE_INY:
            RetVal = "SRE_INY";
            break;
        default:
            RetVal = "UNKNOWN-OPCODE";
            break;
    }

    return RetVal;
}

void CPU6502::pushStack8(unsigned char value) {
    memory->writeMemory(0x100 + (stackPointer--), value);
}

void CPU6502::pushStack16(unsigned short value) {
    pushStack8(value >> 8);
    pushStack8(value);
}

unsigned char CPU6502::popStack() {
    return memory->readMemory(0x100 + (++stackPointer));
}

void CPU6502::HandleInterrupt(int type) {
    // Forces the CPU to jump to an interrupt vector. may be called be a CPU instruction or piece of emulated hardware

    /* Vectors:
    NMI: $FFFA/$FFFB
    RESET: $FFFC/$FFFD
    IRQ/BRK: $FFFE/$FFFF
    */

    unsigned char pushflags;
    unsigned short TargetAddress;
    switch (type) {
        case CPUInterrupt::iReset:
            // Push the program counter
            pushStack16(programCounter);
            // Push the flag register
            pushflags = flagRegister;
            pushflags = SetBit(4, 0, pushflags); // Set bit 4 to 0 if not from a CPU instruction
            pushStack8(pushflags);
            // Jump to the RESET vector
            JMP((memory->readMemory(0xFFFD) * 256) + memory->readMemory(0xFFFC));
            break;
        case CPUInterrupt::iNMI:
            // Push the program counter
            pushStack16(programCounter);
            // Push the flag register
            pushflags = flagRegister;
            pushflags = SetBit(4, 0, pushflags); // Set bit 4 to 0 if not from a CPU instruction
            pushStack8(pushflags);
            // Jump to the NMI vector
            TargetAddress = (memory->readMemory(0xFFFB) * 256) + memory->readMemory(0xFFFA);
            //JMP((memory->readMemory(0xFFFB) * 256) + memory->readMemory(0xFFFA));
            programCounter = TargetAddress;
            // Set the NMI Flip-flop back to false
#ifdef PRINTCPUSTATUS
            std::cout<<"INTERRUPT_NMI"<<std::endl;
#endif
            fireNMI = false;
            break;
        case CPUInterrupt::iIRQ:
            // Push the program counter
            pushStack16(programCounter);
            // Push the flag register
            pushflags = flagRegister;
            pushflags = SetBit(4, 0, pushflags); // Set bit 4 to 0 if not from a CPU instruction
            pushStack8(pushflags);
            // Jump to the BRK/IRQ vector
            JMP((memory->readMemory(0xFFFF) * 256) + memory->readMemory(0xFFFE));
            break;
        case CPUInterrupt::iBRK:
            // Push the program counter
            pushStack16(programCounter);
            // Push the flag register
            pushflags = flagRegister;
            pushflags = SetBit(4, 1, pushflags);
            pushStack8(pushflags);
            // Jump to the BRK/IRQ vector
            JMP((memory->readMemory(0xFFFF) * 256) + memory->readMemory(0xFFFE));
            break;
    }

    interruptProcessed = true;
}
