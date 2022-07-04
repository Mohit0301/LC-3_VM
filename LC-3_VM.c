#include <stdint.h> // uint16_t
#include <stdio.h>  // FILE
#include <signal.h> // SIGINT
/* windows only */
#include <Windows.h>
#include <conio.h>  // _kbhit
#include <stdbool.h>

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
    R_R7, /*General Purpose registers.*/
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

enum
{
    TRAP_GETC = 0x20,  /* get character from keyboard, not echoed onto the terminal */
    TRAP_OUT = 0x21,   /* output a character */
    TRAP_PUTS = 0x22,  /* output a word string */
    TRAP_IN = 0x23,    /* get character from keyboard, echoed onto the terminal */
    TRAP_PUTSP = 0x24, /* output a byte string */
    TRAP_HALT = 0x25   /* halt the program */
};

enum
{
    MR_KBSR = 0xFE00, /*Keyboard status register*/
    MR_KBDR = 0xFE02 /*Keyboard data register*/
};

void mem_write(const uint16_t address, const uint16_t data)
{
    memory[address] = data;
}

uint16_t mem_read(const uint16_t address)
{
    //checking the status of keyboard
    if(address == MR_KBSR)
    {
        if(check_key())
        {
            //setting status to active
            memory[MR_KBSR] = (1 << 15);
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0;
        }
    }
    return memory[address];
}

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

void not(const uint16_t instruction)
{
    uint16_t destination_register = (instruction >> 9) & (0x7);
    uint16_t source_register = (instruction >> 6) & (0x7);

    registers[destination_register] = !(registers[source_register]);

    update_flags(destination_register);
}

void branch(const uint16_t instruction)
{
    //find out which kind of branch. P, N, or Z
    uint16_t flag = (instruction >> 9) & (0x7);
    if(flag & registers[R_COND])
    {
        uint16_t pc_offset9 = sign_extend((instruction) & (0x1FF), 9);
        registers[R_PC] += pc_offset9;
    }
}

void jump(const uint16_t instruction)
{
    /*Also handles RET.*/
    uint16_t base_register = (instruction >> 6) & (0x7);
    registers[R_PC] = registers[base_register];
}

void jump_to_subroutine(const uint16_t instruction)
{
    //In LC-3, the return address is stored in the R7 register.
    registers[R_R7] = registers[R_PC];

    //Look at JSR instruction specification.
    if((instruction >> 11) & (0x1))
    {
        uint16_t pc_offset11 = sign_extend((instruction) & (0x7FF), 11);
        registers[R_PC] += pc_offset11;
    }
    else
    {
        uint16_t base_register = (instruction >> 6) & (0x7);
        registers[R_PC] = registers[base_register];
    }
}

void load(const uint16_t instruction)
{
    uint16_t base_register = (instruction >> 9) & (0x7);
    uint16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);

    registers[base_register] = mem_read(registers[R_PC] + pc_offset9);
    
    update_flags(base_register);
}

void load_register(const uint16_t instruction)
{
    uint16_t destination_register = (instruction >> 9) & (0x7);
    uint16_t base_register = (instruction >> 6) & (0x7);

    uint16_t br_offset6 = sign_extend(instruction & 0x3F, 6);

    registers[destination_register] = mem_read(registers[base_register] + br_offset6);

    update_flags(destination_register);
}

void load_effective(const uint16_t instruction)
{
    uint16_t destination_register = (instruction >> 9) & (0x7);
    uint16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);

    registers[destination_register] = registers[R_PC] + pc_offset9;

    update_flags(destination_register);
}

void store(const uint16_t instruction)
{
    uint16_t source_register = (instruction >> 9) & (0x7);
    uint16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);

    mem_write(registers[R_PC] + pc_offset9, registers[source_register]);
}

void store_indirect(const uint16_t instruction)
{
    uint16_t source_register = (instruction >> 9) & (0x7);
    uint16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);

    mem_write(mem_read(registers[R_PC] + pc_offset9), registers[source_register]);
}

void store_register(const uint16_t instruction)
{
    uint16_t source_register = (instruction >> 9) & (0x7);
    uint16_t base_register = (instruction >> 6) & (0x7);

    uint16_t br_offset6 = sign_extend(instruction & 0X3F, 6);

    mem_write(registers[base_register] + br_offset6, registers[source_register]);
}

void trap_puts()
{
    //address stores the address of the first character of the string.
    uint16_t* address = memory + registers[R_R0];
    //end of string is marked by 0xFFFF. Since characters are a byte in c, typecast is necessary.
    while(*address)
    {
        putc((char)*address, stdout);
        ++address;
    }
    fflush(stdout);
}

void trap_getc()
{
    //read character from keyboard into register R0.
    registers[R_R0] = (uint16_t) getchar();
    update_flags(R_R0);
}

void trap_out()
{
    putc((char)registers[R_R0], stdout);
    fflush(stdout);
}

void trap_in()
{
    printf("Enter a character: ");
    char c = getchar();
    putc(c, stdout);
    fflush(stdout);
    registers[R_R0] = (uint16_t) c;
    update_flags(R_R0);

}

void trap_putsp()
{
    /* one char per byte (two bytes per word)
       here we need to swap back to
       big endian format */
    uint16_t* c = memory + registers[R_R0];
    while (*c)
    {
        char char1 = (*c) & 0xFF;
        putc(char1, stdout);
        char char2 = (*c) >> 8;
        if (char2) putc(char2, stdout);
        ++c;
    }
    fflush(stdout);
}

void trap_halt(bool* FETCH)
{
    puts("HALTING.");
    fflush(stdout);
    *FETCH = false;
}

uint16_t swap16(uint16_t x)
{
    return ((x << 8) | (x >> 8));
}

void read_image_file(FILE* file)
{
    /*The first 16bits in a LC-3 program specify the origin. Origin is the location where this program is loaded into memory.*/
    uint16_t origin;
    fread(&origin, sizeof(uint16_t), 1, file);
    origin = swap16(origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* program = memory + origin;
    //since we already know the maxsize of the program, we only need to call fread once.
    uint16_t read = fread(program, sizeof(uint16_t), max_read, file);

    //converting the Big-endian program to little-endian formater
    while((read--) > 0)
    {
        *program = swap16(*program);
        ++program;
    }
}

int read_image(const char* image_path)
{
    FILE* file = fopen(image_path, "rb");
    if (!file) { return 0; };
    read_image_file(file);
    fclose(file);
    return 1;
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
                not(instruction);
                break;
            case OP_BR:
                branch(instruction);
                break;
            case OP_JMP:
                jump(instruction);
                break;
            case OP_JSR:
                jump_to_subroutine(instruction);
                break;
            case OP_LD:
                load(instruction);
                break;
            case OP_LDI:
                ldi(instruction);
                break;
            case OP_LDR:
                load_register(instruction);
                break;
            case OP_LEA:
                load_effective(instruction);
                break;
            case OP_ST:
                store(instruction);
                break;
            case OP_STI:
                store_indirect(instruction);
                break;
            case OP_STR:
                store_register(instruction);
                break;
            case OP_TRAP:
                {
                    //On LC-3, the return address is stored on register R7.
                    registers[R_R7] = registers[R_PC];
                    switch (instruction & 0xFF)
                    {
                        case TRAP_GETC:
                            trap_getc();
                            break;
                        case TRAP_OUT:
                            trap_out();
                            break;
                        case TRAP_PUTS:
                            trap_puts();
                            break;
                        case TRAP_IN:
                            trap_in();
                            break;
                        case TRAP_PUTSP:
                            trap_putsp();
                            break;
                        case TRAP_HALT:
                            trap_halt(&FETCH);
                            break;
                    }
                }
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