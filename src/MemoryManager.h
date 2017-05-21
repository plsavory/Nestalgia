#pragma once
enum MemoryMapper {None,Test};
enum HeaderData {Const0,Const1,Const2,Const3,PROMSize,CROMSize,Flags6,Flags7,PRAMSize,Flags9,Flags10,Null};

struct Cartridge
{
	unsigned char Header[15];
	unsigned char Trainer[512];
	unsigned char pROM[0x200000]; // 2kb is the largest ROM size ever found in iNes format, so have this as a hard limit.
	unsigned char cROM[0x80000]; // 512KiB max
	unsigned char InstROM[8192];
	unsigned char PROM[32];
};

class MemoryManager
{
public:
	MemoryManager();
	~MemoryManager();
	int LoadFile(std::string FilePath);
	unsigned char ReadMemory(unsigned short Location);
	void WriteMemory(unsigned short Location, unsigned char Value);
	unsigned short IN(unsigned char Lo, unsigned char Hi);
	unsigned short INdX(unsigned char rX, unsigned char Loc);
	unsigned short INdY(unsigned char rY, unsigned char Loc);
	unsigned short AB(unsigned char Lo, unsigned char Hi);
	unsigned short AB(unsigned char Off, unsigned char Lo, unsigned char Hi);
	unsigned short ZP(unsigned char Location);
	unsigned short ZP(unsigned char Location, unsigned char Register);
private:
	unsigned char MainMemory[0xFFFF];
	Cartridge *mainCartridge;
	MemoryMapper mapper;
};

/*

*/
