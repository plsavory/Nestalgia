namespace Input {

enum InputType {iKeyboard,iMouse,iJoystickAxis,iJoystickButton}; // We only support keyboards for now, but will need this later

struct Key {
  InputType myType;
  sf::Keyboard::Key keyCode;
  sf::Mouse::Button mouseButton;
  int JoystickID;
  int JoystickButton;
  int JoystickAxis;

  Key() {
    // By default, use keyboard controls
    myType = InputType::iKeyboard;
  }

  bool isPressed() {
    if (myType == InputType::iKeyboard) {
      // Return if the key in question is pressed or not
      return sf::Keyboard::isKeyPressed(keyCode);
    }
  }
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
    B.keyCode = sf::Keyboard::S;
    Start.myType = InputType::iKeyboard;
    Start.keyCode = sf::Keyboard::Return;
    Select.myType = InputType::iKeyboard;
    Select.keyCode = sf::Keyboard::Space;
  }
};


struct Controller {
  Keymap myKeymap;
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

  void Update() {
    // Update all of the values within this struct
    Up = myKeymap.Up.isPressed();
    Down = myKeymap.Down.isPressed();
    Left = myKeymap.Left.isPressed();
    Right = myKeymap.Right.isPressed();
    A = myKeymap.A.isPressed();
    B = myKeymap.B.isPressed();
    Start = myKeymap.Start.isPressed();
    Select = myKeymap.Select.isPressed();
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
