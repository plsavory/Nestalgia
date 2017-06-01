#pragma once
#include <fstream>
#include "CPUInstructions.h"
using namespace M6502;


class CPU6502
{
public:
	CPU6502(MemoryManager &mManager);
	~CPU6502();
	CPUState myState;
	void SetFlag(Flag flag, bool val);
	unsigned char SetBit(int bit, bool val,unsigned char value);
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
	void CMP(unsigned char Register, unsigned char Value);
	unsigned short answer16;
	void Execute();
	void Reset();
private:
	unsigned char b1;
	unsigned char FlagRegister;
	unsigned short pc;
	signed char dataoffset; // Causes the pc to be incremented at the end of an instruction
	unsigned short jumpoffset; // Used to tell the CPU where to jump next
	unsigned char NB();
	void JMP(unsigned short Location);
	void branch(bool value);
	void PrintCPUStatus(std::string inst_name);
	MemoryManager *mainMemory;
	int CyclesRemain = 0;
	unsigned char sp;
	bool pboundarypassed;
	unsigned char location;
	unsigned short location16;
	unsigned char result;
	std::string InstName(unsigned char opcode);
	void PushStack8(unsigned char value);
	void PushStack16(unsigned short value);
	void fPHP();
	unsigned char PopStack();
	void fPLP(unsigned char value);
	void BIT(unsigned char value);
	void fRTS();
	void fRTI();
	void fBRK();
	int cpucycles;
	std::string ConvertHex(int value);
	std::ofstream * redirectfile;
	unsigned char currentInst;
};
