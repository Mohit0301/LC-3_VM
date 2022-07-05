#ifndef _OPCODES_
#define _OPCODES_

#include <stdint.h>

enum
{
    OP_BR = 0, /* branch */
    OP_ADD,    /* add  */
    OP_LD,     /* load */
    OP_ST,     /* store */
    OP_JSR,    /* jump register */
    OP_AND,    /* bitwise and */
    OP_LDR,    /* load register */
    OP_STR,    /* store register */
    OP_RTI,    /* unused */
    OP_NOT,    /* bitwise not */
    OP_LDI,    /* load indirect */
    OP_STI,    /* store indirect */
    OP_JMP,    /* jump */
    OP_RES,    /* reserved (unused) */
    OP_LEA,    /* load effective address */
    OP_TRAP    /* execute trap */
}Opcodes;

//LC-3 has three conditional flags, used to indicate the sign of the previous operation.
enum
{
    FL_POS = (1 << 0), /*POSITIVE*/
    FL_ZRO = (1 << 1), /*ZERO*/
    FL_NEG = (1 << 2) /*NEGATIVE*/
}ConditionFlags;

enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
}Traps;

uint16_t sign_extend(uint16_t operand, uint16_t bit_count);
void update_flags(uint16_t r_no);
void add(const uint16_t instruction);
void ldi(const uint16_t instruction);
void and(const uint16_t instruction);
void not(const uint16_t instruction);
void branch(const uint16_t instruction);
void jump(const uint16_t instruction);
void jump_to_subroutine(const uint16_t instruction);
void load(const uint16_t instruction);
void load_register(const uint16_t instruction);
void load_effective(const uint16_t instruction);
void store(const uint16_t instruction);
void store_indirect(const uint16_t instruction);
void store_register(const uint16_t instruction);
void trap_puts();
void trap_getc();
void trap_out();
void trap_in();
void trap_putsp();
void trap_halt(bool* FETCH);

#endif