#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#define BYTE_SIZE 8
#define NO_PARAM 0
#define HAS_SS 1
#define HAS_DD (1<<1)
#define HAS_NN (1<<3)
#define HAS_R (1<<2)
#define HAS_B (1<<4)
#define HAS_XX (1<<5)
#define PC 7
#define SP 6
#define N 1
#define Z 2
#define V 3
#define C 4

typedef unsigned char byte;
typedef short word;
typedef int adr;
byte mem[64*1024];
word reg[8] = {};  //регистор
word PSW[4] = {};  //processor status word

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
void do_sob();
void do_clr();
void do_beq();
void do_br();
void do_unknown();
void reg_dump();
void check_Z(word res);
void check_N(word res);
word get_xx(word cur);
word get_r(word cur);
word get_nn(word cur);
word get_b(word cur);
struct MR get_mr(word cur);


struct Command
{
    word mask;
    word opcode;
    char * name;
    void (*do_action)(); //параметры
    word param;  // ss, dd, xx, n
};

typedef struct MR
{
    word val;
    adr a;
} MR;

typedef struct PARAM
{
    word xx;
    word nn;
    word r;
    MR ss;
    MR dd;
} PARAM;

const struct Command command_list[] = {
    {0xFFFF,  0,       "HALT", do_halt, NO_PARAM},
    {0170000, 0010000, "MOV", do_mov, HAS_SS | HAS_DD | HAS_B},
    {0170000, 0060000, "ADD", do_add, HAS_SS | HAS_DD},
    {0177000, 0077000, "SOB", do_sob, HAS_R | HAS_NN},
    {0xFF00,  0005000, "CLR", do_clr, HAS_DD},
    {0170000, 0110000, "MOVb", do_mov, HAS_SS | HAS_DD | HAS_B},
    {0xFFC0, 001400, "BEQ", do_beq, HAS_XX},
    {0x0100, 000400, "BR", do_br, HAS_XX},
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
    run_programme(0x0200, 0x000c);
    return 0;
}

void run_programme(adr start, word n)
{
	word i = 0;
    word cur = 0;
    reg[PC] = start; 
    while(1)
    {
		cur = w_read(reg[PC]);
        if(cur < 0)	
        {												
            printf("%06o : %06o ", reg[PC], cur + 0200000);
        }
        else
            printf("%06o : %06o ", reg[PC], cur);
        reg[PC] += 2; 
        struct PARAM res = {};
        struct Command cmd;

        for(i = 0; ; i++)
        {
            cmd = command_list[i];
            if((cur & cmd.mask) == cmd.opcode)
            {
                printf("%s ", cmd.name);
                if(cmd.param & HAS_SS)
                    res.ss = get_mr(cur >> 6);
                if(cmd.param & HAS_DD)
                    res.dd = get_mr(cur);
                if(cmd.param & HAS_R)
                    res.r = get_r(cur);
                if(cmd.param & HAS_NN)
                    res.nn = get_nn(cur);
                if(cmd.param & HAS_XX)
                    res.xx = get_xx(cur);
                cmd.do_action(res);
                break;
            }   
        }
        printf("\n");    
    }
}

word get_r(word cur)
{
    return (cur>>6) & 7;
}

word get_nn(word cur)
{
    return cur & 0x3F;
}

word get_xx(word cur)
{
    cur = cur & 0xFF;
    if((cur >> 7) & 1 == 1)
    {
        cur = - (0x100 - cur);
    }
    return cur;
}

word get_b(word cur)
{
    return cur & 1;
}

struct MR get_mr(word cur)
{
    adr adress = 0;
    int n = 0;
    int mode = 0;
    MR res;
    word b = get_b(cur >> 15);
    n = cur & 7;
    mode = (cur >> 3) & 7;
    switch(mode)
    {
        case 0  :     //R1                  //подумать над проблемой реализации
            res.a = n;
            res.val = reg[n];
            printf("R%d ", n);
            break;
        case 1:  //(R1) 
            res.a = reg[n];
            if(b == 1)
                res.val = b_read(res.a);                
            else
                res.val = w_read(res.a);
            printf("(R%d) \n", n);
            break;
        case 2:
            res.a = reg[n];
            if(b == 1)
            {
                res.val = b_read(res.a);    // отрицательные числа                       
                if(res.val >> 7 == 1)
                    res.val = res.val | 0xFF00;
                else
                    res.val = res.val | 0;
                if(n == PC || n == SP)
                    reg[n] += 2;
                else
                    reg[n] += 1;
            }
            else
            {
                res.val = w_read(res.a);
                reg[n] += 2;
            }
            if(n == PC)
                printf("#%d ", res.val);
            else
                printf("(R%d)+ ", n);
            break;
        case 3:
            res.a = w_read(reg[n]);
            if(b == 1)
            {
                res.val = b_read(res.a);  
                if(res.val >> 7 == 1)
                    res.val = res.val | 0xFF00;
                else
                    res.val = res.val | 0;                     
                if(n == PC || n == SP)
                    reg[n] += 2;
                else
                    reg[n] += 1;
            }
            else
            {
                res.val = w_read(res.a);
                reg[n] += 2;
            }       
            if(n == PC)
                printf("@#%d ", res.val);
            else
                printf("@(R%d)+ ", n);
            break;
        case 4:
            res.a = reg[n];
            if(b == 1)
            {
                res.val = b_read(res.a); 
                if(res.val >> 7 == 1)
                    res.val = res.val | 0xFF00;
                else
                    res.val = res.val | 0;
                if(n == PC || n == SP)
                    reg[n] -= 2;
                else
                    reg[n] -= 1;                      
            }
            else
            {
                res.val = w_read(res.a);
                reg[n] -= 2;
            }
            printf("-(R%d) ", n);
            break;
        case 5:
            res.a = w_read(reg[n]);
            if(b == 1)
            {
                res.val = b_read(res.a);
                if(res.val >> 7 == 1)
                    res.val = res.val | 0xFF00;
                else
                    res.val = res.val | 0;                       
                if(n == PC || n == SP)
                    reg[n] -= 2;
                else
                    reg[n] -= 1;
            }
            else
            {
                res.val = w_read(res.a);
                reg[n] -= 2;
            }
            printf("@-(R%d) ", n);
            reg[n] -= 2;
            break;
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
    reg_dump();
    exit(0);
}

void do_br(struct PARAM res)
{
    reg[PC] = reg[PC] + 2 * res.xx;
}

void do_beq(struct PARAM res)
{
    if(PSW[Z] == 1)
        do_br(res);
}

void reg_dump()
{
    int i = 0;
    printf("\n");
    for(i = 0; i < 8; i ++)
    {
        printf("reg[%d] = %o\n", i, reg[i]);
    }
}

void check_Z(word res)
{
    if(res == 0)
        PSW[Z] = 1;
    else
        PSW[Z] = 0;
}

void check_N(word res)
{
    if(res < 0)
        PSW[N] = 1;
    else
        PSW[N] = 0;
}

void do_mov(struct PARAM res)
{
    reg[res.dd.a] = res.ss.val;
    check_Z(reg[res.dd.a]);
    check_N(reg[res.dd.a]);
}

void do_add(struct PARAM res)
{
    reg[res.dd.a] = res.dd.val + res.ss.val;
    check_Z(reg[res.dd.a]);
    check_N(reg[res.dd.a]);
    printf("\nAdd result: %o", reg[res.dd.a]);
}

void do_unknown()
{
    ;   
}

void do_sob(struct PARAM res)
{
    printf("R%d LABEL:%o ", res.r, reg[PC] - 2 * res.nn);
    if(--reg[res.r] != 0)
        reg[PC] = reg[PC] - 2 * res.nn;
}

void do_clr(struct PARAM res)
{
    reg[res.dd.a] = 0; 
    PSW[N] = 0;
    PSW[Z] = 1;
}