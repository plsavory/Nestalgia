#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include "InputManager.h"


InputManager::InputManager() {
  // Do whatever is needed (set config state in future)
  ReadCount[0] = 0;
  ReadCount[1] = 0;
}

void InputManager::Update() {
  // Todo: Later add support for customizable controls
  controllers[0].Update(); // Update controller 1
}

unsigned char InputManager::GetStatus(unsigned short Location) {
  // Return the state of the requested input device in a format readable by the emulated CPU
  // Return format: A,B,Select,Start,Up,Down,Left,Right (8-bit value)
  unsigned char RetVal = 0;

  if (Location == 0x4016) {
    // Controller 1
    RetVal = SetBit(0,GetButtonPress(ReadCount[0]),RetVal);
    ReadCount[0]++;
  }

  return RetVal;
}

bool InputManager::GetButtonPress(int Read) {
  if (Read < 8)
    return controllers[0].buttons[Read];
  else
    return 1;
}

void InputManager::WriteStatus(unsigned short Location) {
  // For now, just reset the latch. Will have to emulate the actual shift register later.
  ReadCount[0] = 0;
  ReadCount[1] = 0;
}

unsigned char InputManager::SetBit(int bit, bool val, unsigned char value) // Used for setting flags to a value which is not the flag register
{
	// Sets a specific flag to true or false depending on the value of "val"
	unsigned char RetVal = value;
	if (val)
		RetVal |= (1 << bit);
	else
		RetVal &= ~(1 << bit);

	return RetVal;
}
