#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Cartridge.h"
#include "PPU.h"
#include <iostream>

#define PPULogging55
#define PPULOGNEWLINE66
#define DATABUSLOGGING

PPU::PPU() {
  pixels = new sf::Uint8[256*240*4];
  NESPixels = new unsigned char[256*262]; // The NES PPU's internal render memory
  displayImage = new sf::Image();
  displayTexture = new sf::Texture;
  displaySprite = new sf::Sprite;
  displayTexture->create(256,240);
  displaySprite->setTexture(*displayTexture);
  displaySprite->setScale(3,3);
  Reset();
  NMI_Fired = false;
}

bool PPU::GetBit(int bit, unsigned char value)
{
	// Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
	if (value & (1 << bit))
		return true;
	else
		return false;
}

unsigned char PPU::SetBit(int bit, bool val, unsigned char value) // Used for setting flags to a value which is not the flag register
{
	// Sets a specific flag to true or false depending on the value of "val"
	unsigned char RetVal = value;
	if (val)
		RetVal |= (1 << bit);
	else
		RetVal &= ~(1 << bit);

	return RetVal;
}


void PPU::Reset() {
  // Set the initial state of the PPU
  CurrentCycle = 0;
  CurrentScanline = 0;
  CurrentTile = 0;
  NMI_Fired = false; // If this is true, a NMI will be fired to the CPU
  AddressSelectCounter = 0;


  // Initialize the buffers (Set real SFML draw area to grey for now)
  for (int i = 0; i <= ((256*240)*4);i+=4)
  {
    pixels[i] = 128; // R
    pixels[i+1] = 128; // G
    pixels[i+2] = 128; // B
    pixels[i+3] = 255; // A
  }

  for (int i = 0; i <= (256*262); i++)
    NESPixels[i] = 0x15;

  // Zero-out the PPU's memory
  //for (int i = 0; i < 0x3FFF;i++)
    	//Memory[i] = 0x0;

  for (int i = 0; i <256;i++)
    	OAM[i] = 0x0;

  // Have nametables 2 and 3 mirror the first two initially
  Nametables[2].Type = 0;
  Nametables[3].Type = 1;
}

void PPU::Execute(float PPUClock) {

  PPUClocks = 0;

    while (PPUClocks < PPUClock) {
    // Perform one PPU cycle (Render one pixel - we don't need to replicate cycle by cycle memory accesses, just make sure it renders 1 pixel per cycle and timing will be fine.)

    // Every new scanline, we should fetch the appropriate data from the nametable for this scanline.
    // RenderNametable() function will handle the rest
    RenderNametable(1,0,0);

    // Render to the internal pixel buffer only if we're on a visible pixel (1 to 256 & scanlines 0 to 240)
    if (CurrentScanline == -1 && CurrentCycle == 1)
    {
          // Reset VBLANK flag
          Registers[2] = SetBit(7,0,Registers[2]);
    }

      // Get the nametable byte for this tile
      //if (PixelOffset == 0)
      int Pixel = CurrentCycle-1; // Account for the non-drawing cycle

      if (CurrentCycle >= 1 && CurrentCycle <= 256 && CurrentScanline <= 240) {
        DrawPixel(0x0,CurrentScanline,Pixel); // For now just render a solid colour

      if ((Pixel & 7) == 0)
        nametablebyte = ReadNametableByte(Pixel,CurrentScanline);

      if (nametablebyte != 0x0)
        DrawPixelTest(0x33,CurrentScanline,Pixel,nametablebyte);

      // Next need to draw the actual pixel bitmap

      // Bit shift the bitmap for the next pixel
      bitmapLo = (bitmapLo<<1);
      bitmapHi = (bitmapHi<<1);

      PixelOffset++; // The pixel offset of the current tile

      // Reset the pixel counter if it goes above 7 as we'll need to fetch the next tile soon
      if (PixelOffset > 7)
        PixelOffset = 0;
    }

    // Handle switching to the next scanline here
    if (CurrentCycle == 341) {
      CurrentCycle = 0;
      CurrentScanline++;
    }

    // VBLANK time - Set the VBLANK flag to true and fire an NMI to the CPU (if this functionality is enabled)
    if (CurrentScanline == 241 && CurrentCycle == 1)
    {
      // Fire VBLANK NMI if NMI_Enable is true
      // Set PPU's VBLANK flag
      Registers[2] = SetBit(7,1,Registers[2]);

      if (GetBit(7,Registers[0]))
        NMI_Fired = true;

      #ifdef PPULogging
        std::cout<<"VBLANK_BEGIN"<<std::endl;
        std::cout<<(int)Registers[2]<<std::endl;
      #endif
    }

    // If we're on the last scanline, reset the counters to 0 for the next frame.
    if (CurrentScanline >= 260) {
      CurrentScanline = -1;
      CurrentCycle = 0;
      PixelOffset = 0;
    } else {
    PPUClocks++;
    CurrentCycle++;
  }

  }

  PPUClocks = 0;
}

void PPU::DrawBitmapPixel(int pixeldata,int Pixel,int Scanline) {
}

void PPU::DrawPixel(unsigned char value, int Scanline, int Pixel) {
  NESPixels[(Scanline*256)+Pixel-1] = value; // Scanline 0 is an idle scanline, so -1 so we don't overflow the pixel space here (so that pixel 256 actually appears on the right hand side)
}

void PPU::DrawPixelTest(unsigned char value, int Scanline, int Pixel, unsigned char ID) {
  NESPixels[(Scanline*256)+Pixel-1] = value-ID;
}

void PPU::Draw(sf::RenderWindow &mWindow) {
    // Draws everything in the PPU's bitmap buffer to the window. should be called once per frame.
    // Could potentially be called in the PPU::Execute function on the last clock of a frame.

    // Cycle through the NES's video output and convert it into a form for SFML to display
    int PixelInc = 0;
    for (int i = 0; i <= (256*239); i++)
    {
      // Get the colour of the current pixel
      sf::Color CurrentColour = GetColour(NESPixels[i]);

      pixels[PixelInc] = CurrentColour.r;
      pixels[PixelInc+1] = CurrentColour.g;
      pixels[PixelInc+2] = CurrentColour.b;
      pixels[PixelInc+3] = 255;
      PixelInc+=4;
    }

    displayTexture->update(pixels);
    if (mWindow.isOpen()) {
    mWindow.draw(*displaySprite);
    }
}

void PPU::RenderNametable(int Nametable, int OffsetX, int OffsetY) {
  // Renders 1 pixel of a Nametable

  if (TileCounter == 7) { // Each tile is 8 pixels in size, so increment this with each 8 pixels
    TileCounter = 0;
    CurrentTile++;
  }
}

// CPU Read/Write functions - handle when the CPU writes or reads the PPUDATA registers
unsigned char PPU::PPUDRead() {

}

void PPU::PPUDWrite(unsigned char value) {
  // Write the value to the PPU's memory, then increment the data bus
  WriteMemory(db,value);
  NB();
}

void PPU::WriteMemory(unsigned short Location, unsigned char value) {
  // Write a value to the PPU's memory
  #ifdef DATABUSLOGGING
    std::cout<<"PPU_DATA_WRITE $"<<std::hex<<(int)Location<<" = $"<<(int)value<<std::endl;
  #endif

  // Direct the data to the appropriate part of the PPU's memory...
  if (Location >= 0x2000 && Location <= 0x2FFF)
    WriteNametable(Location,value); // Write to the appropriate nametable
}

unsigned char PPU::NB() {
  // Increment the address bus depending on the value of the address increment flag
  int inc = 1 + (GetBit(Registers[0],2)*31);
  return (db+=inc);
}

unsigned char PPU::ReadNametableByte(int Pixel, int Scanline) {
  // Get the Nametable byte for the current pixel (Right now just assume nametable 0)
  // Get the Nametable byte for the current pixel (Right now just assume nametable 0)
	// Should be called once per tile (so 33 times per scanline) or every 8 pixels
	int TileX = (Pixel / 8);
	int TileY = (Scanline / 8) * 32;
	int CurrentpxLine;

	if (Scanline > 7)
		//CurrentpxLine = (((Scanline/8) - 1) * 8) - Scanline;
    CurrentpxLine = Scanline & 7;
	else
		CurrentpxLine = Scanline;

/*
  if (Scanline >= 0 && Scanline <= 256) {
   std::cout<<"Current Scanline:"<<std::dec<<(int)Scanline<< " ";
	 std::cout<<"Current Line of tile: " <<std::dec<< (int)CurrentpxLine << std::endl;
 }
 */
	// Get the ID of the bitmap data for target nametable entry
	unsigned char data = Nametables[0].Data[TileX + TileY];

	// Fill the bitmap shift registers with the CHR data for this tile
	//unsigned short bitmapLo = PPU::ReadPatternTable(data+CurrentLine);
	//unsigned short bitmapHi = PPU::ReadPatternTable(8+data+CurrentLine);
	bitmapLo = ReadPatternTable(data+CurrentpxLine);
	bitmapHi = ReadPatternTable(8 + data+CurrentpxLine);

	return data;
}

void PPU::RenderTilePixel(unsigned char ID, int Pixel, int Scanline) {
      // This assumes that the "Pixel" number is bit shifted with each line
      // Fetch a pixel of character data from CHR and render it
      unsigned short Lo = PPU::ReadPatternTable(ID)<<CurrentCycle;
      unsigned short Hi = PPU::ReadPatternTable(8+ID)<<CurrentCycle;

      // Draw a pixel on screen of temporary colour for now
      int PixelColour = GetBit(7,Lo);
      int PixelColour1 = PixelColour + GetBit(7,Hi);

}

unsigned char PPU::ReadPatternTable(unsigned short Location) {
  // Add more logic to this later for handling mappers
  return cROM[Location];
}

void PPU::WriteCROM(unsigned short Location,unsigned char Value) {
  // Will need to add more logic to these later for mappers
  cROM[Location] = Value;
}
unsigned char PPU::ReadCROM(unsigned short Location) {
  // Will need to add more logic to these later for mappers
  return cROM[Location];
}

void PPU::DisplayNametableID(unsigned char ID,int Pixel,int Scanline) {
  // For debug reasons - Draw the nametable ID (as text) to the position where it should be
}
void PPU::SelectAddress(unsigned char value) {
  // The first time the CPU writes to PPUData it is writing the msb of the target address
  // The following byte is the lsb of the target address
  // Need to store these somewhere and detect when two writes have occured.

  if (AddressSelectCounter == 2) {
    db = ((unsigned short)DataAddresses[1]) + ((unsigned short)DataAddresses[0] << 8);
    AddressSelectCounter = 0;

    #ifdef DATABUSLOGGING
    std::cout<<"PPU_DATA Location Selected: $"<<std::hex<<(int)db<<std::endl;
    #endif

  }
  else {
    // If we don't have two writes, that means the address has not been selected yet.
    DataAddresses[AddressSelectCounter] = value;
    AddressSelectCounter++;
  }
}

unsigned char PPU::ReadRegister(unsigned short Location) {
  unsigned char RetVal;

  switch (Location)
  {
    case 2:
    AddressSelectCounter = 0; // Reading from the PPUADDR register means the CPU wants to reset the address writing.
    RetVal = Registers[2]; // We need to clear the vblank flag when this register is read, so store its previous state here
    Registers[2] = SetBit(7,0,Registers[2]); // Clear the vblank flag

    return RetVal;
    default:
    return Registers[Location];
  }
}

void PPU::WriteNametable(unsigned short Location,unsigned char Value) {
  unsigned short OldLocation = Location;
  Location-=0x2000; // Strip off the first $2000 - we don't need it
  int TargetNametable = 0;

  if (Location <= 0x3FF) {
    TargetNametable = 0;
  } else if (Location <= 0x7FF) {
    TargetNametable = 1;
  } else if (Location <= 0xBFF) {
    TargetNametable = 2;
  } else {
    TargetNametable = 3;
  }

  unsigned short WriteLocation = Location-(0x400*TargetNametable);

  #ifdef DATABUSLOGGING
  std::cout<<std::hex<<"NAMETABLE "<<(int) TargetNametable<<" WRITE at: $"<<(int)OldLocation<<" : $"<<(int)Location<< " : $"<<(int) WriteLocation<<" = $"<<(int)Value<<std::endl;
  #endif

  // Write the data to the Nametable
  if (Nametables[TargetNametable].Type == 2)
      Nametables[TargetNametable].Data[WriteLocation] = Value; // Nametable is not mirrored, so write it directly in
  else
      Nametables[Nametables[TargetNametable].Type].Data[WriteLocation] = Value; // if type is not 3, this nametable mirrors to another one.
}

void PPU::WriteRegister(unsigned short Register,unsigned char value) {
  switch (Register)
  {
    case 6:
    SelectAddress(value); // PPU is writing to PPUDATA, so handle address selecting
    break;
    case 7:
    PPUDWrite(value); // Write to PPU memory
    break;
    default:
    Registers[Register] = value;
    break;
  }

  #ifdef PPULogging

  int value1 = (int)value;
  std::cout<<std::hex<<" ";
  switch (Register)
  {
    case 0:
      std::cout<<"PPUCTRL = $"<<value1;
    break;
    case 1:
      std::cout<<"PPUSTATUS = $"<<value1;
    break;
    case 2:
      std::cout<<"OAMADDR = $"<<value1;
    break;
    case 3:
      std::cout<<"OAMDATA = $"<<value1;
    break;
    case 4:
      std::cout<<"PPUSCROLL = $"<<value1;
    break;
    case 5:
      std::cout<<"PPUADDR = $"<<value1;
    break;
    case 6:
      std::cout<<"PPUDATA = $"<<value1;
    break;
    case 7:
      std::cout<<"OAMDMA = $"<<value1;
    break;
  }
  #endif

  #ifdef PPULOGNEWLINE
  std::cout<<" "<<std::endl;
  #endif

}

sf::Color PPU::GetColour(unsigned char NESColour) {
  // Converts a NES colour to a sf::Color to be displayed on the actual emulator's output.
  sf::Color RetVal;

  // Nasty hack-ish solution, and some of the colours are wrong. Be sure to fix this when implementing real colour support.
  // Just use this for now to check that the PPU is drawing the correct values to the internal bitmap
  switch (NESColour)
  {
    case 0x00:
    return sf::Color(84,84,84,255);
    case 0x01:
    return sf::Color(0,30,116,255);
    case 0x02:
    return sf::Color(8,16,144,255);
    case 0x03:
    return sf::Color(48,0,136,255);
    case 0x04:
    return sf::Color(68,0,100,255);
    case 0x05:
    return sf::Color(92,0,48,255);
    case 0x06:
    return sf::Color(84,4,0,255);
    case 0x07:
    return sf::Color(60,24,0,255);
    case 0x08:
    return sf::Color(32,42,0,255);
    case 0x09:
    return sf::Color(8,58,0,255);
    case 0x0A:
    return sf::Color(0,64,0,255);
    case 0x0B:
    return sf::Color(0,60,0,255);
    case 0x0C:
    return sf::Color(0,50,60,255);
    case 0x10:
    return sf::Color(152,150,152,255);
    case 0x11:
    return sf::Color(8,76,196,255);
    case 0x12:
    return sf::Color(48,50,236,255);
    case 0x13:
    return sf::Color(92,30,228,255);
    case 0x14:
    return sf::Color(136,20,176,255);
    case 0x15:
    return sf::Color(160,20,100,255);
    case 0x16:
    return sf::Color(152,34,32,255);
    case 0x17:
    return sf::Color(120,60,0,255);
    case 0x18:
    return sf::Color(84,90,0,255);
    case 0x19:
    return sf::Color(40,114,0,255);
    case 0x1A:
    return sf::Color(8,124,0,255);
    case 0x1B:
    return sf::Color(0,118,40,255);
    case 0x1C:
    return sf::Color(0,102,120,255);
    case 0x20:
    return sf::Color(236,238,236,255);
    case 0x21:
    return sf::Color(76,154,236,255);
    case 0x22:
    return sf::Color(120,124,236,255);
    case 0x23:
    return sf::Color(176,98,236,255);
    case 0x24:
    return sf::Color(228,84,236,255);
    case 0x25:
    return sf::Color(236,88,180,255);
    case 0x26:
    return sf::Color(236,106,100,255);
    case 0x27:
    return sf::Color(212,136,32,255);
    case 0x28:
    return sf::Color(160,170,0,255);
    case 0x29:
    return sf::Color(116,196,0,255);
    case 0x2A:
    return sf::Color(76,208,32,255);
    case 0x2B:
    return sf::Color(56,204,108,255);
    case 0x2C:
    return sf::Color(56,180,204,255);
    case 0x2D:
    return sf::Color(236,238,236,255);
    case 0x30:
    return sf::Color(168,204,236,255);
    case 0x31:
    return sf::Color(118,118,236,255);
    case 0x32:
    return sf::Color(212,178,236,255);
    case 0x33:
    return sf::Color(236,174,236,255);
    case 0x34:
    return sf::Color(236,164,212,255);
    case 0x35:
    return sf::Color(236,180,176,255);
    case 0x36:
    return sf::Color(228,196,144,255);
    case 0x37:
    return sf::Color(204,210,120,255);
    case 0x38:
    return sf::Color(180,222,120,255);
    case 0x39:
    return sf::Color(168,226,144,255);
    case 0x3A:
    return sf::Color(152,226,180,255);
    case 0x3B:
    return sf::Color(160,214,228,255);
    case 0x3C:
    return sf::Color(160,162,160,255);
    default:
    return sf::Color(0,0,0,255);
  }
}
