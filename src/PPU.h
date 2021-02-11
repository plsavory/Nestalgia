enum PPUReg {
    PPUCTRL, PPUMASK, PPUSTATUS, OAMADDR, Latch1, Latch2, PPUSCROLL, PPUADDR, PPUDATA
};


struct NameTable {
    int type; // 0 = Mirrors NameTable 0, 1 = Mirrors NameTable 1, 2 = Standard
    unsigned char data[0x400];

    NameTable() {
        type = 2;

        for (unsigned char & i : data) {
            i = 0;
        }
    }
};

struct Palette {
    unsigned char Colours[3];
};

struct SpriteUnit {
public:
    SpriteUnit() {
        palette.Colours[0] = 0;
        palette.Colours[1] = 0;
        palette.Colours[2] = 0;
        bitmapLo = 0;
        bitmapHi = 0;
        XPos = 0;
        YPos = 0;
    }

    Palette palette;
    bool padding; // This is required to be here or the bitmapLo variable becomes corrupted - need to look at this in case we are writing out of the array's bounds somewhere
    unsigned char bitmapLo;
    unsigned char bitmapHi;
    unsigned char XPos;
    unsigned char YPos;
};

class PPU {

public:

    sf::Image *displayImage;
    sf::Texture *displayTexture;
    sf::Uint8 *pixels;
    unsigned char registers[8];
    bool NMIFired;
    bool CHRRAM;
    int nameTableMirrorMode;
    unsigned char OAMAddress;

    PPU();

    void reset();

    void execute(int PPUClock);

    void draw(sf::RenderWindow &window);

    void writeRegister(unsigned short registerId, unsigned char value);

    unsigned char readRegister(unsigned short location);

    void writeCROM(unsigned short location, unsigned char value);

    unsigned char readCROM(unsigned short location);

    void writeOAM(unsigned short location, unsigned char value);

    void writeScrollRegister(unsigned char value);

private:
    SpriteUnit *spriteUnits[8];
    int PPUClocks;
    int tileCounter;
    int currentCycle;
    int currentTile;
    int currentScanline;
    unsigned char XScroll;
    unsigned char YScroll;
    int addressSelectCounter;
    int scrollLatchCounter;
    unsigned char dataAddresses[2];
    unsigned short db; // Internal data bus register
    unsigned short idb;
    unsigned char tileBitmapXOffset; // Finex register (Contains currently drawing pixel)
    unsigned char finey;
    int pixelOffset;
    int bitmapLine;
    int currentAttribute;
    unsigned char bitmapLo;
    unsigned char bitmapHi;
    Palette currentPalette;
    unsigned char cROM[0x80000]; // 512KiB max - PPU has its own memory for CHR data, this will be copied over from the MemoryManager's cartridge upon ROM load
    int frame = 0;
    int OldAttribute;
    unsigned char tempOAM[0x20]; // Temporary OAM, holds the data for the sprites on the currently-rendering scanline. Should be filled by a sprite evaluation function executed during the previous scanline.
    int spritesOnThisScanline;
    int Sprite[8];
    bool spriteExists[8];
    bool SpriteZeroOnThisScanline;
    unsigned char OAM[256];
    NameTable Nametables[4]; // PPU has 4 nametables. (Extra one here for data padding - might need to look at later: Sprites get corrupted when writing to nametable 3 otherwise)
    bool pad;
    unsigned char PaletteMemory[0x20]; // Memory for storing colour palette information
    unsigned char *NESPixels;
    void RenderNametable(int Nametable, int OffsetX, int OffsetY);
    sf::Sprite *displaySprite;

    sf::Color getColour(unsigned char NESColour);

    void drawPixel(unsigned char value, int scanLine, int pixel);

    unsigned char setBit(int bit, bool val, unsigned char value);

    bool getBit(int bit, unsigned char value);

    unsigned char PPUDRead();

    void PPUDWrite(unsigned char value);

    unsigned char getNextByte();

    void selectAddress(unsigned char value);

    void selectOAMAddress(unsigned char value);

    void writeMemory(unsigned short location, unsigned char value);

    void writeOAM(unsigned char value);

    unsigned char readMemory(unsigned short location);

    void writeNameTable(unsigned short location, unsigned char value);

    unsigned char readNameTable(unsigned short location);

    /**
     * Fetch the bitmap data for the current tile
     * @param databus
     */
    void getBitmapDataFromNameTable(unsigned short databus);

    unsigned char readAttribute(int pixel, int scanline, int nameTable);

    void readColour(int attribute);

    void readColour(int attribute, int spriteId);

    unsigned char readPatternTable(unsigned short Location, int PatternType);

    void drawBitmapPixel(bool lo, bool hi, int pixel, int scanLine);

    void drawBitmapPixel(bool lo, bool hi, int pixel, int scanLine, int spriteId);

    unsigned char readPalette(unsigned short location);

    void evaluateSprites(int currentScanLine); // Fill tempOAM with the sprite data for the next scanline

    void writePalette(unsigned short location, unsigned char value);

    void setDataBus(int currentScanLine, int currentPixelXLocation);

    void renderSprites(int scanLine, int pixel);

};
