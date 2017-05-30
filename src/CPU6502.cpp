#include <iostream>
#include "MemoryManager.h"
#include "CPU6502.h"

#define PRINTCPUSTATUS

CPU6502::CPU6502(MemoryManager &mManager)
{
	myState = CPUState::Running;
	mainMemory = &mManager;
	FlagRegister = 0x24; // Initialize the flags register
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
	FlagRegister = 0x24; // Initialize the flags register - unused and I should be set to 1.
	rX = 0x0;
	rY = 0x0;
	rA = 0x0;
	//pc = (mainMemory->ReadMemory(0xFFFD) * 256) + mainMemory->ReadMemory(0xFFFC);
	pc = 0xC000;
	sp = 0xFD; // Set the stack pointer
	SetFlag(Flag::Unused,1);
	SetFlag(Flag::EInterrupt,1);
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

bool CPU6502::SignBit(unsigned char value)
{
	if (value & 1 << 7)
		return true;
	else
		return false;
}

bool CPU6502::GetFlag(Flag flag, unsigned char value)
{
	// Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
	if (value & flag)
		return true;
	else
		return false;
}

unsigned char CPU6502::ORA(unsigned char value) {
	// Inclusive OR between A and value. Save result in A. Set Zero and Negative flags appropriately
	unsigned char result = value | rA;
	SetFlag(Flag::Zero, result == 0x0);
	SetFlag(Flag::Sign, result > 0x7F);
	return result;
}

unsigned char CPU6502::EOR(unsigned char value) {
	unsigned char result = value ^ rA;
	SetFlag(Flag::Zero, result == 0x0);
	SetFlag(Flag::Sign, result > 0x7F);
	return result;
}

unsigned char CPU6502::AND(unsigned char value)
{
	// AND the value with the accumulator, and then set the flags accordingly and return the result.
	unsigned char result = value & rA;
	SetFlag(Flag::Zero, result == 0x0);
	SetFlag(Flag::Sign, result > 0x7F);
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

	if ((SignBit(rA) == SignBit(value)) && (SignBit(rA) != SignBit(OperationResult)))
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

unsigned char CPU6502::IN(unsigned char value) {
	unsigned char result = value+1;
	SetFlag(Flag::Zero, result == 0);
	SetFlag(Flag::Sign, result > 0x7F);
	return result;
}

unsigned char CPU6502::DE(unsigned char value) {
	unsigned char result = value-1;
	SetFlag(Flag::Zero, result == 0);
	SetFlag(Flag::Sign, result > 0x7F);
	return result;
}

unsigned char CPU6502::NB()
{
	// Get the value of the next byte from Memory
	return mainMemory->ReadMemory(pc+(dataoffset++));
}

void CPU6502::Execute()
{
	// Debug reasons
	unsigned char currentInst = mainMemory->ReadMemory(pc,1);

	// Print the CPU's current status
	#ifdef PRINTCPUSTATUS
	if (currentInst) // if not null
		PrintCPUStatus(InstName(currentInst));
	#endif

	// Handle interrupts if neccesary (add this later)

	// Fetch the next opcode
	unsigned char opcode = NB();
 	pboundarypassed = 0;
	jumpoffset = 0;

	// Attempt to execute the opcode
	switch (opcode)
	{
		// LD_ZP instructions
		case LDA_ZP:
			rA = LD(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case LDX_ZP:
			rX = LD(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case LDY_ZP:
			rY = LD(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		// LD_IMM instructions
		case LDA_IMM:
			rA = LD(NB());
			CyclesRemain = 2;
		break;
		case LDX_IMM:
			rX = LD(NB());
			CyclesRemain = 2;
		break;
		case LDY_IMM:
			rY = LD(NB());
			CyclesRemain = 2;
		break;
		// LD_AB instructions
		case LDA_AB:
			b1 = NB(); // Get first byte of next address
			rA = LD(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case LDX_AB:
			b1 = NB();
			rX = LD(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case LDY_AB:
			b1 = NB();
			rY = LD(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		// LD_ABX/Y instructions
		case LDA_ABX:
			b1 = NB();
			rA = LD(mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case LDA_ABY:
			b1 = NB();
			rA = LD(mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case LDX_ABY:
			b1 = NB();
			rX = LD(mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case LDY_ABX:
			b1 = NB();
			rY = LD(mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		// LD_ZPX instructions
		case LDA_ZPX:
			rA = LD(mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		case LDX_ZPY:
			rX = LD(mainMemory->ReadMemory(mainMemory->ZP(NB(),rY)));
			CyclesRemain = 4;
		break;
		case LDY_ZPX:
			rY = LD(mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		// LDA_IN instructions
		case LDA_INX:
			rA = LD(mainMemory->ReadMemory(mainMemory->INdX(rX,NB())));
			CyclesRemain = 6;
		break;
		case LDA_INY:
			rA = LD(mainMemory->ReadMemory(mainMemory->INdY(rY,NB())));
			CyclesRemain = 5+pboundarypassed;
		break;
		// AND instructions
		case AND_IMM:
			rA = AND(NB());
			CyclesRemain = 2;
		break;
		case AND_ZP:
			rA = AND(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case AND_ZPX:
			rA = AND(mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		case AND_AB:
			b1 = NB();
			rA = AND(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case AND_ABX:
			b1 = NB();
			rA = AND(mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case AND_ABY:
			b1 = NB();
			rA = AND(mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case AND_INX:
			rA = AND(mainMemory->ReadMemory(mainMemory->INdX(rX,NB())));
			CyclesRemain = 6;
		break;
		case AND_INY:
			rA = AND(mainMemory->ReadMemory(mainMemory->INdY(rY,NB())));
			CyclesRemain = 5+pboundarypassed;
		break;
		// ORA instructions
		case ORA_IMM:
			rA = ORA(NB());
			CyclesRemain = 2;
		break;
		case ORA_ZP:
			rA = ORA(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case ORA_ZPX:
			rA = ORA(mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		case ORA_AB:
			b1 = NB();
			rA = ORA(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case ORA_ABX:
			b1 = NB();
			rA = ORA(mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case ORA_ABY:
			b1 = NB();
			rA = ORA(mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case ORA_INX:
			rA = ORA(mainMemory->ReadMemory(mainMemory->INdX(rX,NB())));
			CyclesRemain = 6;
		break;
		case ORA_INY:
			rA = ORA(mainMemory->ReadMemory(mainMemory->INdY(rY,NB())));
			CyclesRemain = 5+pboundarypassed;
		break;
		// EOR instructions
		case EOR_IMM:
			rA = EOR(NB());
			CyclesRemain = 2;
		break;
		case EOR_ZP:
			rA = EOR(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case EOR_ZPX:
			rA = EOR(mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		case EOR_AB:
			b1 = NB();
			rA = EOR(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case EOR_ABX:
			b1 = NB();
			rA = EOR(mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case EOR_ABY:
			b1 = NB();
			rA = EOR(mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case EOR_INX:
			rA = EOR(mainMemory->ReadMemory(mainMemory->INdX(rX,NB())));
			CyclesRemain = 6;
		break;
		case EOR_INY:
			rA = EOR(mainMemory->ReadMemory(mainMemory->INdY(rY,NB())));
			CyclesRemain = 5+pboundarypassed;
		break;
		// ADC instructions
		case ADC_IMM:
			rA = ADC(NB());
			CyclesRemain = 2;
		break;
		case ADC_ZP:
			rA = ADC(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case ADC_ZPX:
			rA = ADC(mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		case ADC_AB:
			b1 = NB();
			rA = ADC(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case ADC_ABX:
			b1 = NB();
			rA = ADC(mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case ADC_ABY:
			b1 = NB();
			rA = ADC(mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case ADC_INX:
			rA = ADC(mainMemory->ReadMemory(mainMemory->INdX(rX,NB())));
			CyclesRemain = 6;
		break;
		case ADC_INY:
			rA = ADC(mainMemory->ReadMemory(mainMemory->INdY(rY,NB())));
			CyclesRemain = 5+pboundarypassed;
		break;
		// ABC instructions
		case SBC_IMM:
			rA = SBC(NB());
			CyclesRemain = 2;
		break;
		case SBC_ZP:
			rA = SBC(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case SBC_ZPX:
			rA = SBC(mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		case SBC_AB:
			b1 = NB();
			rA = SBC(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case SBC_ABX:
			b1 = NB();
			rA = SBC(mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case SBC_ABY:
			b1 = NB();
			rA = SBC(mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case SBC_INX:
			rA = SBC(mainMemory->ReadMemory(mainMemory->INdX(rX,NB())));
			CyclesRemain = 6;
		break;
		case SBC_INY:
			rA = SBC(mainMemory->ReadMemory(mainMemory->INdY(rY,NB())));
			CyclesRemain = 5+pboundarypassed;
		break;
		// Increment instructions
		case INX:
			rX = IN(rX);
			CyclesRemain = 2;
		break;
		case INY:
			rY = IN(rY);
			CyclesRemain = 2;
		break;
		case DEX:
			rX = DE(rX);
			CyclesRemain = 2;
		break;
		case DEY:
			rY = DE(rY);
			CyclesRemain = 2;
		break;
		// ASL instructions
		case ASL_ACC:
			rA = ASL(rA);
			CyclesRemain = 2;
		break;
		case ASL_ZP:
			location = mainMemory->ZP(NB());
			result = ASL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 5;
		break;
		case ASL_ZPX:
			location = mainMemory->ZP(NB(),rX);
			result = ASL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 6;
		break;
		case ASL_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			result = ASL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 6;
		break;
		case ASL_ABX:
			b1 = NB();
			location16 = mainMemory->AB(rX,b1,NB());
			result = ASL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 7;
		break;
		// LSR functions
		case LSR_ACC:
			rA = LSR(rA);
			CyclesRemain = 2;
		break;
		case LSR_ZP:
			location = mainMemory->ZP(NB());
			result = LSR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 5;
		break;
		case LSR_ZPX:
			location = mainMemory->ZP(NB(),rX);
			result = LSR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 6;
		break;
		case LSR_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			result = LSR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 6;
		break;
		case LSR_ABX:
			b1 = NB();
			location16 = mainMemory->AB(rX,b1,NB());
			result = LSR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 7;
		break;
		// ROL operations
		case ROL_ACC:
			rA = ROL(rA);
			CyclesRemain = 2;
		break;
		case ROL_ZP:
			location = mainMemory->ZP(NB());
			result = ROL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 5;
		break;
		case ROL_ZPX:
			location = mainMemory->ZP(NB(),rX);
			result = ROL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 6;
		break;
		case ROL_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			result = ROL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 6;
		break;
		case ROL_ABX:
			b1 = NB();
			location16 = mainMemory->AB(rX,b1,NB());
			result = ROL(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 7;
		break;
		case ROR_ACC:
			rA = ROR(rA);
			CyclesRemain = 2;
		break;
		case ROR_ZP:
			location = mainMemory->ZP(NB());
			result = ROR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 5;
		break;
		case ROR_ZPX:
			location = mainMemory->ZP(NB(),rX);
			result = ROR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 6;
		break;
		case ROR_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			result = ROR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 6;
		break;
		case ROR_ABX:
			b1 = NB();
			location16 = mainMemory->AB(rX,b1,NB());
			result = ROR(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 7;
		break;
		// INC & DEC Instructions
		case INC_ZP:
			location = mainMemory->ZP(NB());
			result = IN(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 5;
		break;
		case INC_ZPX:
			location = mainMemory->ZP(NB(),rX);
			result = IN(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 6;
		break;
		case INC_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			result = IN(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 6;
		break;
		case INC_ABX:
			b1 = NB();
			location16 = mainMemory->AB(rX,b1,NB());
			result = IN(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 7;
		break;
		case DEC_ZP:
			location = mainMemory->ZP(NB());
			result = DE(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 5;
		break;
		case DEC_ZPX:
			location = mainMemory->ZP(NB(),rX);
			result = DE(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location,result);
			CyclesRemain = 6;
		break;
		case DEC_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			result = DE(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 6;
		break;
		case DEC_ABX:
			b1 = NB();
			location16 = mainMemory->AB(rX,b1,NB());
			result = DE(mainMemory->ReadMemory(location));
			mainMemory->WriteMemory(location16,result);
			CyclesRemain = 7;
		break;
		// Store operations
		case STA_ZP:
			location = mainMemory->ZP(NB());
			mainMemory->WriteMemory(location,rA);
			CyclesRemain = 3;
		break;
		case STA_ZPX:
			location = mainMemory->ZP(NB(),rX);
			mainMemory->WriteMemory(location,rA);
			CyclesRemain = 4;
		break;
		case STA_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			mainMemory->WriteMemory(location16,rA);
			CyclesRemain = 4;
		break;
		case STA_ABX:
			b1 = NB();
			location16 = mainMemory->AB(rX,b1,NB());
			mainMemory->WriteMemory(location16,rA);
			CyclesRemain = 5;
		break;
		case STA_ABY:
			b1 = NB();
			location16 = mainMemory->AB(rY,b1,NB());
			mainMemory->WriteMemory(location16,rA);
			CyclesRemain = 5;
		break;
		case STA_INX:
			location16 = mainMemory->INdX(rX,NB());
			mainMemory->WriteMemory(location16,rA);
			CyclesRemain = 6;
		break;
		case STA_INY:
			location16 = mainMemory->INdY(rY,NB());
			mainMemory->WriteMemory(location16,rA);
			CyclesRemain = 6;
		break;
		case STX_ZP:
			location = mainMemory->ZP(NB());
			mainMemory->WriteMemory(location,rX);
			CyclesRemain = 3;
		break;
		case STX_ZPY:
			location = mainMemory->ZP(NB(),rY);
			mainMemory->WriteMemory(location,rX);
			CyclesRemain = 4;
		break;
		case STX_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			mainMemory->WriteMemory(location16,rX);
			CyclesRemain = 4;
		break;
		case STY_ZP:
			location = mainMemory->ZP(NB());
			mainMemory->WriteMemory(location,rY);
			CyclesRemain = 3;
		break;
		case STY_ZPX:
			location = mainMemory->ZP(NB(),rX);
			mainMemory->WriteMemory(location,rY);
			CyclesRemain = 4;
		break;
		case STY_AB:
			b1 = NB();
			location16 = mainMemory->AB(b1,NB());
			CyclesRemain = 4;
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
			CyclesRemain = 2;
		break;
		case TAY:
			rY = LD(rA);
			CyclesRemain = 2;
		break;
		case TXA:
			rA = LD(rX);
			CyclesRemain = 2;
		break;
		case TYA:
			rA = LD(rY);
			CyclesRemain = 2;
		break;
		case TSX:
			rX = LD(sp);
			CyclesRemain = 2;
		break;
		case TXS:
			sp = rX; // Does not effect flags
			CyclesRemain = 2;
		break;
		// Clear Flag instructions
		case CLC:
			SetFlag(Flag::Carry,0);
			CyclesRemain = 2;
		break;
		case CLV:
			SetFlag(Flag::Overflow,0);
			CyclesRemain = 2;
		break;
		case CLD:
			SetFlag(Flag::BCDMode,0);
			CyclesRemain = 2;
		break;
		case CLI:
			SetFlag(Flag::EInterrupt,0);
			CyclesRemain = 2;
		break;
		// Set Flag instructions
		case SEC:
			SetFlag(Flag::Carry,1);
			CyclesRemain = 2;
		break;
		case SED:
			SetFlag(Flag::BCDMode,1);
			CyclesRemain = 2;
		break;
		case SEI:
			SetFlag(Flag::EInterrupt,1);
			CyclesRemain = 2;
		break;
		// JMP instructions
		case JMP_AB:
			b1 = NB();
			JMP(mainMemory->AB(b1,NB()));
			CyclesRemain = 3;
		break;
		case JMP_IN:
		// Jump to the target address which is contained in the memory address after the byte.
		b1 = NB();
		JMP(mainMemory->IN(b1,NB()));
		CyclesRemain = 5;
		break;
		case JSR:
			b1 = NB();
			PushStack16(pc+2); // Push the location of the next instruction -1 to the stack
			JMP(mainMemory->AB(b1,NB()));
			CyclesRemain = 6;
		break;
		case RTS:
			fRTS();
			CyclesRemain = 6;
		break;
		case RTI:
			fRTI();
			CyclesRemain = 6;
			break;
		case BIT_ZP:
			BIT(mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case BIT_AB:
			b1 = NB();
			BIT(mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		// Stack operations
		case PHA:
			PushStack8(rA);
			CyclesRemain = 3;
		break;
		case PHP:
			PushStack8(FlagRegister);
			CyclesRemain = 3;
		break;
		case PLA:
			rA = LD(PopStack());
			CyclesRemain = 4;
		break;
		case PLP:
			fPLP(PopStack());
			CyclesRemain = 4;
		break;
		// CMP Instructions
		case CMP_IMM:
			CMP(rA,NB());
			CyclesRemain = 2;
		break;
		case CMP_ZP:
			CMP(rA,mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case CMP_ZPX:
			CMP(rA,mainMemory->ReadMemory(mainMemory->ZP(NB(),rX)));
			CyclesRemain = 4;
		break;
		case CMP_AB:
			b1 = NB();
			CMP(rA,mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case CMP_ABX:
			b1 = NB();
			CMP(rA,mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case CMP_ABY:
			b1 = NB();
			CMP(rA,mainMemory->ReadMemory(mainMemory->AB(rY,b1,NB())));
			CyclesRemain = 4+pboundarypassed;
		break;
		case CMP_INX:
			CMP(rA, mainMemory->ReadMemory(mainMemory->INdX(rX,NB())));
			CyclesRemain = 6;
		break;
		case CMP_INY:
			CMP(rA, mainMemory->ReadMemory(mainMemory->INdY(rY,NB())));
			CyclesRemain = 5+pboundarypassed;
		break;
		case CPX_IMM:
			CMP(rX,NB());
			CyclesRemain = 2;
		break;
		case CPX_ZP:
			CMP(rX,mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case CPX_AB:
			b1 = NB();
			CMP(rX,mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case CPY_IMM:
			CMP(rY,NB());
			CyclesRemain = 2;
		break;
		case CPY_ZP:
			CMP(rY,mainMemory->ReadMemory(mainMemory->ZP(NB())));
			CyclesRemain = 3;
		break;
		case CPY_AB:
			b1 = NB();
			CMP(rY,mainMemory->ReadMemory(mainMemory->AB(b1,NB())));
			CyclesRemain = 4;
		break;
		case NOP:
			// Do nothing
			CyclesRemain = 2;
		break;
		// Undocumented opcodes from here on outside
		// NOP Varients
		case NOP1:
		case NOP2:
		case NOP3:
		case NOP4:
		case NOP5:
		case NOP6:
			CyclesRemain = 2;
		break;
		// DOP (Double NOP) (All of these read from the memory address following it, and do nothing with the result)
		// Actually reading the values to keep consistency with log files
		case DOP1:
		case DOP4:
		case DOP6:
			mainMemory->ReadMemory(mainMemory->ZP(NB()));
			CyclesRemain = 3;
		break;
		case DOP2:
		case DOP3:
		case DOP5:
		case DOP7:
		case DOP12:
		case DOP14:
		mainMemory->ReadMemory(mainMemory->ZP(NB(),rX));
		CyclesRemain = 4;
		break;
		case DOP8:
		case DOP9:
		case DOP10:
		case DOP11:
		case DOP13:
		NB();
		CyclesRemain = 2;
		break;
		// TOP (Triple NOP)
		case TOP1:
			b1 = NB();
			mainMemory->ReadMemory(mainMemory->AB(b1,NB()));
			CyclesRemain = 4;
		break;
		case TOP2:
		case TOP3:
		case TOP4:
		case TOP5:
		case TOP6:
		case TOP7:
			b1 = NB();
			mainMemory->ReadMemory(mainMemory->AB(rX,b1,NB()));
			CyclesRemain = 4+pboundarypassed;
		break;
		default:
		if (InstName(opcode) != "UNKNOWN-OPCODE")
			std::cout<<"CPU-Error: Handler not yet implemented: " << InstName(opcode) << " at: "<<(int)pc<< std::endl;
			else
			std::cout<<"CPU-Error: Unknown opcode: $" << std::hex << (int)opcode << " at: "<<(int)pc<< std::endl;

		myState = CPUState::Error;
		break;
	}

	// If we have not jumped or branched, increment the pc
	if (jumpoffset == 0)
		pc += dataoffset;
		else
		pc = jumpoffset; // If jumpoffset is not 0, the pc will automatically move there for the next cycle - use this for jmp and branch operations.

	// Reset the dataoffset
	dataoffset = 0;

	// Reduce the remaining cycles variable as we've just done one (put this outside an if statement later to enable cycle accuracy when it is implemented).
	CyclesRemain--;
}

void CPU6502::fPLP(unsigned char value) {
// Bits 4 and 5 should be ignored, so we need to set the status register manually
	SetFlag(Flag::Carry,GetFlag(Flag::Carry,value));
	SetFlag(Flag::Zero,GetFlag(Flag::Zero,value));
	SetFlag(Flag::EInterrupt,GetFlag(Flag::EInterrupt,value));
	SetFlag(Flag::BCDMode,GetFlag(Flag::BCDMode,value));
	SetFlag(Flag::Overflow,GetFlag(Flag::Overflow,value));
	SetFlag(Flag::Sign,GetFlag(Flag::Sign,value));
}

void CPU6502::fRTS()
{
	// Return from a subroutine
	unsigned char lo = PopStack();
	unsigned char hi = PopStack();
	unsigned short location = (hi << 8) + lo;
	JMP(location+1); // Add 1 as what should have been pushed to the stack when JSR was called was address-1
}

void CPU6502::fRTI() {
	// Return from an interrupt handler
	unsigned char flags = PopStack();
	fPLP(flags);
	unsigned char lo = PopStack();
	unsigned char hi = PopStack();
	unsigned short location = (hi << 8) + lo;
	JMP(location); // Unlike with RTS, the pushed address contains the actual address we need to jump back to.
}

void CPU6502::BIT(unsigned char value) {
	SetFlag(Flag::Sign,(value >> 7)); // Set S flag to bit 7
	SetFlag(Flag::Overflow,(unsigned char)(value << 1) >> 7); // Set V flag to bit 6
	SetFlag(Flag::Zero,!(rA & value));
}

void CPU6502::CMP(unsigned char Register, unsigned char Value) {
	SetFlag(Flag::Carry,Register >= Value);
	SetFlag(Flag::Zero,Register == Value);
	SetFlag(Flag::Sign,(Register-Value) > 127);
}

void CPU6502::JMP(unsigned short Location) {
	// Jump to the specified address
	jumpoffset = Location;
}

void CPU6502::branch(bool value)
{
	if(value)
	{
		unsigned char branchloc = NB();
		//Get the sign of the value
		bool sign = (branchloc>0x7F);

		if (sign)
			dataoffset = -branchloc+2;
			else
			dataoffset = branchloc+2;

		 // Dataoffset should = the byte after the instruction
		//std::cout<<"Branching to: "<<std::dec<<(int) dataoffset<<std::endl;
		CyclesRemain = 3+pboundarypassed;
	}
	else {
	CyclesRemain = 2;
	NB(); // Skip the next byte as it is data for the branch instruction.
	}
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

std::string CPU6502::InstName(unsigned char opcode) {
	// Used for debugging purposes, just spits out the name of the current opcode - makes it easier to compare CPU logs with other emulators for debugging.
	std::string RetVal;
	switch (opcode)
	{
		case ADC_IMM:
		RetVal = "ADC_IMM";
		break;
		case ADC_ZP:
		RetVal = "ADC_ZP";
		break;
		case ADC_ZPX:
		RetVal = "ADC_ZPX";
		break;
		case ADC_AB:
		RetVal = "ADC_AB";
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
		RetVal = "AND_ZP";
		break;
		case AND_ZPX:
		RetVal = "AND_ZPX";
		break;
		case AND_AB:
		RetVal = "AND_AB";
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
		RetVal = "ASL_ZP";
		break;
		case ASL_ZPX:
		RetVal = "ASL_ZPX";
		break;
		case ASL_AB:
		RetVal = "ASL_AB";
		break;
		case ASL_ABX:
		RetVal = "ASL_ABX";
		break;
		case BCC:
		RetVal = "BCC";
		break;
		case BCS:
		RetVal = "BCS";
		break;
		case BEQ:
		RetVal = "BEQ";
		break;
		case BMI:
		RetVal = "BMI";
		break;
		case BNE:
		RetVal = "BNE";
		break;
		case BPL:
		RetVal = "BPL";
		break;
		case BVC:
		RetVal = "BVC";
		break;
		case BVS:
		RetVal = "BVS";
		break;
		case BIT_ZP:
		RetVal = "BIT_ZP";
		break;
		case BIT_AB:
		RetVal = "BIT_AB";
		break;
		case CLC:
		RetVal = "CLC";
		break;
		case CLD:
		RetVal = "CLD";
		break;
		case CLI:
		RetVal = "CLI";
		break;
		case CLV:
		RetVal = "CLV";
		break;
		case CMP_IMM:
		RetVal = "CMP_IMM";
		break;
		case CMP_ZP:
		RetVal = "CMP_ZP";
		break;
		case CMP_ZPX:
		RetVal = "CMP_ZPX";
		break;
		case CMP_AB:
		RetVal = "CMP_AB";
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
		RetVal = "CPX_ZP";
		break;
		case CPX_AB:
		RetVal = "CPX_AB";
		break;
		case CPY_IMM:
		RetVal = "CPY_IMM";
		break;
		case CPY_ZP:
		RetVal = "CPY_ZP";
		break;
		case CPY_AB:
		RetVal = "CPY_AB";
		break;
		case DEC_ZP:
		RetVal = "DEC_ZP";
		break;
		case DEC_ZPX:
		RetVal = "DEC_ZPX";
		break;
		case DEC_AB:
		RetVal = "DEC_AB";
		break;
		case DEC_ABX:
		RetVal = "DEC_ABX";
		break;
		case DEX:
		RetVal = "DEX";
		break;
		case DEY:
		RetVal = "DEY";
		break;
		case EOR_IMM:
		RetVal = "EOR_IMM";
		break;
		case EOR_ZP:
		RetVal = "EOR_ZP";
		break;
		case EOR_ZPX:
		RetVal = "EOR_ZPX";
		break;
		case EOR_AB:
		RetVal = "EOR_AB";
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
		RetVal = "INC_ZP";
		break;
		case INC_ZPX:
		RetVal = "INC_ZPX";
		break;
		case INC_AB:
		RetVal = "INC_AB";
		break;
		case INC_ABX:
		RetVal = "INC_ABX";
		break;
		case INX:
		RetVal = "INX";
		break;
		case INY:
		RetVal = "INY";
		break;
		case JMP_AB:
		RetVal = "JMP_AB";
		break;
		case JMP_IN:
		RetVal = "JMP_IN";
		break;
		case JSR:
		RetVal = "JSR";
		break;
		case LDA_IMM:
		RetVal = "LDA_IMM";
		break;
		case LDA_ZP:
		RetVal = "LDA_ZP";
		break;
		case LDA_ZPX:
		RetVal = "LDA_ZPX";
		break;
		case LDA_AB:
		RetVal = "LDA_AB";
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
		RetVal = "LDX_ZP";
		break;
		case LDX_ZPY:
		RetVal = "LDX_ZPY";
		break;
		case LDX_AB:
		RetVal = "LDX_AB";
		break;
		case LDX_ABY:
		RetVal = "LDX_ABY";
		break;
		case LDY_IMM:
		RetVal = "LDY_IMM";
		break;
		case LDY_ZP:
		RetVal = "LDY_ZP";
		break;
		case LDY_ZPX:
		RetVal = "LDY_ZPX";
		break;
		case LDY_AB:
		RetVal = "LDY_AB";
		break;
		case LDY_ABX:
		RetVal = "LDY_ABX";
		break;
		case LSR_ACC:
		RetVal = "LSR_A";
		break;
		case LSR_ZP:
		RetVal = "LSR_ZP";
		break;
		case LSR_ZPX:
		RetVal = "LSR_ZPX";
		break;
		case LSR_AB:
		RetVal = "LSR_AB";
		break;
		case LSR_ABX:
		RetVal = "LSR_ABX";
		break;
		case NOP:
		RetVal = "NOP";
		break;
		case ORA_IMM:
		RetVal = "ORA_IMM";
		break;
		case ORA_ZP:
		RetVal = "ORA_ZP";
		break;
		case ORA_ZPX:
		RetVal = "ORA_ZPX";
		break;
		case ORA_AB:
		RetVal = "ORA_AB";
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
		RetVal = "PHA";
		break;
		case PHP:
		RetVal = "PHP";
		break;
		case PLA:
		RetVal = "PLA";
		break;
		case PLP:
		RetVal = "PLP";
		break;
		case ROL_ACC:
		RetVal = "ROL_ACC";
		break;
		case ROL_ZP:
		RetVal = "ROL_ZP";
		break;
		case ROL_ZPX:
		RetVal = "ROL_ZPX";
		break;
		case ROL_AB:
		RetVal = "ROL_AB";
		break;
		case ROL_ABX:
		RetVal = "ROL_ABX";
		break;
		case ROR_ACC:
		RetVal = "ROR_ACC";
		break;
		case ROR_ZP:
		RetVal = "ROR_ZP";
		break;
		case ROR_ZPX:
		RetVal = "ROR_ZPX";
		break;
		case ROR_AB:
		RetVal = "ROR_AB";
		break;
		case ROR_ABX:
		RetVal = "ROR_ABX";
		break;
		case RTI:
		RetVal = "RTI";
		break;
		case RTS:
		RetVal = "RTS";
		break;
		case SBC_IMM:
		RetVal = "SBC_IMM";
		break;
		case SBC_ZP:
		RetVal = "SBC_ZP";
		break;
		case SBC_ZPX:
		RetVal = "SBC_ZPX";
		break;
		case SBC_AB:
		RetVal = "SBC_AB";
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
		RetVal = "SEC";
		break;
		case SED:
		RetVal = "SED";
		break;
		case SEI:
		RetVal = "SEI";
		break;
		case STA_ZP:
		RetVal = "STA_ZP";
		break;
		case STA_ZPX:
		RetVal = "STA_ZPX";
		break;
		case STA_AB:
		RetVal = "STA_AB";
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
		RetVal = "STX_ZP";
		break;
		case STX_ZPY:
		RetVal = "STX_ZPY";
		break;
		case STX_AB:
		RetVal = "STX_AB";
		break;
		case STY_ZP:
		RetVal = "STY_ZP";
		break;
		case STY_ZPX:
		RetVal = "STY_ZPX";
		break;
		case STY_AB:
		RetVal = "STY_AB";
		break;
		case TAX:
		RetVal = "TAX";
		break;
		case TAY:
		RetVal = "TAY";
		break;
		case TSX:
		RetVal = "TSX";
		break;
		case TXA:
		RetVal = "TXA";
		break;
		case TXS:
		RetVal = "TXS";
		break;
		case TYA:
		RetVal = "TYA";
		break;
		// Undocumented opcodes from here on out
		case NOP1:
		case NOP2:
		case NOP3:
		case NOP4:
		case NOP5:
		case NOP6:
		RetVal = "*NOP";
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
		RetVal = "DOP";
		break;
		case TOP1:
		case TOP2:
		case TOP3:
		case TOP4:
		case TOP5:
		case TOP6:
		case TOP7:
		RetVal = "TOP";
		break;
		default:
		RetVal = "UNKNOWN-OPCODE";
		break;
	}

	return RetVal;
}

void CPU6502::PushStack8(unsigned char value) {
	mainMemory->WriteMemory(0x100+(sp--),value);
}

void CPU6502::PushStack16(unsigned short value) {
	PushStack8(value >> 8);
	PushStack8(value);
}

unsigned char CPU6502::PopStack() {
	return mainMemory->ReadMemory(0x100 + (++sp));
}
