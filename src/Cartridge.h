enum CartRegion {NTSC,PAL,Dual};
enum FileType {iNESOriginal,iNES,iNES2};

struct Cartridge
{
	unsigned char Header[16];
	unsigned char Trainer[512];
	unsigned char pROM[0x200000]; // 2kb is the largest ROM size ever found in iNes format, so have this as a hard limit.
	unsigned char cROM[0x80000]; // 512KiB max
	unsigned char InstROM[8192];
	unsigned char PROM[32];
	FileType FileFormat;
	int PRGRomSize;
	int CHRRomSize;
	int PRGRamSize;
	CartRegion Region;
	bool TrainerPresent;
	int Mapper;
};
