#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "InputManager.h"
#include "PPU.h"
#include "MemoryManager.h"
#include "CPU6502.h"

class MainSystem {
public:
  MainSystem();

  ~MainSystem();

  bool loadROM(std::string fileName);

  void execute();

  void reset();

  void run();
private:
  InputManager *mainInput;
  MemoryManager *mainMemory;
  CPU6502 *mainCPU;
  PPU *mainPPU;
  sf::Clock *frameRate;
  int fps; // Increment each time the PPU outputs 1 frame
  bool hasFocus; // Does the window have focus or not?
};
