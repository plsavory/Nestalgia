enum CartRegion {NTSC,PAL,Dual};
enum FileType {iNESOriginal,iNES,iNES2};

struct Cartridge
{
	unsigned char header[16];
	unsigned char trainer[512];
	unsigned char PRGROM[0x200000]; // 2kb is the largest ROM size ever found in iNes format, so have this as a hard limit.
	unsigned char CHRROM[0x80000]; // 512KiB max
	unsigned char instROM[8192];
	unsigned char PROM[32];
	FileType fileFormat;
	int PRGRomSize;
	int CHRRomSize;
	int PRGRamSize;
	CartRegion region;
	bool trainerPresent;
	int mapper;
};
