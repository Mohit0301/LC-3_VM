#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "headers/opcodes.h"
#include "headers/hardware.h"
#include "headers/memory_access.h"

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

    registers[destination_register] = ~(registers[source_register]);

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