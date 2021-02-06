#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Cartridge.h"
#include "PPU.h"
#include <iostream>

#define PPULogging55
#define PPULOGNEWLINE55
#define DATABUSLOGGING66

PPU::PPU() {
    pixels = new sf::Uint8[256 * 240 * 4];
    NESPixels = new unsigned char[256 * 262]; // The NES PPU's internal render memory
    displayImage = new sf::Image();
    displayTexture = new sf::Texture;
    displaySprite = new sf::Sprite;
    displayTexture->create(256, 240);
    displaySprite->setTexture(*displayTexture);
    displaySprite->setScale(3, 3);
    nameTableMirrorMode = 0;
    reset();
    NMIFired = false;
    OldAttribute = 0;
    OAMAddress = 0;

    // Clear the temporary OAM
    for (int i = 0; i < 0x20; i++) {
        tempOAM[i] = 0;
    }

    // Clear the sprite render units
    for (int i = 0; i < 8; i++) {
        Sprite[i] = 0;
    }

    // Create the sprite render units
    for (int i = 0; i < 8; i++) {
        spriteUnits[i] = new SpriteUnit();
    }

}

bool PPU::getBit(int bit, unsigned char value) {
    // Figures out the value of a current flag by AND'ing the flag register against the flag that needs extracting.
    return value & (1 << bit);
}

/**
 * Sets a bit on the given value and returns the result
 * @param bit - The bit number to change
 * @param val - The value to change the bit to
 * @param value - The value to change a bit on
 * @return - The changed value
 */
unsigned char
PPU::setBit(int bit, bool val, unsigned char value) // Used for setting flags to a value which is not the flag register
{
    return val ? value | (1 << bit) : value & ~(1 << bit);
}


void PPU::reset() {
    // Set the initial state of the PPU
    currentCycle = 0;
    currentScanline = 0;
    currentTile = 0;
    NMIFired = false; // If this is true, a NMI will be fired to the CPU
    addressSelectCounter = 0;
    scrollLatchCounter = 0;
    XScroll = 0;
    YScroll = 0;


    // Initialize the buffers (Set real SFML draw area to grey for now)
    for (int i = 0; i <= ((256 * 240) * 4); i += 4) {
        pixels[i] = 128; // R
        pixels[i + 1] = 128; // G
        pixels[i + 2] = 128; // B
        pixels[i + 3] = 255; // A
    }

    for (int i = 0; i <= (256 * 262); i++) {
        NESPixels[i] = 0x0;
    }

    for (int i = 0; i < 256; i++) {
        OAM[i] = 0x0;
    }

    // Have nametables 2 and 3 mirror the first two initially
    Nametables[2].type = 0;
    Nametables[3].type = 1;
}

void PPU::execute(int PPUClock) {

    PPUClocks = 0;

    while (PPUClocks < PPUClock) {
        // Perform one PPU cycle (Render one pixel - we don't need to replicate cycle by cycle memory accesses, just make sure it renders 1 pixel per cycle and timing will be fine.)

        // Every new scanline, we should fetch the appropriate data from the nametable for this scanline.
        // RenderNametable() function will handle the rest

        // Render to the internal pixel buffer only if we're on a visible pixel (1 to 256 & scanlines 0 to 240)
        if (currentScanline == -1 && currentCycle == 1) {
            // reset VBLANK flag
            registers[2] = setBit(7, 0, registers[2]);

            // Set the data bus for initial rendering
            int basenametable = getBit(0, registers[0]) + getBit(1, registers[0]);
            idb = basenametable << 10; // Register layout is yyyNNYYYYYXXXXX
        }

        // Get the nametable byte for this tile
        int Pixel = currentCycle - 1; // Account for the non-drawing cycle

        if (currentCycle == 0) {
            tileBitmapXOffset = 7; // reset tileBitmapXOffset on the idle scanline
        }

        if (currentCycle >= 1 && currentCycle <= 256 && currentScanline <= 240 && currentScanline >= 0) {

            // At the start of each tile on this scanline, fetch its bitmap data
            if (tileBitmapXOffset == 7) {
                setDataBus(currentScanline, Pixel);
                getBitmapDataFromNameTable(idb);
                idb++; // Increment the data bus to fetch the next tile
            }

            // Get what the value of the current pixel should be from the current tile's bitmap data
            bool lo = getBit(tileBitmapXOffset, bitmapLo);
            bool hi = getBit(tileBitmapXOffset, bitmapHi);

            tileBitmapXOffset--;
            tileBitmapXOffset &= 7; // 3-bit register, bit 4 should always be 0.

            // Read an attribute byte for it to display
            currentAttribute = readAttribute(Pixel, currentScanline,
                                             0); // For now just read from the first attribute table to make sure things are working

            readColour(currentAttribute);
            drawBitmapPixel(lo, hi, Pixel, currentScanline);

            pixelOffset++; // The pixel offset of the current tile

            // reset the pixel counter if it goes above 7 as we'll need to fetch the next tile soon
            if (pixelOffset > 7)
                pixelOffset = 0;

            // Render the sprites for this scanline
            renderSprites(currentScanline, Pixel);

            // Evaluate sprites for the next scanline at cycle 256
            if (currentCycle == 256)
                evaluateSprites(currentScanline + 1);
        }

        // Handle switching to the next scanline here
        if (currentCycle == 341) {
            currentCycle = 0;
            currentScanline++;
        }

        // VBLANK time - Set the VBLANK flag to true and fire an NMI to the CPU (if this functionality is enabled)
        if (currentScanline == 241 && currentCycle == 1) {
            // Fire VBLANK NMI if NMI_Enable is true
            // Set PPU's VBLANK flag
            registers[2] = setBit(7, 1, registers[2]);

            if (getBit(7, registers[0])) {
                NMIFired = true;
                frame = 1; // Just so we don't get spammed by std::couts
            }


#ifdef PPULogging
            std::cout<<"VBLANK_BEGIN"<<std::endl;
            std::cout<<(int)registers[2]<<std::endl;
#endif
        }

        // If we're on the last scanline, reset the counters to 0 for the next frame.
        if (currentScanline >= 260) {
            currentScanline = -1;
            currentCycle = 0;
            pixelOffset = 0;
        } else {
            PPUClocks++;
            currentCycle++;
        }

    }

    PPUClocks = 0;
}

void PPU::renderSprites(int scanLine, int pixel) {
    // Check if any sprites need to be rendered on this pixel. if so, render them...
    int count = 7;

    while (count >= 0) {
        // Figure out if a sprite is on this pixel (if it exists in OAM too)
        if (spriteExists[count]) {
            // The sprite exists on this scanline if this check passed
            if (pixel >= spriteUnits[count]->XPos && pixel < spriteUnits[count]->XPos + 8) {
                // Render the bit of the the sprite's bitmap data for this pixel
                bool bitlo = getBit(7, spriteUnits[count]->bitmapLo);
                bool bithi = getBit(7, spriteUnits[count]->bitmapHi);

                drawBitmapPixel(bitlo, bithi, pixel, scanLine, count);

                // Bit-shift the bitmap to the next pixel
                spriteUnits[count]->bitmapHi <<= 1;
                spriteUnits[count]->bitmapLo <<= 1;
            }
        }
        count--;
    }

}

void PPU::drawBitmapPixel(bool lo, bool hi, int pixel, int scanLine) {
    int pixelvalue = (lo + (hi << 1));
    unsigned char col = 0x5;
    switch (pixelvalue) {
        case 0:
            col = readPalette(0x3F00);
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

    drawPixel(col, scanLine, pixel);
}

void PPU::drawBitmapPixel(bool lo, bool hi, int pixel, int scanLine, int spriteId) {
    int pixelValue = (lo + (hi << 1));

    if (pixelValue != 0) {
        drawPixel(spriteUnits[spriteId]->palette.Colours[pixelValue - 1], scanLine, pixel);
    }
}

void PPU::drawPixel(unsigned char value, int scanLine, int pixel) {
    NESPixels[(scanLine * 256) +
              pixel] = value; // scanLine 0 is an idle scanline, so -1 so we don't overflow the pixel space here (so that pixel 256 actually appears on the right hand side)
}

void PPU::draw(sf::RenderWindow &window) {
    // Draws everything in the PPU's bitmap buffer to the window. should be called once per frame.
    // Could potentially be called in the PPU::execute function on the last clock of a frame.

    // Cycle through the NES's video output and convert it into a form for SFML to display
    int PixelInc = 0;
    for (int i = 0; i <= (256 * 239); i++) {
        // Get the colour of the current pixel
        sf::Color CurrentColour = getColour(NESPixels[i]);

        pixels[PixelInc] = CurrentColour.r;
        pixels[PixelInc + 1] = CurrentColour.g;
        pixels[PixelInc + 2] = CurrentColour.b;
        pixels[PixelInc + 3] = 255;
        PixelInc += 4;
    }

    displayTexture->update(pixels);
    if (window.isOpen()) {
        window.draw(*displaySprite);
    }
}

void PPU::RenderNametable(int Nametable, int OffsetX, int OffsetY) {
    // Renders 1 pixel of a NameTable

    if (tileCounter == 7) { // Each tile is 8 pixels in size, so increment this with each 8 pixels
        tileCounter = 0;
        currentTile++;
    }
}

void PPU::PPUDWrite(unsigned char value) {
    // Write the value to the PPU's memory, then increment the data bus
    writeMemory(db, value);
    getNextByte();
}

unsigned char PPU::PPUDRead() {
    unsigned char RetVal = 0;
    RetVal = readMemory(db);
    getNextByte();
    return RetVal;
}

void PPU::writeMemory(unsigned short location, unsigned char value) {
    // Write a value to the PPU's memory

#ifdef DATABUSLOGGING
    std::cout<<"PPU_DATA_WRITE $"<<std::hex<<(int)location<<" = $"<<(int)value<<std::endl;
#endif

    // Direct the data to the appropriate part of the PPU's memory...
    if (CHRRAM && location <= 0x1FFF)
        cROM[location] = value;

    if (location >= 0x2000 && location <= 0x2FFF)
        writeNameTable(location, value); // Write to the appropriate nametable

    if (location >= 0x3F00 && location <= 0x3F1F)
        writePalette(location, value);
}

unsigned char PPU::readMemory(unsigned short location) {
    if (location <= 0x1FFF) {
        return cROM[location];
    }

    if (location <= 0x2FFF) {
        return readNameTable(location);
    }

    if (location <= 0x3F1F) {
        return readPalette(location);
    }

    return 0x0; // Out of range
}

unsigned char PPU::getNextByte() {
    // Increment the address bus depending on the value of the address increment flag
    return (db += getBit(2, registers[0]) ? 32 : 1);
}


void PPU::setDataBus(int currentScanLine, int currentPixelXLocation) {
    // Work out values from scroll register
    unsigned short coarseyreg = (YScroll & 0xF8);

    unsigned short basenametable = getBit(1, registers[0]) + (getBit(0, registers[0]) << 1);

    idb = (basenametable << 10) + (coarseyreg << 2); // Register layout is yyyNNYYYYYXXXXX
    unsigned short yScrollOffset = (currentScanLine / 8);
    unsigned short xScrollOffset = currentPixelXLocation / 8;
    idb = idb + (yScrollOffset << 5);
    idb = idb + (xScrollOffset);
    idb &= 0x7FFF;

    // Set the y position in the bitmap for this tile
    bitmapLine = currentScanLine & 7;
}

void PPU::getBitmapDataFromNameTable(unsigned short databus) {

    // Ignore the top 3 bits of the data bus as this is relating to fine scroll (the MSB is not used as this is a 15-bit register)
    unsigned short TileLocation = databus;

    // Should be called once per tile (so 33 times per scanline) or every 8 pixels
    unsigned char data = readNameTable(0x2000 + TileLocation);
    unsigned short PatternOffset = 0;
    int PatternToRead = (data) * 16;

    // Fill the bitmap shift registers with the CHR data for this tile
    bitmapLo = readPatternTable(PatternOffset + (PatternToRead + bitmapLine), 0);
    bitmapHi = readPatternTable(PatternOffset + (8 + PatternToRead + bitmapLine), 0);
}

unsigned char PPU::readPatternTable(unsigned short Location, int PatternType) {
    // Pattern type 0 = Background, 1 = Sprites

    // Add more logic to this later for handling mappers
    unsigned short offset = 0x0;

    if (getBit((4 - PatternType), registers[0])) {
        offset = 0x1000;
    }
    return cROM[offset + Location];
}

void PPU::writeCROM(unsigned short location, unsigned char value) {
    // Will need to add more logic to these later for mappers
    cROM[location] = value;
}

unsigned char PPU::readCROM(unsigned short location) {
    // Will need to add more logic to these later for mappers
    return cROM[location];
}

void PPU::selectAddress(unsigned char value) {
    /* The first time the CPU writes to PPUData it is writing the msb of the target address
       The following byte is the lsb of the target address
       Need to store these somewhere and detect when two writes have occured. */
    if (addressSelectCounter < 2) {
        // If we don't have two writes, that means the address has not been selected yet.
        dataAddresses[addressSelectCounter] = value;
        addressSelectCounter++;
    }

    // reset the counter immediately after if it has reached 2
    if (addressSelectCounter == 2) {
        db = ((unsigned short) dataAddresses[1]) + ((unsigned short) dataAddresses[0] << 8);
        addressSelectCounter = 0;
        dataAddresses[0] = 0x0;
        dataAddresses[1] = 0x0;
#ifdef DATABUSLOGGING
        std::cout<<"PPU_DATA Location Selected: $"<<std::hex<<(int)db<<std::endl;
#endif

    }

}

void PPU::writeScrollRegister(unsigned char value) {
    // Is a latch - works like the above selectAddress function.
    if (addressSelectCounter == 0) {
        // Writing to X
        XScroll = value;
        addressSelectCounter++;
    } else {
        // Writing to Y
        YScroll = value;
        addressSelectCounter = 0;
    }
}

void PPU::selectOAMAddress(unsigned char value) {
    OAMAddress = value;
}

void PPU::writeOAM(unsigned short location, unsigned char value) {
    // Will be used to implement OAMDMA
    OAM[location] = value;
}

void PPU::writeOAM(unsigned char value) {
    // Perform a single write to OAM
    OAM[OAMAddress] = value;
    OAMAddress++;
}

unsigned char PPU::readRegister(unsigned short location) {
    unsigned char RetVal;

    switch (location) {
        case 2:
            addressSelectCounter = 0; // Reading from the PPUADDR register means the CPU wants to reset the address writing.
            RetVal = registers[2]; // We need to clear the vblank flag when this register is read, so store its previous state here
            registers[2] = setBit(7, 0, registers[2]); // Clear the vblank flag
            scrollLatchCounter = 0;
            return RetVal;
        case 0x7:
            return PPUDRead();
            break;
        default:
            return registers[location];
    }

}

unsigned char PPU::readNameTable(unsigned short location) {

    location -= 0x2000; // Strip off the first $2000 - we don't need it
    int TargetNametable = 0;

    if (location <= 0x3FF) {
        TargetNametable = 0;
    } else if (location <= 0x7FF) {
        TargetNametable = 1;
    } else if (location <= 0xBFF) {
        TargetNametable = 2;
    } else {
        TargetNametable = 3;
    }

    unsigned short ReadLocation = location & 0x3FF;

    return Nametables[TargetNametable].data[ReadLocation];
}

void PPU::writeNameTable(unsigned short location, unsigned char value) {
    location -= 0x2000; // Strip off the first $2000 - we don't need it
    int TargetNametable = 0;

    if (location <= 0x3FF) {
        TargetNametable = 0;
    } else if (location <= 0x7FF) {
        TargetNametable = 1;
    } else if (location <= 0xBFF) {
        TargetNametable = 2;
    } else {
        TargetNametable = 3;
    }

    //unsigned short WriteLocation = location-(0x400*TargetNametable);
    unsigned short WriteLocation = location & 0x3FF;
#ifdef DATABUSLOGGING
    std::cout<<std::hex<<"NAMETABLE "<<(int) TargetNametable<<" WRITE at: $"<<(int)OldLocation<<" : $"<<(int)location<< " : $"<<(int) WriteLocation<<" = $"<<(int)value<<std::endl;
#endif

    // Write the data to the NameTable
    if (nameTableMirrorMode == 0) {
        // Vertical mirroring
        if (TargetNametable == 0 || TargetNametable == 1) {
            // Nametables 0 and 1 mirror eachother in this mode (2000 & 2400)
            Nametables[0].data[WriteLocation] = value;
            Nametables[1].data[WriteLocation] = value;
        } else {
            Nametables[2].data[WriteLocation] = value;
            Nametables[3].data[WriteLocation] = value;
        }
    }

    if (nameTableMirrorMode == 1) {
        // Horizontal mirroring
        if (TargetNametable == 0 || TargetNametable == 2) {
            Nametables[0].data[WriteLocation] = value;
            Nametables[2].data[WriteLocation] = value;
        } else {
            Nametables[1].data[WriteLocation] = value;
            Nametables[3].data[WriteLocation] = value;
        }
    }
}

void PPU::writeRegister(unsigned short registerId, unsigned char value) {
    switch (registerId) {
        case 2:
            // registerId 2 is read only
            return;
        case 3:
            // Select OAM address
            selectOAMAddress(value);
            break;
        case 4:
            // Write to PPU OAM
            writeOAM(value);
            break;
        case 5:
            // Write to scroll register
            writeScrollRegister(value);
            break;
        case 6:
            selectAddress(value); // PPU is writing to PPUDATA, so handle address selecting
            break;
        case 7:
            PPUDWrite(value); // Write to PPU memory
            break;
        default:
            registers[registerId] = value;
            break;
    }

    // Write the 5 least significant bits of the written value to the PPUStatus register
    unsigned char lsb = value;
    lsb = setBit(7, false, lsb);
    lsb = setBit(6, false, lsb);
    lsb = setBit(5, false, lsb);
    registers[2] = registers[2] + lsb;

#ifdef PPULogging
    if (registerId == 0) {
    int value1 = (int)value;
    std::cout<<std::hex<<" ";
    switch (registerId)
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
    if (registerId == 0)
    std::cout<<" "<<std::endl;
#endif

}

unsigned char PPU::readAttribute(int pixel, int scanline, int nameTable) {
    // Read the Attribute for this tile and return the colour palette that it should be using
    // 1 Atrribute byte covers a 32x32 pixel are of the screen (4x4 tiles)
    int AttributeX = (pixel / 8) / 4;
    int AttributeY = ((scanline / 8) / 4) * 8;

    //unsigned char Attribute = Nametables[nameTable].data[0xC0+(AttributeX+AttributeY)];
    int Location = 0x23C0 + (AttributeX + AttributeY);

    unsigned char Attribute = readNameTable(Location);

    // Each attribute byte contains data for the colour palette listing of 4 tiles, so we need to figure out which tile we are currently working on
    int CurrentXTile = ((pixel / 8) / 2) % 2;
    int CurrentYTile = ((scanline / 8) / 2) % 2;

    unsigned char tempstore1 = (CurrentYTile << 1) | CurrentXTile; // The current tile number that we're on (0,1,2 or 3)

    unsigned char tempstore2 = (Attribute >> (tempstore1 * 2)) & 0x3;

    return tempstore2;

}

void PPU::evaluateSprites(int currentScanLine) {
    // Fill tempOAM with the data for the sprites on this scanline
    spritesOnThisScanline = 0;
    SpriteZeroOnThisScanline = false;

    // Clear the temporary OAM
    for (int i = 0; i < 0x20; i++) {
        tempOAM[i] = 0;
    }

    // Clear the sprite render units
    for (int i = 0; i < 8; i++) {
        Sprite[i] = 0;
        spriteExists[i] = false;
        spriteUnits[i]->bitmapHi = 0;
        spriteUnits[i]->bitmapLo = 0;
        spriteUnits[i]->palette.Colours[0] = 0;
        spriteUnits[i]->palette.Colours[1] = 0;
        spriteUnits[i]->palette.Colours[2] = 0;
        spriteUnits[i]->XPos = 0;
    }

    for (int i = 0; i < 64; i++) {
        // If the y position of the sprite matches the next scanline, add it to tempOAM
        if (spritesOnThisScanline < 8) {

            int SpriteLocation = i * 4; // The location in memory of the sprite data

            if (currentScanLine >= OAM[SpriteLocation] && currentScanLine < (OAM[SpriteLocation] + 8)) {
                tempOAM[(spritesOnThisScanline * 4)] = OAM[SpriteLocation];
                tempOAM[(spritesOnThisScanline * 4) + 1] = OAM[SpriteLocation + 1];
                tempOAM[(spritesOnThisScanline * 4) + 2] = OAM[SpriteLocation + 2];
                tempOAM[(spritesOnThisScanline * 4) + 3] = OAM[SpriteLocation + 3];


                // If sprite zero is on this scanline, set the internal flag to true
                if (SpriteLocation == 0)
                    SpriteZeroOnThisScanline = true;

                // Assign this sprite to a sprite output unit
                Sprite[spritesOnThisScanline] = (spritesOnThisScanline * 4);
                spriteExists[spritesOnThisScanline] = true;

                // Get the sprite's X Position and store it
                spriteUnits[spritesOnThisScanline]->XPos = tempOAM[(spritesOnThisScanline * 4) + 3];

                // Fetch the bitmap data for this sprite
                //int TileBank = getBit(0,tempOAM[spritesOnThisScanline*4)+1]); - for 8x16 tiles which is not yet implemented
                int TileBank = 0;

                if (getBit(3, registers[0]))
                    TileBank = 0x1000; // Selection of the pattern table to use is done by writing to bit 3 of PPUCTRL register

                int TileID = ((tempOAM[(spritesOnThisScanline * 4) + 1])) *
                             16;// >> 1)<<1)*16; // Discard the least significant bit as we just extracted it above.

                int bitmapline = (currentScanLine - tempOAM[spritesOnThisScanline * 4]) & 7;

                // If vertical mirroring is enabled, fetch bytes in reverse from normal
                if (getBit(7, tempOAM[(spritesOnThisScanline * 4) + 2]))
                    bitmapline = (~bitmapline) & 7;

                // Fetch the bitmap data for the sprite
                spriteUnits[spritesOnThisScanline]->bitmapLo = readPatternTable(TileID + bitmapline, 1);
                spriteUnits[spritesOnThisScanline]->bitmapHi = readPatternTable(8 + TileID + bitmapline, 1);

                // Handle horizontal mirroring
                if (getBit(6, tempOAM[(spritesOnThisScanline * 4) + 2])) {

                    unsigned char tmp1 = 0;
                    unsigned char tmp2 = 0;

                    for (int i = 0; i < 8; i++) {
                        tmp1 |= ((spriteUnits[spritesOnThisScanline]->bitmapLo >> i) & 1) << (7 - i);
                        tmp2 |= ((spriteUnits[spritesOnThisScanline]->bitmapHi >> i) & 1) << (7 - i);
                    }

                    spriteUnits[spritesOnThisScanline]->bitmapLo = tmp1;
                    spriteUnits[spritesOnThisScanline]->bitmapHi = tmp2;

                }

                // Figure out which colour palette the sprite is supposed to use, and store it.
                int Attribute = ((getBit(0, tempOAM[(spritesOnThisScanline * 4) + 2])) +
                                 (getBit(1, tempOAM[(spritesOnThisScanline * 4) + 2]) << 1) + 4);

                readColour(Attribute, spritesOnThisScanline);

                spritesOnThisScanline++;
            }
        } else {
            // todo If we have reached here, a sprite overflow has occured - so set the appropriate flag in the PPU's registers to alert the emulated CPU of this
        }

    }

}

void PPU::readColour(int attribute) {
    int Location = 0x3F01 + (attribute * 4);

    currentPalette.Colours[0] = readPalette(Location);
    currentPalette.Colours[1] = readPalette(Location + 1);
    currentPalette.Colours[2] = readPalette(Location + 2);

}

void PPU::readColour(int attribute, int spriteId) {
    int Location = 0x3F01 + (attribute * 4);
    spriteUnits[spriteId]->palette.Colours[0] = readPalette(Location);
    spriteUnits[spriteId]->palette.Colours[1] = readPalette(Location + 1);
    spriteUnits[spriteId]->palette.Colours[2] = readPalette(Location + 2);


}

unsigned char PPU::readPalette(unsigned short location) {
    location -= 0x3F00;
    return PaletteMemory[location];
}

void PPU::writePalette(unsigned short location, unsigned char value) {

    location -= 0x3F00;

    // Handle the mirrored values
    if (location == 0x0 || location == 0x10) {
        PaletteMemory[0x0] = value;
        PaletteMemory[0x10] = value;
    } else if (location == 0x04 || location == 0x14) {
        PaletteMemory[0x04] = value;
        PaletteMemory[0x14] = value;
    } else if (location == 0x08 || location == 0x18) {
        PaletteMemory[0x08] = value;
        PaletteMemory[0x18] = value;
    } else if (location == 0x0C || location == 0x1C) {
        PaletteMemory[0x0C] = value;
        PaletteMemory[0x1C] = value;
    }
    PaletteMemory[location] = value;
}

sf::Color PPU::getColour(unsigned char NESColour) {
    // Converts a NES colour to a sf::Color to be displayed on the actual emulator's output.
    sf::Color RetVal;

    // Nasty hack-ish solution, and some of the colours are wrong. Be sure to fix this when implementing real colour support.
    // Just use this for now to check that the PPU is drawing the correct values to the internal bitmap
    switch (NESColour) {
        case 0x00:
            return sf::Color(84, 84, 84, 255);
        case 0x01:
            return sf::Color(0, 30, 116, 255);
        case 0x02:
            return sf::Color(8, 16, 144, 255);
        case 0x03:
            return sf::Color(48, 0, 136, 255);
        case 0x04:
            return sf::Color(68, 0, 100, 255);
        case 0x05:
            return sf::Color(92, 0, 48, 255);
        case 0x06:
            return sf::Color(84, 4, 0, 255);
        case 0x07:
            return sf::Color(60, 24, 0, 255);
        case 0x08:
            return sf::Color(32, 42, 0, 255);
        case 0x09:
            return sf::Color(8, 58, 0, 255);
        case 0x0A:
            return sf::Color(0, 64, 0, 255);
        case 0x0B:
            return sf::Color(0, 60, 0, 255);
        case 0x0C:
            return sf::Color(0, 50, 60, 255);
        case 0x10:
            return sf::Color(152, 150, 152, 255);
        case 0x11:
            return sf::Color(8, 76, 196, 255);
        case 0x12:
            return sf::Color(48, 50, 236, 255);
        case 0x13:
            return sf::Color(92, 30, 228, 255);
        case 0x14:
            return sf::Color(136, 20, 176, 255);
        case 0x15:
            return sf::Color(160, 20, 100, 255);
        case 0x16:
            return sf::Color(152, 34, 32, 255);
        case 0x17:
            return sf::Color(120, 60, 0, 255);
        case 0x18:
            return sf::Color(84, 90, 0, 255);
        case 0x19:
            return sf::Color(40, 114, 0, 255);
        case 0x1A:
            return sf::Color(8, 124, 0, 255);
        case 0x1B:
            return sf::Color(0, 118, 40, 255);
        case 0x1C:
            return sf::Color(0, 102, 120, 255);
        case 0x20:
            return sf::Color(236, 238, 236, 255);
        case 0x21:
            return sf::Color(76, 154, 236, 255);
        case 0x22:
            return sf::Color(120, 124, 236, 255);
        case 0x23:
            return sf::Color(176, 98, 236, 255);
        case 0x24:
            return sf::Color(228, 84, 236, 255);
        case 0x25:
            return sf::Color(236, 88, 180, 255);
        case 0x26:
            return sf::Color(236, 106, 100, 255);
        case 0x27:
            return sf::Color(212, 136, 32, 255);
        case 0x28:
            return sf::Color(160, 170, 0, 255);
        case 0x29:
            return sf::Color(116, 196, 0, 255);
        case 0x2A:
            return sf::Color(76, 208, 32, 255);
        case 0x2B:
            return sf::Color(56, 204, 108, 255);
        case 0x2C:
            return sf::Color(56, 180, 204, 255);
        case 0x2D:
            return sf::Color(236, 238, 236, 255);
        case 0x30:
            return sf::Color(168, 204, 236, 255);
        case 0x31:
            return sf::Color(118, 118, 236, 255);
        case 0x32:
            return sf::Color(212, 178, 236, 255);
        case 0x33:
            return sf::Color(236, 174, 236, 255);
        case 0x34:
            return sf::Color(236, 164, 212, 255);
        case 0x35:
            return sf::Color(236, 180, 176, 255);
        case 0x36:
            return sf::Color(228, 196, 144, 255);
        case 0x37:
            return sf::Color(204, 210, 120, 255);
        case 0x38:
            return sf::Color(180, 222, 120, 255);
        case 0x39:
            return sf::Color(168, 226, 144, 255);
        case 0x3A:
            return sf::Color(152, 226, 180, 255);
        case 0x3B:
            return sf::Color(160, 214, 228, 255);
        case 0x3C:
            return sf::Color(160, 162, 160, 255);
        default:
            return sf::Color(0, 0, 0, 255);
    }
}
