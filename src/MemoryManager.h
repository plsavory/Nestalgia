#pragma once
enum MemoryMapper {None,Test};
enum HeaderData {Const0,Const1,Const2,Const3,PROMSize,CROMSize,Flags6,Flags7,PRAMSize,Flags9,Flags10,Null};

class MemoryManager
{
public:
	MemoryManager(PPU &mPPU);
	~MemoryManager();
	int LoadFile(std::string FilePath);
	unsigned char ReadMemory(unsigned short Location);
	unsigned char ReadMemory(unsigned short Location,bool Silent);
	void WriteMemory(unsigned short Location, unsigned char Value);
	unsigned short IN(unsigned char Lo, unsigned char Hi);
	unsigned short INdX(unsigned char rX, unsigned char Loc);
	unsigned short INdY(unsigned char rY, unsigned char Loc);
	unsigned short AB(unsigned char Lo, unsigned char Hi);
	unsigned short AB(unsigned char Off, unsigned char Lo, unsigned char Hi);
	unsigned short ZP(unsigned char Location);
	unsigned short ZP(unsigned char Location, unsigned char Register);
	void PrintDebugInfo();
		bool pboundarypassed; // Holds true if the previous indirect memory operation passed a page boundary, reset otherwise.
	// Allow hardware connected to the MemoryManager to set these.
	bool CheckIRQ();
	bool CheckNMI();
	Cartridge *mainCartridge;
private:
	unsigned char MainMemory[0xFFFF];
	void WriteRAM(unsigned short Location, unsigned char Value);
	void WriteCartridge(unsigned short Location, unsigned char Value);
	unsigned short WrapMemory(unsigned short Location, unsigned short WrapValue);
	unsigned char ReadCartridge(unsigned short Location);
	unsigned char ReadPPU(unsigned short Location);
	void WritePPU(unsigned short location,unsigned char value);
	int CheckCartridge(Cartridge &cartridge);
	PPU *mainPPU;
	MemoryMapper mapper;
	// Mapper functions
	unsigned char ReadNROM(unsigned short Location);
	bool IRQLine;
	bool NMILine;
};

/*

*/
