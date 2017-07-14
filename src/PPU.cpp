#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Cartridge.h"
#include "PPU.h"
#include <iostream>

#define PPULogging55
#define PPULOGNEWLINE55
#define DATABUSLOGGING66

PPU::PPU() {
  pixels = new sf::Uint8[256*240*4];
  NESPixels = new unsigned char[256*262]; // The NES PPU's internal render memory
  displayImage = new sf::Image();
  displayTexture = new sf::Texture;
  displaySprite = new sf::Sprite;
  displayTexture->create(256,240);
  displaySprite->setTexture(*displayTexture);
  displaySprite->setScale(3,3);
  NametableMirrorMode = 0;
  Reset();
  NMI_Fired = false;
  OldAttribute = 0;
  OAMAddress = 0;

  // Clear the temporary OAM
  for (int i = 0; i <0x20;i++)
    tempOAM[i] = 0;

  // Clear the sprite render units
  for (int i = 0; i < 8; i++)
    Sprite[i] = 0;

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
    NESPixels[i] = 0x0;

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

    // Render to the internal pixel buffer only if we're on a visible pixel (1 to 256 & scanlines 0 to 240)
    if (CurrentScanline == -1 && CurrentCycle == 1)
    {
          // Reset VBLANK flag
          Registers[2] = SetBit(7,0,Registers[2]);

          // Set the data bus for initial rendering
          int basenametable = GetBit(0,Registers[0]) + GetBit(1,Registers[0]);
          idb = basenametable << 10; // Register layout is yyyNNYYYYYXXXXX
    }

      // Get the nametable byte for this tile
      //if (PixelOffset == 0)
      int Pixel = CurrentCycle-1; // Account for the non-drawing cycle

      if (CurrentCycle == 0)
        finex = 7; // Reset finex on the idle scanline

      if (CurrentCycle >= 1 && CurrentCycle <= 256 && CurrentScanline <= 240 && CurrentScanline >= 0) {

      //if ((Pixel & 7) == 0) {
      if (finex==7) {
        //nametablebyte = ReadNametableByte(Pixel,CurrentScanline);
        SetDataBus(CurrentScanline,Pixel);
        nametablebyte = ReadNametableByteb(idb);
        idb++; // Increment the data bus to fetch the next tile
      }

      //}

      //if (CurrentScanline > 100)
      //std::cout<<"PixelTick"<<std::endl;
    //  if (nametablebyte != 0x0)
        //DrawPixelTest(0x33,CurrentScanline,Pixel,nametablebyte);

      // Next need to draw the actual pixel bitmap
      bool lo = GetBit(finex,bitmapLo);
      bool hi = GetBit(finex,bitmapHi);

      finex--;
      finex&=7; // 3-bit register, bit 4 should always be 0.

      // Read an attribute byte for it to display
      currentAttribute = ReadAttribute(Pixel,CurrentScanline,0); // For now just read from the first attribute table to make sure things are working

      ReadColour(currentAttribute,0);
      DrawBitmapPixel(lo,hi,Pixel,CurrentScanline);

      // Bit shift the bitmap for the next pixel
      //bitmapLo = (bitmapLo<<1);
      //bitmapHi = (bitmapHi<<1);

      PixelOffset++; // The pixel offset of the current tile

      // Reset the pixel counter if it goes above 7 as we'll need to fetch the next tile soon
      if (PixelOffset > 7)
        PixelOffset = 0;

      // Render the sprites for this scanline
      RenderSprites(CurrentScanline,Pixel);

      // Evaluate sprites for the next scanline at cycle 256
        if (CurrentCycle == 256)
          EvaluateSprites(CurrentScanline+1);
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
      {
        NMI_Fired = true;
        //std::cout<<"PPU: Fired NMI"<<std::endl;

        frame = 1; // Just so we don't get spammed by std::couts
      }


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

void PPU::RenderSprites(int Scanline, int Pixel) {
  // Check if any sprites need to be rendered on this pixel. if so, render them...
  int count = 7;

  while (count >= 0) {
    // Figure out if a sprite is on this pixel (if it exists in OAM too)
    if (SpriteExists[count]) {
      // The sprite exists on this scanline if this check passed
      if (Pixel >= tempOAM[Sprite[count]+3] && Pixel < tempOAM[Sprite[count]+3]+8)
      {
        // For now, render a solid colour where part of a sprite should be rendered

        bool bitlo = GetBit(7,SpriteBitmapLo[count]);
        bool bithi = GetBit(7,SpriteBitmapHi[count]);

        DrawBitmapPixel(bitlo,bithi,Pixel,Scanline);

        // Bit-shift the bitmap to the next pixel
        SpriteBitmapHi[count]<<=1;
        SpriteBitmapLo[count]<<=1;
      }
    }
    count--;
  }

}

void PPU::DrawBitmapPixel(bool lo, bool hi,int Pixel,int Scanline) {
  int pixelvalue = (lo + (hi << 1));
  unsigned char col = 0x5;
  switch (pixelvalue)
  {
    case 0:
      col = ReadPalette(0x3F00);
    break;
      case 1:
      col = currentPalette.Colours[0];
      //col = 0x24;
    break;
    case 2:
      col = currentPalette.Colours[1];
    //col = 0x1B;
    break;
    case 3:
      col = currentPalette.Colours[2];
      //col = 0x1;
    break;
  }

  DrawPixel(col,Scanline,Pixel);
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

void PPU::PPUDWrite(unsigned char value) {
  // Write the value to the PPU's memory, then increment the data bus
  WriteMemory(db,value);
  NB();
}

unsigned char PPU::PPUDRead() {
  unsigned char RetVal = 0;
  RetVal = ReadMemory(db);
  NB();
  return RetVal;
}

void PPU::WriteMemory(unsigned short Location, unsigned char value) {
  // Write a value to the PPU's memory

  #ifdef DATABUSLOGGING
    std::cout<<"PPU_DATA_WRITE $"<<std::hex<<(int)Location<<" = $"<<(int)value<<std::endl;
  #endif

  // Direct the data to the appropriate part of the PPU's memory...
  if (CHRRAM && Location <= 0x1FFF)
    cROM[Location] = value;

  if (Location >= 0x2000 && Location <= 0x2FFF)
    WriteNametable(Location,value); // Write to the appropriate nametable

  if (Location >= 0x3F00 && Location <= 0x3F1F)
    WritePalette(Location,value);
}

unsigned char PPU::ReadMemory(unsigned short Location) {
  if (Location <= 0x1FFF)
    return cROM[Location];

  if (Location >= 0x2000 && Location <= 0x2FFF)
    return ReadNametable(Location);

  if (Location >= 0x3F00 && Location <= 0x3F1F)
    return ReadPalette(Location);
  }

unsigned char PPU::NB() {
  // Increment the address bus depending on the value of the address increment flag
  int inc;

  if (GetBit(2,Registers[0]))
    db += 32;
  else
    db += 1;

  return db;
}


void PPU::SetDataBus(int Scanline, int Pixel) {
  // Returns the location in memory of the requested nametable entry
  int basenametable = GetBit(0,Registers[0]) + GetBit(1,Registers[0]);
  idb = basenametable << 10; // Register layout is yyyNNYYYYYXXXXX
  int coarsex = Scanline/8;
  int coarsey = Pixel/8;
  idb = idb+(coarsex << 5);
  idb = idb+coarsey;

  // Set the y position in the bitmap for this tile
  bitmapline = Scanline & 7;
}

unsigned char PPU::ReadNametableByteb(unsigned short databus) {
  // Get the Nametable byte for the current pixel (Right now just assume nametable 0)
  // Get the Nametable byte for the current pixel (Right now just assume nametable 0)

  //databus&=0xFFFF; // In the unlikely case that the most significant bit is set, un set it as we want to discard it.

  // Ignore the top 3 bits of the data bus as this is relating to fine scroll (the MSB is not used as this is a 15-bit register)
  unsigned short TileLocation = databus << 4;
  TileLocation = TileLocation >> 4;
  //TileLocation+=0x2000;

  // Get the current fine Y scroll
  //unsigned char finey = databus >> 12;

	// Should be called once per tile (so 33 times per scanline) or every 8 pixels
  unsigned char data = ReadNametable(0x2000+TileLocation);
  int PatternOffset = 0;
  int PatternToRead = (data)*16;

	// Fill the bitmap shift registers with the CHR data for this tile
	bitmapLo = ReadPatternTable(PatternOffset+(PatternToRead+bitmapline),0);
	bitmapHi = ReadPatternTable(PatternOffset+(8+PatternToRead+bitmapline),0);
  //std::cout<<(int)finey<<std::endl;

	return data;
}

void PPU::RenderTilePixel(unsigned char ID, int Pixel, int Scanline) {
      // This assumes that the "Pixel" number is bit shifted with each line
      // Fetch a pixel of character data from CHR and render it
      unsigned short Lo = ReadPatternTable(ID,0)<<CurrentCycle;
      unsigned short Hi = ReadPatternTable(8+ID,0)<<CurrentCycle;

      // Draw a pixel on screen of temporary colour for now
      int PixelColour = GetBit(7,Lo);
      int PixelColour1 = PixelColour + GetBit(7,Hi);

}

unsigned char PPU::ReadPatternTable(unsigned short Location, int PatternType) {
  // Pattern type 0 = Background, 1 = Sprites

  // Add more logic to this later for handling mappers
  unsigned short offset = 0x0;

  if (GetBit((4-PatternType),Registers[0]))
    offset = 0x1000;
  return cROM[offset+Location];
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
  if (AddressSelectCounter < 2) {
    // If we don't have two writes, that means the address has not been selected yet.
    DataAddresses[AddressSelectCounter] = value;
    AddressSelectCounter++;
  }

    // Reset the counter immediately after if it has reached 2
    if (AddressSelectCounter == 2) {
      db = ((unsigned short)DataAddresses[1]) + ((unsigned short)DataAddresses[0] << 8);
      //db &= 0x3FFF;
      AddressSelectCounter = 0;
      DataAddresses[0] = 0x0;
      DataAddresses[1] = 0x0;
      #ifdef DATABUSLOGGING
      std::cout<<"PPU_DATA Location Selected: $"<<std::hex<<(int)db<<std::endl;
      #endif

  }

}

void PPU::SelectOAMAddress(unsigned char value) {
  OAMAddress = value;
}

void PPU::WriteOAM(unsigned short Location, unsigned char value) {
  // Will be used to implement OAMDMA
  OAM[Location] = value;
}

void PPU::WriteOAM(unsigned char value) {
  // Perform a single write to OAM
  OAM[OAMAddress] = value;
  OAMAddress++;
}

unsigned char PPU::ReadRegister(unsigned short Location) {
  unsigned char RetVal;


  //  std::cout<<"PPU_Read_Register: $"<<(int)Location<<std::endl;

  switch (Location)
  {
    case 2:
    AddressSelectCounter = 0; // Reading from the PPUADDR register means the CPU wants to reset the address writing.
    RetVal = Registers[2]; // We need to clear the vblank flag when this register is read, so store its previous state here
    Registers[2] = SetBit(7,0,Registers[2]); // Clear the vblank flag
    return RetVal;
    case 0x7:
    return PPUDRead();
    break;
    default:
    return Registers[Location];
  }

}

unsigned char PPU::ReadNametable(unsigned short Location) {
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

    //unsigned short WriteLocation = Location-(0x400*TargetNametable);
    unsigned short ReadLocation = Location & 0x3FF;

    return Nametables[TargetNametable].Data[ReadLocation];
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

  //unsigned short WriteLocation = Location-(0x400*TargetNametable);
  unsigned short WriteLocation = Location & 0x3FF;
  #ifdef DATABUSLOGGING
  std::cout<<std::hex<<"NAMETABLE "<<(int) TargetNametable<<" WRITE at: $"<<(int)OldLocation<<" : $"<<(int)Location<< " : $"<<(int) WriteLocation<<" = $"<<(int)Value<<std::endl;
  #endif

  // Write the data to the Nametable
  if (NametableMirrorMode == 1) {
    // Vertical mirroring
    if (TargetNametable == 0 || TargetNametable == 1)
      {
        // Nametables 0 and 1 mirror eachother in this mode (2000 & 2400)
        Nametables[0].Data[WriteLocation] = Value;
        Nametables[1].Data[WriteLocation] = Value;
      } else
      {
        Nametables[2].Data[WriteLocation] = Value;
        Nametables[3].Data[WriteLocation] = Value;
      }
  }

  if (NametableMirrorMode == 0) {
    // Horizontal mirroring
    if (TargetNametable == 0 || TargetNametable == 2) {
      Nametables[0].Data[WriteLocation] = Value;
      Nametables[2].Data[WriteLocation] = Value;
    } else
    {
      Nametables[1].Data[WriteLocation] = Value;
      Nametables[2].Data[WriteLocation] = Value;
    }
  }

/*
  if (Nametables[TargetNametable].Type == 2)
      Nametables[TargetNametable].Data[WriteLocation] = Value; // Nametable is not mirrored, so write it directly in
  else
      Nametables[Nametables[TargetNametable].Type].Data[WriteLocation] = Value; // if type is not 3, this nametable mirrors to another one.
*/
}

void PPU::WriteRegister(unsigned short Register,unsigned char value) {
  switch (Register)
  {
    case 2:
      // Register 2 is read only
    break;
    case 3:
      // Select OAM address
      SelectOAMAddress(value);
    break;
    case 4:
      // Write to PPU OAM
      WriteOAM(value);
    break;
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

  // Write the 5 least significant bits of the written value to the PPUStatus register
  unsigned char lsb = value;
  lsb = SetBit(7,0,lsb);
  lsb = SetBit(6,0,lsb);
  lsb = SetBit(5,0,lsb);
  Registers[2] = Registers[2] + lsb;

  #ifdef PPULogging
  if (Register == 0) {
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

  std::cout<<std::endl;
}
  #endif

  #ifdef PPULOGNEWLINE
  if (Register == 0)
  std::cout<<" "<<std::endl;
  #endif

}

unsigned char PPU::ReadAttribute(int Pixel, int Scanline, int Nametable) {
  // Read the Attribute for this tile and return the colour palette that it should be using
  // 1 Atrribute byte covers a 32x32 pixel are of the screen (4x4 tiles)
  int AttributeX = (Pixel/8)/4;
  int AttributeY = ((Scanline/8)/4) * 8;

  //unsigned char Attribute = Nametables[Nametable].Data[0xC0+(AttributeX+AttributeY)];
  int Location = 0x23C0+(AttributeX+AttributeY);

  unsigned char Attribute = ReadNametable(Location);

  // Each attribute byte contains data for the colour palette listing of 4 tiles, so we need to figure out which tile we are currently working on
  int CurrentXTile = ((Pixel/8)/2) % 2;
  int CurrentYTile = ((Scanline/8)/2) % 2;

  unsigned char tempstore1 = (CurrentYTile << 1) | CurrentXTile; // The current tile number that we're on (0,1,2 or 3)

  unsigned char tempstore2 = (Attribute >> (tempstore1*2)) & 0x3;

  return tempstore2;

}

void PPU::EvaluateSprites(int Scanline) {
  // Fill tempOAM with the data for the sprites on this scanline
  spritesOnThisScanline = 0;
  SpriteZeroOnThisScanline = false;

  // Clear the temporary OAM
  for (int i = 0; i <0x20;i++)
    tempOAM[i] = 0;

  // Clear the sprite render units
  for (int i = 0; i < 8; i++) {
    Sprite[i] = 0;
    SpriteExists[i] = false;
  }

  for (int i = 0; i<64;i++) {
    // If the y position of the sprite matches the next scanline, add it to tempOAM

    // Pixel >= tempOAM[Sprite[count]+3] && Pixel < tempOAM[Sprite[count]+3]+8

    if (spritesOnThisScanline < 8) {

    int SpriteLocation = i*4; // The location in memory of the sprite data
    //if ((OAM[SpriteLocation] <= (Scanline+7)) && (OAM[SpriteLocation] >= (Scanline))) {
    if (Scanline >= OAM[SpriteLocation] && Scanline < (OAM[SpriteLocation]+8)) {
      tempOAM[(spritesOnThisScanline*4)] = OAM[SpriteLocation];
      tempOAM[(spritesOnThisScanline*4)+1] = OAM[SpriteLocation+1];
      tempOAM[(spritesOnThisScanline*4)+2] = OAM[SpriteLocation+2];
      tempOAM[(spritesOnThisScanline*4)+3] = OAM[SpriteLocation+3];


      // If sprite zero is on this scanline, set the internal flag to true
      if (SpriteLocation == 0)
        SpriteZeroOnThisScanline = true;

      // Assign this sprite to a sprite output unit
      Sprite[spritesOnThisScanline] = (spritesOnThisScanline*4);
      SpriteExists[spritesOnThisScanline] = true;

      // Fetch the bitmap data for this sprite
      //int TileBank = GetBit(0,tempOAM[spritesOnThisScanline*4)+1]); - for 8x16 tiles which is not yet implemented
      int TileBank = 0;

      if (GetBit(3,Registers[0]))
        TileBank = 0x1000; // Selection of the pattern table to use is done by writing to bit 3 of PPUCTRL register

      unsigned char TileID = (tempOAM[(spritesOnThisScanline*4)+1]);// >> 1)<<1; // Discard the least significant bit as we just extracted it above.

      int bitmapline = (Scanline-tempOAM[spritesOnThisScanline*4])&7;

      SpriteBitmapLo[spritesOnThisScanline] = ReadPatternTable(TileID+bitmapline,1);
      SpriteBitmapHi[spritesOnThisScanline] = ReadPatternTable(8+TileID+bitmapline,1);

      spritesOnThisScanline++;
    }
  } else {
    // If we have reached here, a sprite overflow has occured - so set the appropriate flag in the PPU's registers to alert the emulated CPU of this
  }

}

}

void PPU::ReadColour(int Attribute,int col) {
  int Location = 0x3F01+(Attribute*4);

  switch (Attribute) {
    case 0:
      Location = 0x3F01;
    break;
    case 1:
      Location = 0x3F05;
    break;
    case 2:
      Location = 0x3F09;
    break;
    case 3:
      Location = 0x3F0D;
    break;
  }

  currentPalette.Colours[0] = ReadPalette(Location);
  currentPalette.Colours[1] = ReadPalette(Location+1);
  currentPalette.Colours[2] = ReadPalette(Location+2);

}

unsigned char PPU::ReadPalette(unsigned short Location) {
  Location-=0x3F00;
  return PaletteMemory[Location];
}
void PPU::WritePalette(unsigned short Location, unsigned char Value) {

  Location-=0x3F00;

  // Handle the mirrored values
  if (Location == 0x0 || Location == 0x10) {
    PaletteMemory[0x0] = Value;
    PaletteMemory[0x10] = Value;
  } else if (Location == 0x04 || Location == 0x14) {
    PaletteMemory[0x04] = Value;
    PaletteMemory[0x14] = Value;
  } else if (Location == 0x08 || Location == 0x18) {
    PaletteMemory[0x08] = Value;
    PaletteMemory[0x18] = Value;
  } else if (Location == 0x0C || Location == 0x1C) {
    PaletteMemory[0x0C] = Value;
    PaletteMemory[0x1C] = Value;
  }
    PaletteMemory[Location] = Value;
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
