#include <stdio.h>
#include <stdint.h>
#include "hardware.h"
#include "load_program.h"

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
    size_t read = fread(program, sizeof(uint16_t), max_read, file);

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
