#ifndef _HARDWARE_
#define _HARDWARE_

#include <stdint.h>

#define MEMORY_MAX (1 << 16)

//Main Memory
uint16_t memory[MEMORY_MAX]; /* 65536 16 bit locations */

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

enum
{
    MR_KBSR = 0xFE00, /*Keyboard status register*/
    MR_KBDR = 0xFE02 /*Keyboard data register*/
};

uint16_t registers[R_COUNT];

#endif