#include "start.h"

byte mem[64*1024];

byte b_read(adr a)
{
    return mem[a];
}

void b_write(adr a, byte val)
{
    mem[a] = val;
}

word w_read(adr a)
{
    word val = 0;
    val = val | mem[a + 1];
    val = val << BYTE_SIZE;
    val = val | mem[a];
    return val;
}

void w_write(adr a, word val)
{
    mem[a] = val & 0xFF;
    mem[a + 1] = (val >> BYTE_SIZE) & 0xFF;
}
