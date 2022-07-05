#ifndef _LOAD_PROGRAM_
#define _LOAD_PROGRAM_

#include <stdio.h>
#include <stdint.h>

uint16_t swap16(uint16_t x);
void read_image_file(FILE* file);
int read_image(const char* image_path);

#endif