
# LC-3 Virtual Machine

A virtual machine which simulates the fictional RISC style LC-3 architecture.


## Architecture
### Memory
The LC-3 has 65,536 addressable memory locations. Every memory location stores a 16-bit value.
Therefore, LC-3 has a total of 128Kb memory.


### Registers
LC-3 has 10 16-bit registers.
8 of these are general purpose registers(R0-R7), 1 PC(program counter) and
1 condition flags(COND) register.

### Instruction Set
LC-3 has 16 instructions. Each instruction is 16 bits long, with the left 4 bits storing the opcode.
The rest of the bits store the parameters.

Instructions:

    BR = 0, /* branch */
    ADD,    /* add  */
    LD,     /* load */
    ST,     /* store */
    JSR,    /* jump register */
    AND,    /* bitwise and */
    LDR,    /* load register */
    STR,    /* store register */
    RTI,    /* unused */
    NOT,    /* bitwise not */
    LDI,    /* load indirect */
    STI,    /* store indirect */
    JMP,    /* jump */
    RES,    /* reserved (unused) */
    LEA,    /* load effective address */
    TRAP    /* execute trap */

Condition Flags:
    
    POS = 1 << 0, /* Positive */
    ZRO = 1 << 1, /* Zero */
    NEG = 1 << 2, /* Negative */

### Memory Mapped Registers
The LC-3 has 2 memory mapped registers, they are
KBSR(keyboard status register) and KBDR(keyboard data register).
The KBSR indicates whether a key has been pressed, and the KBDR 
identifies which key was pressed.

## Memory Layout
    0x0000 - 0x00FF - Trap Vector Table
    0x0100 - 0x01FF - Interrupt Vector Table
    0x0200 - 0x2FFF - Operating system and Supervisor Stack
    0x3000 - 0xFDFF - Available for user programs
    0xFE00 - 0xFFFF - Device register addresses

## TODO
Add a makefile and sample assembled LC-3 programs.
Add build and usage instructions.
Add documentation links. 
