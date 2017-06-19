enum PPUReg {PPUCTRL,PPUMASK,PPUSTATUS,OAMADDR,Latch1,Latch2,PPUSCROLL,PPUADDR,PPUDATA};


struct Nametable {
    int Type; // 0 = Mirrors Nametable 0, 1 = Mirrors Nametable 1, 2 = Standard
    unsigned char Data[0x400];

    Nametable()
    {
      Type = 2;

      for (int i = 0; i <0x400;i++)
        Data[i] = 0;
    }
};

class PPU {

public:
  PPU();
  void Reset();
  void Execute(float PPUClock);
  void Draw(sf::RenderWindow &mWindow);
  sf::Image* displayImage;
  sf::Texture *displayTexture;
  sf::Uint8 *pixels;
  unsigned char Registers[8];
  void WriteRegister(unsigned short Register,unsigned char value);
  unsigned char ReadRegister(unsigned short Location);
  bool NMI_Fired;
private:
  int PPUClocks;
  int TileCounter;
  int CurrentPixel;
  int CurrentTile;
  int CurrentScanline;
  void RenderNametable(int Nametable, int OffsetX, int OffsetY);
  sf::Color GetColour(unsigned char NESColour);
  unsigned char *NESPixels;
  void DrawPixel(unsigned char value, int Scanline, int Pixel);
  void DrawPixelTest(unsigned char value, int Scanline, int Pixel, unsigned char ID); 
  sf::Sprite *displaySprite;
  unsigned char SetBit(int bit, bool val,unsigned char value);
  bool GetBit(int bit, unsigned char value);
  unsigned char PPUDRead();
  void PPUDWrite(unsigned char value);
  unsigned char NB();
  void SelectAddress(unsigned char value);
  int AddressSelectCounter;
  unsigned char DataAddresses[1];
  unsigned short db;
  //unsigned char Memory[0x3FFF]; // main PPU memory (16kB)
  unsigned char OAM[256];
  Nametable Nametables[3]; // PPU has 4 nametables.
  void WriteMemory(unsigned short Location, unsigned char value);
  void WriteNametable(unsigned short Location,unsigned char Value);
  unsigned char ReadNametable(unsigned short Location);
  unsigned char ReadNametableByte(int Pixel, int Scanline);
  void DisplayNametableID(unsigned char ID,int Pixel,int Scanline);
};
