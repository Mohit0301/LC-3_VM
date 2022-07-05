#ifndef _MEMORY_ACCESS_
#define _MEMORY_ACCESS_

#include <stdint.h>

uint16_t check_key();
uint16_t mem_read(const uint16_t address);
void mem_write(const uint16_t address, const uint16_t data);

#endif