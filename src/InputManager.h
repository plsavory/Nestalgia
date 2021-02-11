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

    return false;
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
  bool buttons[8];

  Controller()
  {
    for (int i = 0; i <8;i++)
      buttons[i] = false;
  }

  void Update() {
    // Update all of the values within this struct
    buttons[0] = myKeymap.A.isPressed();
    buttons[1] = myKeymap.B.isPressed();
    buttons[2] = myKeymap.Select.isPressed();
    buttons[3] = myKeymap.Start.isPressed();
    buttons[4] = myKeymap.Up.isPressed();
    buttons[5] = myKeymap.Down.isPressed();
    buttons[6] = myKeymap.Left.isPressed();
    buttons[7] = myKeymap.Right.isPressed();
  }

};

};

class InputManager {
public:
  InputManager();
  ~InputManager();
  void Update(); // Updates the InputManager's state
  unsigned char GetStatus(unsigned short Location); // Gets the current status of a controller and returns it in a format readable by the CPU
  void WriteStatus(unsigned short Location);
private:
  Input::Controller controllers[1];
  void UpdateController(int Controller, Input::Keymap kmap);
  unsigned char SetBit(int bit, bool val, unsigned char value);
  int ReadCount[1]; // The number of times the CPU has read from the registers
  bool GetButtonPress(int Read);
};
