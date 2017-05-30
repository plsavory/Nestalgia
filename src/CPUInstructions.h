#pragma once

// Defines the names of the CPU instructions. Allows the names of the instructions to be used in the source code as well as allowing me to keep track of the emulator development progress.

// ADC instructions
#define ADC_IMM 0x69
#define ADC_ZP 0x65
#define ADC_ZPX 0x75
#define ADC_AB 0x6D
#define ADC_ABX 0x7D
#define ADC_ABY 0x79
#define ADC_INX 0x61
#define ADC_INY 0x71

// AND instructions
#define AND_IMM 0x29
#define AND_ZP 0x25
#define AND_ZPX 0x35
#define AND_AB 0x2D
#define AND_ABX 0x3D
#define AND_ABY 0x39
#define AND_INX 0x21
#define AND_INY 0x31

// ASL instructions
#define ASL_ACC 0x0A
#define ASL_ZP 0x06
#define ASL_ZPX 0x16
#define ASL_AB 0x0E
#define ASL_ABX 0x1E

// Branch instructions - Cycles +=1 if branch is on same page, +=2 if its on a different page
#define BCC 0x90
#define BCS 0xB0
#define BEQ 0xF0
#define BMI 0x30
#define BNE 0xD0
#define BPL 0x10
#define BVC 0x50
#define BVS 0x70

// Bit test instructions
#define BIT_ZP 0x24
#define BIT_AB 0x2C

// Flag clear instructions
#define CLC 0x18
#define CLD 0xD8
#define CLI 0x58
#define CLV 0xB8

// Compare instructions
#define CMP_IMM 0xC9
#define CMP_ZP 0xC5
#define CMP_ZPX 0xD5
#define CMP_AB 0xCD
#define CMP_ABX 0xDD
#define CMP_ABY 0xD9
#define CMP_INX 0xC1
#define CMP_INY 0xD1
#define CPX_IMM 0xE0
#define CPX_ZP 0xE4
#define CPX_AB 0xEC
#define CPY_IMM 0xC0
#define CPY_ZP 0xC4
#define CPY_AB 0xCC

// Decrement instructions
#define DEC_ZP 0xC6
#define DEC_ZPX 0xD6
#define DEC_AB 0xCE
#define DEC_ABX 0xDE
#define DEX 0xCA
#define DEY 0x88

// Eclusive OR instructions
#define EOR_IMM 0x49
#define EOR_ZP 0x45
#define EOR_ZPX 0x55
#define EOR_AB 0x4D
#define EOR_ABX 0x5D
#define EOR_ABY 0x59
#define EOR_INX 0x41
#define EOR_INY 0x51

// Increment instructions
#define INC_ZP 0xE6
#define INC_ZPX 0xF6
#define INC_AB 0xEE
#define INC_ABX 0xFE
#define INX 0xE8
#define INY 0xC8

// Jump instructions
#define JMP_AB 0x4C
#define JMP_IN 0x6C
#define JSR 0x20

// Load instructions
#define LDA_IMM 0xA9
#define LDA_ZP 0xA5
#define LDA_ZPX 0xB5
#define LDA_AB 0xAD
#define LDA_ABX 0xBD
#define LDA_ABY 0xB9
#define LDA_INX 0xA1
#define LDA_INY 0xB1
#define LDX_IMM 0xA2
#define LDX_ZP 0xA6
#define LDX_ZPY 0xB6
#define LDX_AB 0xAE
#define LDX_ABY 0xBE
#define LDY_IMM 0xA0
#define LDY_ZP 0xA4
#define LDY_ZPX 0xB4
#define LDY_AB 0xAC
#define LDY_ABX 0xBC

// Bit shift instructions
#define LSR_ACC 0x4A
#define LSR_ZP 0x46
#define LSR_ZPX 0x56
#define LSR_AB 0x4E
#define LSR_ABX 0x5E

// No operation
#define NOP 0xEA

// Inclusive or instructions
#define ORA_IMM 0x09
#define ORA_ZP 0x05
#define ORA_ZPX 0x15
#define ORA_AB 0x0D
#define ORA_ABX 0x1D
#define ORA_ABY 0x19
#define ORA_INX 0x01
#define ORA_INY 0x11

// Stack instructions
#define PHA 0x48
#define PHP 0x08
#define PLA 0x68
#define PLP 0x28

// Rotate instructions
#define ROL_ACC 0x2A
#define ROL_ZP 0x26
#define ROL_ZPX 0x36
#define ROL_AB 0x2E
#define ROL_ABX 0x3E
#define ROR_ACC 0x6A
#define ROR_ZP 0x66
#define ROR_ZPX 0x76
#define ROR_AB 0x6E
#define ROR_ABX 0x7E

// Return instructions
#define RTI 0x40
#define RTS 0x60

// Subtract instructions
#define SBC_IMM 0xE9
#define SBC_ZP 0xE5
#define SBC_ZPX 0xF5
#define SBC_AB 0xED
#define SBC_ABX 0xFD
#define SBC_ABY 0xF9
#define SBC_INX 0xE1
#define SBC_INY 0xF1

// Set flags
#define SEC 0x38
#define SED 0xF8
#define SEI 0x78

// Store instructions
#define STA_ZP 0x85
#define STA_ZPX 0x95
#define STA_AB 0x8D
#define STA_ABX 0x9D
#define STA_ABY 0x99
#define STA_INX 0x81
#define STA_INY 0x91
#define STX_ZP 0x86
#define STX_ZPY 0x96
#define STX_AB 0x8E
#define STY_ZP 0x84
#define STY_ZPX 0x94
#define STY_AB 0x8C

// Transfer instructions
#define TAX 0xAA
#define TAY 0xA8
#define TSX 0xBA
#define TXA 0x8A
#define TXS 0x9A
#define TYA 0x98

namespace M6502
{

	enum Flag: unsigned char {
		Carry = 1 << 0,
		Zero = 1 << 1,
		EInterrupt = 1 << 2, // Disables interrupts (if set)
		BCDMode = 1 << 3,
		SInterrupt = 1 << 4, // Set when a BRK instruction is executed
		Unused = 1 << 5,
		Overflow = 1 << 6,
		Sign = 1 << 7
	};

	enum CPUState {Running,Halt,Interrupt,Stopped,Error,WaitForInterrupt};
}
