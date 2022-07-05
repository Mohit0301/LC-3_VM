#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
/* unix */
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>
#include <stdbool.h>

#include "headers/hardware.h"
#include "headers/load_program.h"
#include "headers/memory_access.h"
#include "headers/opcodes.h"

uint16_t check_key()
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
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

int main(const int argc, const char* argv[])
{
    //Load arguments i.e arguments given in the argument vector
    load_arguments(argc, argv);
    
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

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
    restore_input_buffering();
}