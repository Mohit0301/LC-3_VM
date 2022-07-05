#include <stdio.h>
#include <stdint.h>
#include "hardware.h"
#include "memory_access.h"

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

void mem_write(const uint16_t address, const uint16_t data)
{
    memory[address] = data;
}