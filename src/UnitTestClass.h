#pragma once
#pragma GCC system_header
#include <assert.h>

class UnitTestClass
{
public:
	UnitTestClass();
	~UnitTestClass();
	void TestMemory();
	void TestCPU();
private:
	bool TestCPUFlags();
	bool TestInstructionHandlers();
};
