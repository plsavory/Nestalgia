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

struct Palette {
  unsigned char Colours[2];
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
  void WriteCROM(unsigned short Location,unsigned char Value);
  unsigned char ReadCROM(unsigned short Location);
  bool CHRRAM;
  int NametableMirrorMode;
  void WriteOAM(unsigned short Location, unsigned char value);
  unsigned char OAMAddress;
private:
  int PPUClocks;
  int TileCounter;
  int CurrentCycle;
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
  void SelectOAMAddress(unsigned char value);
  int AddressSelectCounter;
  unsigned char DataAddresses[1];
  unsigned short db; // Internal data bus register
  unsigned short idb;
  unsigned char finex; // Finex register (Contains currently drawing pixel)
  unsigned char finey;
  //unsigned char Memory[0x3FFF]; // main PPU memory (16kB)
  unsigned char OAM[256];
  Nametable Nametables[3]; // PPU has 4 nametables.
  unsigned char PaletteMemory[0x1F]; // Memory for storing colour palette information
  void WriteMemory(unsigned short Location, unsigned char value);
  void WriteOAM(unsigned char value);
  unsigned char ReadMemory(unsigned short Location);
  void WriteNametable(unsigned short Location,unsigned char Value);
  unsigned char ReadNametable(unsigned short Location);
  unsigned char ReadNametableByte(int Pixel, int Scanline);
  unsigned char ReadNametableByteb(unsigned short databus);
  unsigned char ReadAttribute(int Pixel, int Scanline,int Nametable);
  void ReadColour(int Attribute,int col);
  void DisplayNametableID(unsigned char ID,int Pixel,int Scanline);
  void RenderTilePixel(unsigned char ID, int Pixel, int Scanline);
  unsigned char ReadPatternTable(unsigned short Location, int PatternType);
  void DrawBitmapPixel(bool lo, bool hi,int Pixel,int Scanline);
  unsigned char ReadPalette(unsigned short Location);
  void EvaluateSprites(int Scanline); // Fill tempOAM with the sprite data for the next scanline
  void WritePalette(unsigned short Location,unsigned char Value);
  void SetDataBus(int Scanline, int Pixel);
  int PixelOffset;
  unsigned char nametablebyte;
  int bitmapline;
  int currentAttribute;
  unsigned char bitmapLo;
  unsigned char bitmapHi;
  Palette currentPalette;
  int PaletteX;
  int PaletteY;
  unsigned char cROM[0x80000]; // 512KiB max - PPU has its own memory for CHR data, this will be copied over from the MemoryManager's cartridge upon ROM load
  int frame = 0;
  int OldAttribute;
  unsigned char tempOAM[0x20]; // Temporary OAM, holds the data for the sprites on the currently-rendering scanline. Should be filled by a sprite evaluation function executed during the previous scanline.
  int spritesOnThisScanline;
  int Sprite[8];
  unsigned char SpriteBitmapLo[8];
  unsigned char SpriteBitmapHi[8];
  bool SpriteExists[8];
  bool SpriteZeroOnThisScanline;
  void RenderSprites(int Scanline,int Pixel);
  //int HexColours[] = {0x7C7C7C,0x0000FC,0x0000BC,0x4428BC,0x940084,0xA80020,0x81000,0x881400,0x503000,0x007800,0x006800,0x005800,0x004058,0x0,0x0,0x0};
};
