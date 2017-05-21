#include "MemoryManager.h"
#include "CPU6502.h"

class MainSystem
{
public:
  MainSystem();
  ~MainSystem();
  void LoadROM();
  void Execute();
  void Reset();
  void Run();
private:
  MemoryManager *mainMemory;
  CPU6502 *mainCPU;
};
