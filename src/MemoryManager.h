#pragma once
enum MemoryMapper {None,Test};

class MemoryManager
{
public:
	MemoryManager();
	~MemoryManager();
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
	MemoryMapper mapper;
};

/*

*/