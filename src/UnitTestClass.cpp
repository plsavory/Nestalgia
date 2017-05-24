#include <iostream>
#include "UnitTestClass.h"
#include "CPUInstructions.h"
#include "MemoryManager.h"
#include "CPU6502.h"


#pragma GCC diagnostic ignored "-Woverflow"
// This class should not be neccessary in future. This is currently being used just to test the functionality of the emulator before it can actually run emulator test programs.



// Compile-time warnings have been disabled for this file only since I have purposefully used bad programming practice in some places to test function behavior.

UnitTestClass::UnitTestClass()
{
}


UnitTestClass::~UnitTestClass()
{
}

void UnitTestClass::TestMemory()
{
	// Runs the standard tests for memory.
	MemoryManager *testMemory = new MemoryManager();

	// Test the main memory read and write functions
	testMemory->WriteMemory(0x100, 0xFE);
	assert(testMemory->ReadMemory(0x100) == 0xFE);


	// Test RAM Wrapping
	testMemory->WriteMemory(0x0,0xAE);
	assert(testMemory->ReadMemory(0x0 == 0xAE));
	assert(testMemory->ReadMemory(0x800 == 0xAE));
	assert(testMemory->ReadMemory(0x1000 == 0xAE));
	assert(testMemory->ReadMemory(0x1800 == 0xAE));

	// Simulate an Absolute request
	unsigned char B1 = 0x40;
	unsigned char B2 = 0x50;
	assert(testMemory->AB(B1,B2) == 0x5040);

	// Simulate an absolute write and read
	B1 = 0x25;
	B2 = 0x04;
	testMemory->WriteMemory(testMemory->AB(B1, B2),0x41);
	assert(testMemory->ReadMemory(0x0425) == 0x41);

	// Simulate an ABX/ABY write and read
	B1 = 0x25;
	B2 = 0x05;
	testMemory->WriteMemory(testMemory->AB(0x4, B1, B2),0xFE);
	assert(testMemory->ReadMemory(0x0529) == 0xFE);

	// Simulate ZP write and read
	testMemory->WriteMemory(testMemory->ZP(0xFF), 0xFB);
	assert(testMemory->ReadMemory(0xFF) == 0xFB);

	// Simulate overflowed ZP write and read
	testMemory->WriteMemory(testMemory->ZP(257),0x5e);
	assert(testMemory->ReadMemory(0x01) == 0x5e);
	assert(testMemory->ReadMemory(testMemory->ZP(257)) == 0x5e);
	testMemory->WriteMemory(testMemory->ZP(0xC0, 0x60), 0x55);
	assert(testMemory->ReadMemory(testMemory->ZP(0x20)) == 0x55);

	// Simulate an indirect read
	testMemory->WriteMemory(0x1000, 0x52);
	assert(testMemory->ReadMemory(0x1000) == 0x52);
	testMemory->WriteMemory(0x1001, 0x1a);
	testMemory->WriteMemory(0x1a52, 0x50);
	assert(testMemory->IN(0x00, 0x10) == 0x1a52);
	assert(testMemory->ReadMemory(testMemory->IN(0x00, 0x10)) == 0x50);

	// Simulate indexed indirect read
	testMemory->WriteMemory(0x24, 0xFF);
	testMemory->WriteMemory(0x25, 0x1F);
	testMemory->WriteMemory(0x1FFF, 0x43);
	assert(testMemory->INdX(0x4, 0x20) == 0x1FFF);
	assert(testMemory->ReadMemory(testMemory->INdX(0x4, 0x20)) == 0x43);

	// Simulate indirect indexed read
	testMemory->WriteMemory(0x86, 0x28);
	testMemory->WriteMemory(0x87, 0x40);
	assert(testMemory->INdY(0x10, 0x86) == 0x4038);
	delete(testMemory);
	std::cout << "Memory Tests Succeeded" << std::endl;
}

void UnitTestClass::TestCPU()
{
	// Tests all of the processor functions to make sure that they behave correctly. This will grow to a substantial size in future due to the amount of functions that the CPU related code is likely to use.
	TestCPUFlags();
	TestInstructionHandlers();
	std::cout << "CPU Test Succeeded" << std::endl;
}

bool UnitTestClass::TestCPUFlags()
{
	// Test the flag register functionality of the CPU (Not the flag behavior of different CPU functions)
	MemoryManager *mainMemory = new MemoryManager();
	CPU6502 testCPU(*mainMemory);

	assert(testCPU.GetFlags() == 0x0);
	testCPU.SetFlag(Flag::Sign,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlags() == 128);
	testCPU.SetFlag(Flag::Sign,0);

	// Sequentially set or clear each flag and then make sure that the others retain their values
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 0x0);

	testCPU.SetFlag(Flag::Sign,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 128);

	testCPU.SetFlag(Flag::Overflow,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 192);

	testCPU.SetFlag(Flag::Unused,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 224);

	testCPU.SetFlag(Flag::SInterrupt,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 240);

	testCPU.SetFlag(Flag::BCDMode,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == true);
	assert(testCPU.GetFlag(Flag::EInterrupt) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 248);

	testCPU.SetFlag(Flag::EInterrupt,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == true);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 252);

	testCPU.SetFlag(Flag::Zero,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == true);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == true);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 254);

	testCPU.SetFlag(Flag::Carry,1);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == true);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == true);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlags() == 255);

	testCPU.SetFlag(Flag::BCDMode,0);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == true);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == true);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlags() == 247);

	testCPU.SetFlag(Flag::Overflow,0);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == true);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlags() == 183);

	testCPU.SetFlag(Flag::Zero,0);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == true);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlags() == 181);

	testCPU.SetFlag(Flag::Unused,0);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == true);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlags() == 149);

	testCPU.SetFlag(Flag::SInterrupt,0);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlags() == 133);

	testCPU.SetFlag(Flag::Carry,0);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 132);

	testCPU.SetFlag(Flag::Sign,0);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 4);

	testCPU.SetFlag(Flag::EInterrupt,0);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Overflow) == false);
	assert(testCPU.GetFlag(Flag::Unused) == false);
	assert(testCPU.GetFlag(Flag::SInterrupt) == false);
	assert(testCPU.GetFlag(Flag::BCDMode) == false);
	assert(testCPU.GetFlag(Flag::EInterrupt) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlags() == 0);
	return true;
}

bool UnitTestClass::TestInstructionHandlers()
{
	MemoryManager *mainMemory = new MemoryManager();
	CPU6502 testCPU(*mainMemory);

	// Test the AND instruction and make sure the flags behave as they should.
	testCPU.rA = 0xCC;
	testCPU.rA = testCPU.AND(0xCB);
	assert(testCPU.rA == 0xC8);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rA = 0xA1;
	testCPU.rA = testCPU.AND(0x05);
	assert(testCPU.rA == 0x1);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rA = 0x0;
	testCPU.rA = testCPU.AND(0x0);
	assert(testCPU.rA == 0x0);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Zero) == true);

	testCPU.rA = 0xFF;
	testCPU.rA = testCPU.AND(0xFA);
	assert(testCPU.rA == 0xFA);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	// Test the ASL instruction
	testCPU.rA = 128;
	testCPU.rA = testCPU.ASL(testCPU.rA);
	assert(testCPU.rA == 0);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlag(Flag::Zero) == true);

	testCPU.rA = 64;
	testCPU.rA = testCPU.ASL(testCPU.rA);
	assert(testCPU.rA == 128);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rA = 192;
	testCPU.rA = testCPU.ASL(testCPU.rA);
	assert(testCPU.rA == 128);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Carry) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rA = 8;
	testCPU.rA = testCPU.ASL(testCPU.rA);
	assert(testCPU.rA == 16);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Carry) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);


	// Test LSR instruction

	/* Test data - taken from the simulator on here: https://skilldrick.github.io/easy6502/#intro
	FF = 7f N:0 C:1 Z: 0
	7F = 3f N:0 C:1 Z: 0
	3F = 1f N:0 C:1 Z: 0
	1F = 0f N:0 C:1 Z: 0
	0F = 07 N:0 C:1 Z: 0
	07 = 03 N:0 C:1 Z: 0
	03 = 01 N:0 C:1 Z: 0
	01 = 00 N:0 C:1 Z: 1
	*/

	testCPU.rA = 0xFF;

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x7f);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x3f);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x1f);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x0f);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x07);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x03);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x01);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);

	testCPU.rA = testCPU.LSR(testCPU.rA);
	assert(testCPU.rA == 0x0);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 1);

	// Test the ROL function

	/* Test data - taken from the simulator on here: https://skilldrick.github.io/easy6502/#intro
	0xFF: 0xFE, N: 1 Z: 0 C: 1
	0xFE: 0xFC, N: 1 Z: 0 C: 1
	0xFC: 0xF8, N: 1 Z: 0 C: 1
	0x00: 0x00, N: 0 Z: 1 C: 0
	0x05: 0x0A, N: 0 Z: 0 C: 0
	0x14: 0x28, N: 0 Z: 0 C: 0
	*/

	testCPU.SetFlag(Flag::Carry, 0); // Make sure Carry is 0 atm, otherwise it messes with this test (oops!)
	testCPU.rA = testCPU.LD(0xFF);
	testCPU.rA = testCPU.ROL(testCPU.rA);
	assert(testCPU.rA == 0xFE);
	assert(testCPU.GetFlag(Flag::Sign) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);

	testCPU.SetFlag(Flag::Carry, 0);
	testCPU.rA = testCPU.LD(0xFE);
	testCPU.rA = testCPU.ROL(testCPU.rA);
	assert(testCPU.rA == 0xFC);
	assert(testCPU.GetFlag(Flag::Sign) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);

	testCPU.SetFlag(Flag::Carry, 0);
	testCPU.rA = testCPU.LD(0xFC);
	testCPU.rA = testCPU.ROL(testCPU.rA);
	assert(testCPU.rA == 0xF8);
	assert(testCPU.GetFlag(Flag::Sign) == 1);
	assert(testCPU.GetFlag(Flag::Zero) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 1);

	testCPU.SetFlag(Flag::Carry, 0);
	testCPU.rA = testCPU.LD(0x00);
	testCPU.rA = testCPU.ROL(testCPU.rA);
	assert(testCPU.rA == 0x00);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Zero) == 1);
	assert(testCPU.GetFlag(Flag::Carry) == 0);

	testCPU.SetFlag(Flag::Carry, 0);
	testCPU.rA = testCPU.LD(0x05);
	testCPU.rA = testCPU.ROL(testCPU.rA);
	assert(testCPU.rA == 0x0A);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Zero) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 0);

	testCPU.SetFlag(Flag::Carry, 0);
	testCPU.rA = testCPU.LD(0x14);
	testCPU.rA = testCPU.ROL(testCPU.rA);
	assert(testCPU.rA == 0x28);
	assert(testCPU.GetFlag(Flag::Sign) == 0);
	assert(testCPU.GetFlag(Flag::Zero) == 0);
	assert(testCPU.GetFlag(Flag::Carry) == 0);

	// Test the LD function and make sure the flags behave as they should.
	testCPU.rA = testCPU.LD(0x0);
	assert(testCPU.rA == 0x0);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Zero) == true);

	testCPU.rA = testCPU.LD(0x1);
	assert(testCPU.rA == 0x1);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rA = (127);
	assert(testCPU.rA == 127);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rA = testCPU.LD(128);
	assert(testCPU.rA == 128);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rX = testCPU.LD(255);
	assert(testCPU.rX== 255);
	assert(testCPU.GetFlag(Flag::Sign) == true);
	assert(testCPU.GetFlag(Flag::Zero) == false);

	testCPU.rY = testCPU.LD(256); // Test an overflow (the LD instruction does not touch the Overflow flag anyway so don't bother testing that bit)
	assert(testCPU.rY == 0);
	assert(testCPU.GetFlag(Flag::Sign) == false);
	assert(testCPU.GetFlag(Flag::Zero) == true);

	// Test the ADC instruction handler - test data taken from http://www.atariarchives.org/alp/appendix_1.php

	// 1
	testCPU.rA = 2; testCPU.SetFlag(Flag::Sign,0); testCPU.SetFlag(Flag::Zero,0); testCPU.SetFlag(Flag::Carry,0); testCPU.SetFlag(Flag::Overflow,0);
	testCPU.rA = testCPU.ADC(3);
	assert(testCPU.rA == 5);
	assert(!testCPU.GetFlag(Flag::Sign));
	assert(!testCPU.GetFlag(Flag::Zero));
	assert(!testCPU.GetFlag(Flag::Carry));
	assert(!testCPU.GetFlag(Flag::Overflow));

	// 2
	testCPU.rA = 2; testCPU.SetFlag(Flag::Sign,0); testCPU.SetFlag(Flag::Zero,0); testCPU.SetFlag(Flag::Carry,1); testCPU.SetFlag(Flag::Overflow,0);
	testCPU.rA = testCPU.ADC(3);
	assert(testCPU.rA == 6);
	assert(!testCPU.GetFlag(Flag::Sign));
	assert(!testCPU.GetFlag(Flag::Zero));
	assert(!testCPU.GetFlag(Flag::Carry));
	assert(!testCPU.GetFlag(Flag::Overflow));

	// 3
	testCPU.rA = 2; testCPU.SetFlag(Flag::Sign,0); testCPU.SetFlag(Flag::Zero,0); testCPU.SetFlag(Flag::Carry,0); testCPU.SetFlag(Flag::Overflow,0);
	testCPU.rA = testCPU.ADC(254);
	assert(testCPU.rA == 0);
	assert(!testCPU.GetFlag(Flag::Sign));
	assert(testCPU.GetFlag(Flag::Zero));
	assert(testCPU.GetFlag(Flag::Carry));
	assert(!testCPU.GetFlag(Flag::Overflow));

	// 4
	testCPU.rA = 2; testCPU.SetFlag(Flag::Sign,0); testCPU.SetFlag(Flag::Zero,0); testCPU.SetFlag(Flag::Carry,0); testCPU.SetFlag(Flag::Overflow,0);
	testCPU.rA = testCPU.ADC(253);
	assert(testCPU.rA == 255);
	assert(testCPU.GetFlag(Flag::Sign));
	assert(!testCPU.GetFlag(Flag::Zero));
	assert(!testCPU.GetFlag(Flag::Carry));
	assert(!testCPU.GetFlag(Flag::Overflow));

	// 5
	testCPU.rA = 255; testCPU.SetFlag(Flag::Sign,0); testCPU.SetFlag(Flag::Zero,0); testCPU.SetFlag(Flag::Carry,0); testCPU.SetFlag(Flag::Overflow,0);
	testCPU.rA = testCPU.ADC(254);
	assert(testCPU.rA == 253);
	assert(testCPU.GetFlag(Flag::Sign));
	assert(!testCPU.GetFlag(Flag::Zero));
	assert(testCPU.GetFlag(Flag::Carry));


	// Test the SBC instruction handler

	return true;
}

#pragma GCC diagnostic pop
