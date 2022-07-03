#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include <Windows.h>
#include <conio.h>  // _kbhit

HANDLE hStdin = INVALID_HANDLE_VALUE;

/*LC-3 has 65,536 (2 ^16) addressable locations, each of size 16 bits. => Total memory = 65536 * 16 = 1048576 bits = 131072 bytes = 128 KB.*/

#define MEMORY_MAX (1 << 16)

//RAM
uint16_t memory[MEMORY_MAX]; /* 65536 locations */

//LC-3 has 10 16bit registers.

enum
{
    R_R0 = 0, 
    R_R1, 
    R_R2, 
    R_R3, 
    R_R4, 
    R_R5, 
    R_R6, 
    R_7, /*General Purpose registers.*/
    R_PC, 
    R_COND,
    R_COUNT /*Total number of registers.*/
}Registers;

uint16_t registers[R_COUNT];

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

void load_arguments(const int argc, const char* argv[])
{
    if(argc <  2)
    {
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }
    else
    {
        for(int i = 1; i < argc; ++i)
        {
            if (!read_image(argv[i]))
            {
                printf("failed to load image: %s\n", argv[i]);
                exit(1);
            }
        }
    }
}

//negative numbers are represented in 2s complement form.
//Used when opreand is negative.
uint16_t sign_extend(uint16_t operand, uint16_t bit_count)
{
    //check if operand is negative.
    if((operand >> (bit_count - 1)) & 1)
    {
        operand |= (0xFFFF << bit_count);
    }
    return operand;
}

void update_flags(uint16_t r_no)
{
    if(registers[r_no] == 0)
    {
        registers[R_COND] = FL_ZRO;
    }
    //content of the register is negative
    else if(registers[r_no] >> 15)
    {
        registers[R_COND] = FL_NEG;
    }
    else
    {
        registers[R_COND] = FL_POS;
    }
}

void add(const uint16_t instruction)
{
    uint16_t destination_register = (instruction >> 9) & (0x7);
    uint16_t source_register1 = (instruction >> 6) & (0x7);

    //checking if we are in immediate mode
    uint16_t imm_flag = (instruction >> 5) & (0x1);
    if(imm_flag)
    {
        uint16_t operand2 = sign_extend(instruction & 0x1F, 5);
        registers[destination_register] = registers[source_register1] + operand2;
    }
    else
    {
        //source register 2
        uint16_t source_register2 = (instruction) & (0x7);
        registers[destination_register] = registers[source_register1] + registers[source_register2];
    }

    update_flags(destination_register);
}

void ldi(const uint16_t instruction)
{
    uint16_t destination_register = (instruction >> 9) & (0x7);

    uint16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);

    registers[destination_register] = mem_read(mem_read(registers[R_PC] + pc_offset9));

    update_flags(destination_register);
}

void and(const uint16_t instruction)
{
    uint16_t destination_register = (instruction >> 9) & (0x7);
    uint16_t source_register1 = (instruction >> 6) & (0x7);

    //check for immediate mode
    uint16_t imm_flag = (instruction >> 5) & (0x1);
    if(imm_flag)
    {
        uint16_t imm5 = sign_extend(instruction & 0X1F, 5);
        registers[destination_register] = registers[source_register1] & imm5;
    }
    else
    {
        uint16_t source_register2 = instruction & (0x7);
        registers[destination_register] = registers[source_register1] & registers[source_register2];
    }
    update_flags(destination_register);
}


int main(const int argc, const char* argv[])
{
    //Load arguments i.e arguments given in the argument vector
    load_arguments(argc, argv);
    //setup

    registers[R_COND] = FL_ZRO; /*Condition flag set to zero by default.*/

    enum { PC_START = 0x3000 }; /*Default starting location of the program is 0x3000. */
    registers[R_PC] = PC_START;

    bool FETCH = true;

    while(FETCH)
    {
        //Step1: Read the opcode and increment the PC.
        uint16_t instruction = mem_read(registers[R_PC]++);
        uint16_t op = instruction >> 12;

        switch (op)
        {
            case OP_ADD:
                add(instruction);
                break;
            case OP_AND:
                and(instruction);
                break;
            case OP_NOT:
                //{NOT}
                break;
            case OP_BR:
                //{BR}
                break;
            case OP_JMP:
                //{JMP}
                break;
            case OP_JSR:
                //{JSR}
                break;
            case OP_LD:
                //{LD}
                break;
            case OP_LDI:
                ldi(instruction);
                break;
            case OP_LDR:
                //{LDR}
                break;
            case OP_LEA:
                //{LEA}
                break;
            case OP_ST:
                //{ST}
                break;
            case OP_STI:
                //{STI}
                break;
            case OP_STR:
                //{STR}
                break;
            case OP_TRAP:
                //{TRAP}
                break;
            case OP_RES:
            case OP_RTI:
            default:
                //{BAD OPCODE}
                printf("\nINVALID INTRUCTION. TERMINATING.\n");
                exit(3);
                break;
        }
    }

}