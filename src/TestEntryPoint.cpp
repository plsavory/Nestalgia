#include "CPUInstructions.h"
#include "UnitTestClass.h"


int main() {
	// For now, just run the unit tests.
	UnitTestClass test;
	test.TestMemory();
	test.TestCPU();

	return 0;
}
