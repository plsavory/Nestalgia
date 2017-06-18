#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "PPU.h"
#include "MemoryManager.h"
#include "CPU6502.h"

class MainSystem
{
public:
  MainSystem();
  ~MainSystem();
  bool LoadROM(std::string ROMFile);
  void Execute();
  void Reset();
  void Run();
private:
  MemoryManager *mainMemory;
  CPU6502 *mainCPU;
  PPU *mainPPU;
  sf::Clock *frameRate;
  int fps; // Increment each time the PPU outputs 1 frame
};
