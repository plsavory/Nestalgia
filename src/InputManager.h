namespace Input {

enum InputType {iKeyboard,iMouse,iJoystickAxis,iJoystickButton}; // We only support keyboards for now, but will need this later

struct Key {
  InputType myType;
  sf::Keyboard::Key keyCode;
  sf::Mouse::Button mouseButton;
  int JoystickID;
  int JoystickButton;
  int JoystickAxis;
};

struct Keymap {
  Key Up;
  Key Down;
  Key Left;
  Key Right;
  Key A;
  Key B;
  Key Start;
  Key Select;

  Keymap() {
    // For now, just use the default controls
    Up.myType = InputType::iKeyboard;
    Up.keyCode = sf::Keyboard::Up;
    Down.myType = InputType::iKeyboard;
    Down.keyCode = sf::Keyboard::Down;
    Left.myType = InputType::iKeyboard;
    Left.keyCode = sf::Keyboard::Left;
    Right.myType = InputType::iKeyboard;
    Right.keyCode = sf::Keyboard::Right;
    A.myType = InputType::iKeyboard;
    A.keyCode = sf::Keyboard::A;
    B.myType = InputType::iKeyboard;
    B.keyCode = sf::Keyboard::B;
    Start.myType = InputType::iKeyboard;
    Start.keyCode = sf::Keyboard::Return;
    Select.myType = InputType::iKeyboard;
    Select.keyCode = sf::Keyboard::Space;
  }
};


struct Controller {
  bool Up;
  bool Down;
  bool Left;
  bool Right;
  bool A;
  bool B;
  bool Start;
  bool Select;

  Controller()
  {
    Up = false; Down = false; Left = false; Right = false; A = false; B = false; Start = false; Select = false;
  }
};

};

class InputManager {
public:
  InputManager();
  ~InputManager();
  void Update(); // Updates the InputManager's state
  void GetStatus(unsigned short Location); // Gets the current status of a controller and returns it in a format readable by the CPU
private:
  Input::Controller controllers[1];
  void UpdateController(int Controller, Input::Keymap kmap);
};
