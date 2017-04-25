#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#define BYTE_SIZE 8
#define NO_PARAM 0
#define HAS_SS 1
#define HAS_DD 1<<1
#define PC 7

typedef unsigned char byte;
typedef short word;
typedef int adr;
byte mem[64*1024];
word reg[8] = {};  //регистор

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
struct SSDD get_mr(word param, word cur);


struct Command
{
    word mask;
    word opcode;
    char * name;
    void (*do_action)(); //параметры
    word param;  // ss, dd, xx, n
};

typedef struct mr
{
    word val;
    adr a;
} mr;

typedef struct SSDD
{
    mr ss;
    mr dd;
} SSDD;

const struct Command command_list[] = {
    {0xFFFF,  0,       "HALT", do_halt, NO_PARAM},
    {0170000, 0010000, "MOV", do_mov, HAS_SS | HAS_DD},
    {0170000, 0060000, "ADD", do_add, HAS_SS | HAS_DD},
    {0, 0, "unknown", do_unknown, NO_PARAM}
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
    //printf("HAS_SS  =  %d;HAS_DD   =   %d;HAS_SS | HAS_DD   =    %d\n", HAS_SS, HAS_DD, HAS_SS | HAS_DD);
    run_programme(0x0200, 0x000c);
    return 0;
}

void run_programme(adr start, word n)
{
	word i = 0;
    adr a = start;
    word cur = 0;
    reg[PC] = start; 
    while(1)
    {
		cur = w_read(a);																
        printf("%06x : %06x ", a, cur);
        a += 2;
        struct SSDD res = {};
        struct Command cmd;

        for(i = 0; ; i++)
        {
            cmd = command_list[i];
            if((cur & cmd.mask) == cmd.opcode)
            {
                printf("%s ", cmd.name);
                if(cmd.param)
                {
                    res = get_mr(cmd.param, cur);
                }
                cmd.do_action(res);
                break;
            }   
        }
        printf("\n");     
    }
}

struct SSDD get_mr(word param, word cur)
{
    adr adress = 0;
    int n = 0;
    int mode = 0;
    struct SSDD res;
    if(param & HAS_SS)
    {
        n = (cur >> 6) & 7;
        //printf("  n   =   %d ", n);
        //printf("\n param    %d\n", param);
        mode = (cur >> 9) & 7;
        //printf("  mode   =   %d ", mode);
        switch(mode)
        {
            case 0  :     //R1                  //подумать над проблемой реализации
                res.ss.a = n;
                res.ss.val = reg[n];
                printf("R%d ", n);
                break;
            case 1:  //(R1)
                res.ss.a = reg[n];
                res.ss.val = w_read(res.ss.a);
                printf("(R%d) \n", n);
                break;
            case 2:
            {
                if(n == PC)
                {
                    reg[PC] += 2;
                    adress = reg[PC];
                    res.ss.a = n;
                    res.ss.val = mem[adress];
                    reg[PC] += 2;
                    printf("#%d ", res.ss.val);
                }
                else
                {
                    res.ss.a = reg[n];
                    res.ss.val = w_read(res.ss.a);
                    printf("(R%d)+ \n", n);
                }
                break;
            }
            
        }
    }
    //printf("mode  :   %d\n", mode);
    if(param & HAS_DD)
    {
        n = cur & 7;
        mode = (cur >> 3) & 7;
        switch(mode)
        {
            case 0  :     //R1                  //подумать над проблемой реализации
                res.dd.a = n;
                res.dd.val = reg[n];
                printf("R%d ", n);
                break;
            case 1:  //(R1)
                res.dd.a = reg[n];
                res.dd.val = w_read(res.dd.a);
                printf("(R%d) \n", n);
                break;
            case 2:
                res.dd.a = reg[n];
                res.dd.val = w_read(res.dd.a);
                printf("(R%d)+ \n", n);
                break;
            case 3:
                res.dd.a = reg[n];
                res.dd.val = w_read(res.dd.a);
                printf("@(R%d)+ \n", n);
                break;
            case 4:
                res.dd.a = reg[n];
                res.dd.val = w_read(res.dd.a);
                printf("-(R%d) \n", n);
                break;
            case 5:
                res.dd.a = reg[n];
                res.dd.val = w_read(res.dd.a);
                printf("@-(R%d) \n", n);
                break;
        }
    }
    
    return res;
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
    exit(0);
}

void do_mov()
{
    //dd = ss;
    //w_write(dd.a, ss.val);
}

void do_add()
{
    //w_write(dd.a, dd.val + ss.val);
}

void do_unknown()
{
    ;   
}