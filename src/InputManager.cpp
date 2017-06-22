#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include "InputManager.h"


InputManager::InputManager() {
  // Do whatever is needed (set config state in future)
}

void InputManager::Update() {
  // Todo: Later add support for customizable controls
  controllers[0].Update(); // Update controller 1
}
