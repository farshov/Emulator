#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#define BYTE_SIZE 8
typedef unsigned char byte;
typedef short word;
typedef int adr;
byte mem[64*1024];
word reg[8] = {};

byte b_read (adr a);
void b_write (adr a, byte val);
word w_read (adr a);
void w_write (adr a, word val);
void load_file(FILE * input);
void mem_dump(adr start, word n);
void run_programme(adr start, word n);
void do_halt();
void do_mov();
void do_add();
void do_unknown();

struct Command
{
    word mask;
    word opcode;
    char * name;
    void (*do_action)(); //параметры
};

struct Command command_list[] = {
    {0xFFFF, 0, "HALT", do_halt},
    {0170000, 0010000, "MOV", do_mov},
    {0170000, 0060000, "ADD", do_add},
    {0, 0, "unknown", do_unknown}
};

int main(int argc, char * argv[])
{
	FILE * input = fopen(argv[1], "r");
    if(errno)
    {
        perror("INVALID FILE");
        exit(0);
    }
    load_file(input);
    fclose(input);
    run_programme(0x0200, 0x000c);
    return 0;
}

void run_programme(adr start, word n)
{
	word i = 0;
    adr a = start;
    word cur = 0;

    while(1)
    {
		cur = w_read(a);																
        printf("%06x : %06x ", a, cur);
        a += 2;


        for(i = 0; ; i++)
        {
            struct Command cmd = command_list[i];
            if((cur & cmd.mask) == cmd.opcode)
            {
                printf("%s ", cmd.name);
                cmd.do_action();
                break;
            }       
        }
        printf("\n");
    }
}

void load_file(FILE * input)
{
    unsigned int n = 0, i = 0;
    adr cur = 0;
    byte val = 0;
    while(fscanf(input, "%x %x", &cur, &n) == 2)
    {
        for(i = 0; i < n; i++)
        {
            fscanf(input, "%hhx", &val);
            b_write(cur, val);
            cur += 1;
        }
    }
}

void mem_dump(adr start, word n)
{
    word i = 0;
    adr a = start;
    for(i = 0; i < n; i+=2)
    {
        printf("%06o : ", a);
        printf("%06o\n", w_read(a));
        a += 2;
    }
}

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

void do_halt()
{
    printf("THE END");
    exit(0);
}

void do_mov()
{
    printf("mov mov");
}

void do_add()
{
    printf("add add");
}

void do_unknown()
{
    printf("unknown unknown");
}