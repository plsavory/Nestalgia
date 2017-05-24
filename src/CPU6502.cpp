#include <iostream>
#include "MemoryManager.h"
#include "CPU6502.h"
#include <cmath>

#define PRINTCPUSTATUS

CPU6502::CPU6502(MemoryManager &mManager)
{
	myState = CPUState::Running;
	mainMemory = &mManager;
	FlagRegister = 0x0; // Initialize the flags register
	rA, rX, rY = 0x0;
	pc = 0x0;
	myState = CPUState::Running;
}


CPU6502::~CPU6502()
{
}


void CPU6502::Reset()
{
	// Should always be called before starting emulation (Sets the program counter to the appropriate place)
	myState = CPUState::Running;
	FlagRegister = 0x0; // Initialize the flags register
	rX = 0x0;
	rY = 0x0;
	rA = 0x0;
	pc = (mainMemory->ReadMemory(0xFFFD) * 256) + mainMemory->ReadMemory(0xFFFC);
	sp = 0xFF; // Set the stack pointer

	myState = CPUState::Running;
}

void CPU6502::SetFlag(Flag flag, bool val)
{
	// Sets a specific flag to true or false depending on the value of "val"
	if (val)
		FlagRegister |= flag;
	else
		FlagRegister &= ~(flag);
}

bool CPU6502::GetFlag(Flag flag)
{
	// Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
	if (FlagRegister & flag)
		return true;
	else
		return false;
}

unsigned char CPU6502::AND(unsigned char value)
{
	// AND the value with the accumulator, and then set the flags accordingly and return the result.
	unsigned char result = value & rA;
	SetFlag(Flag::Zero, value == 0x0);
	SetFlag(Flag::Sign, value > 0x7F);
	return result;
}

unsigned char CPU6502::ASL(unsigned char value)
{
	unsigned char result = value << 1;
	SetFlag(Flag::Carry, (value&(1 << 7)));
	SetFlag(Flag::Sign, (result&(1 << 7)));
	SetFlag(Flag::Zero, result == 0x0);
	return result;
}

unsigned char CPU6502::LSR(unsigned char value)
{
	unsigned char result = value >> 1;
	SetFlag(Flag::Carry, (value&(1 << 0)));
	SetFlag(Flag::Sign, (result&(1 << 7)));
	SetFlag(Flag::Zero, result == 0x0);
	return result;
}

unsigned char CPU6502::ROL(unsigned char value)
{
	unsigned char result = value << 1;

	// Shift carry onto bit 0 and shift the original bit 7 onto the carry

	if (GetFlag(Flag::Carry))
		result |= 1 << 0;
	else
		result &= ~(1 << 0);

	SetFlag(Flag::Carry, value&(1 << 7));


	return result;
}
unsigned char CPU6502::ROR(unsigned char value)
{
	unsigned char result = value >> 1;
	return result;
}

unsigned char CPU6502::LD(unsigned char value)
{
	// Sets the flag register as an LD operation should and then simply returns the value.

	// If value == 0, set zero flag, otherwise clear it.
	SetFlag(Flag::Zero, value == 0x0);
	// If value > 127, set the sign flag so that the 6502 program knows that the number is negative, otherwise clear it.
	SetFlag(Flag::Sign, value > 0x7F);

	return value;
}

unsigned char CPU6502::ADC(unsigned char value)
{
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

	if ((std::signbit(rA) == std::signbit(value)) && (std::signbit(rA) != std::signbit(OperationResult)))
		SetFlag(Flag::Overflow,1);
	else
		SetFlag(Flag::Overflow,0);

	// Set the zero flag accordingly
	SetFlag(Flag::Zero, RetVal == 0);

	// Set the Sign flag accordingly (most significant bit of the 8-bit result)
	SetFlag(Flag::Sign, RetVal > 0x7F);

	/* Set the Carry flag accordingly - should be set if the result of the operation is > 255,
	 in this case this becomes 511 but it is up to the program running inside the CPU how it represents this. */
	SetFlag(Flag::Carry,OperationResult > 0xFF);
	return RetVal;
}

unsigned char CPU6502::SBC(unsigned char value)
{
	return ADC(~value); // Lazy lol - but it works.
}

unsigned char CPU6502::NB()
{
	// Get the value of the next byte from Memory
	return mainMemory->ReadMemory(pc+(dataoffset++));
}

void CPU6502::Execute()
{
	// Debug reasons
	std::string currentInst = "";

	// Handle interrupts if neccesary (add this later)

	// Fetch the next opcode
	unsigned char opcode = NB();
	jumpoffset = 0;

	// Attempt to execute the opcode
	switch (opcode)
	{
		// LD_IMM instructions
		case LDA_IMM:
			rA = LD(NB());
			CyclesRemain = 2;
			currentInst = "LDA_IMM";
		break;
		case LDX_IMM:
			rX = LD(NB());
			CyclesRemain = 2;
			currentInst = "LDX_IMM";
		break;
		case LDY_IMM:
			rY = LD(NB());
			CyclesRemain = 2;
			currentInst = "LDY_IMM";
		break;
		// Transfer instructions
		case TAX:
			rX = LD(rA);
			CyclesRemain = 2;
			currentInst = "TAX";
		break;
		case TAY:
			rY = LD(rA);
			CyclesRemain = 2;
			currentInst = "TAY";
		break;
		case TXA:
			rA = LD(rX);
			CyclesRemain = 2;
			currentInst = "TXA";
		break;
		case TYA:
			rA = LD(rY);
			CyclesRemain = 2;
			currentInst = "TYA";
		break;
		case TSX:
			rX = LD(sp);
			CyclesRemain = 2;
			currentInst = "TSX";
		break;
		case TXS:
			sp = LD(rX);
			CyclesRemain = 2;
			currentInst = "TXS";
		break;
		// Clear Flag instructions
		case CLC:
			SetFlag(Flag::Carry,0);
			CyclesRemain = 2;
			currentInst = "CLC";
		break;
		case CLV:
			SetFlag(Flag::Overflow,0);
			CyclesRemain = 2;
			currentInst = "CLV";
		break;
		case CLD:
			SetFlag(Flag::BCDMode,0);
			CyclesRemain = 2;
			currentInst = "CLD";
		break;
		case CLI:
			SetFlag(Flag::EInterrupt,0);
			CyclesRemain = 2;
			currentInst = "CLI";
		break;
		// Set Flag instructions
		case SEC:
			SetFlag(Flag::Carry,1);
			CyclesRemain = 2;
			currentInst = "SEC";
		break;
		case SED:
			SetFlag(Flag::BCDMode,1);
			CyclesRemain = 2;
			currentInst = "SED";
		break;
		case SEI:
			SetFlag(Flag::EInterrupt,1);
			CyclesRemain = 2;
			currentInst = "SEI";
		break;
		default:
		std::cout<<"CPU-Error: Unknown opcode: $" << std::hex << (int)opcode << " at: "<<(int)pc<< std::endl;
		myState = CPUState::Error;
		break;
	}

	// Print the CPU state if the option is enabled

	#ifdef PRINTCPUSTATUS
	if (currentInst != "")
		PrintCPUStatus(currentInst);
	#endif

	// If we have not jumped or branched, increment the pc
	if (jumpoffset == 0)
		pc += dataoffset;
		else
		pc += jumpoffset; // If jumpoffset is not 0, the pc will automatically move there for the next cycle - use this for jmp and branch operations.

	// Reset the dataoffset
	dataoffset = 0;

	// Reduce the remaining cycles variable as we've just done one (put this outside an if statement later to enable cycle accuracy when it is implemented).
	CyclesRemain--;
}

void CPU6502::PrintCPUStatus(std::string inst_name)
{
	std::cout<<std::hex<<"--- pc: $"<<(int)pc<<" --- A: $"<<(int)rA<<" iX: $"<<(int)rX<<" iY: $"<<(int)rY<<" Flags: $"<<(int)FlagRegister<<" sp: $"<<(int)sp<<" --- "<<inst_name<<" --- "<<std::endl;
}

// Used for unit testing purposes...
unsigned char CPU6502::GetFlags()
{
	return FlagRegister;
}

unsigned char CPU6502::GetAcc()
{
	return rA;
}
unsigned char CPU6502::GetX()
{
	return rX;
}

unsigned char CPU6502::GetY()
{
	return rY;
}
