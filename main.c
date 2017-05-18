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
#define OSTAT 0xFF74
#define ODATA 0xFF76
#define ODATA_1 0xFFFFFF76
#define MEM 1
#define REG 0
#define BYTE 1

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
void do_tst();
void do_bpl();
void do_rts();
void do_jsr();
void do_unknown();
void reg_dump();
void change_Z(word res);
void change_N(word res, word b);
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
    word b;
    word space;
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
    {0xF000, 0010000, "MOV", do_mov, HAS_SS | HAS_DD | HAS_B},
    {0xF000, 0060000, "ADD", do_add, HAS_SS | HAS_DD},
    {0xFE00, 0077000, "SOB", do_sob, HAS_R | HAS_NN},
    {0xFF00,  0005000, "CLR", do_clr, HAS_DD},
    {0xF000, 0110000, "MOVb", do_mov, HAS_SS | HAS_DD | HAS_B},
    {0xFF00,  0001400, "BEQ", do_beq, HAS_XX},
    {0xFF00,  0000400, "BR", do_br, HAS_XX},
    {0xFFC0,  0105700, "TSTb", do_tst, HAS_B | HAS_DD},
    {0xFFC0,  0005700, "TST", do_tst, HAS_B | HAS_DD},
    {0xFF00,  0100000, "BPL", do_bpl, HAS_XX},
    {0xFE00,  0004000, "JSR", do_jsr, HAS_DD | HAS_R},
    {0xFFF8,  0000200, "RTS", do_rts, HAS_R},
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
    mem[OSTAT] = mem[OSTAT] | 0x80;
    reg[SP] = 0x200;
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
                if((cmd.param & 0x1F) == HAS_R)
                    res.r = get_r(cur);
                else if(cmd.param & HAS_R)
                    res.r = get_r(cur >> 6);
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
    return cur & 7;
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
    res.b = get_b(cur >> 15);
    n = cur & 7;
    mode = (cur >> 3) & 7;
    word nn = 0;
    switch(mode)
    {
        case 0  :     //R1                  //подумать над проблемой реализации
            res.a = n;
            res.val = reg[n];
            printf("R%d ", n);
            res.space = REG;
            break;
        case 1:  //(R1) 
            res.a = reg[n];
            if(res.b == BYTE)
                res.val = b_read(res.a);                
            else
                res.val = w_read(res.a);
            printf("(R%d) \n", n);
            res.space = MEM;
            break;
        case 2:
            res.a = reg[n];
            if(res.b == BYTE)
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
            {
                printf("#%o ", res.val);
            }
            else
                printf("(R%d)+ ", n);
            res.a = n;
            res.space = MEM;
            break;
        case 3:
            res.a = reg[n];
            reg[n] += 2;
            res.a = w_read(res.a)+ 0200000;
            //printf("~res.a = %o~\n", res.a);
            if(res.b == BYTE)
            {
                res.val = b_read(res.a);  
                if(res.val >> 7 == 1)
                    res.val = res.val | 0xFF00;
                else
                    res.val = res.val | 0;                     
            }
            else
            {
                res.val = w_read(res.a);
            }       
            if(n == PC)
                printf("@#%d ", res.val);
            else
                printf("@(R%d)+ ", n);
            res.space = MEM;
            break;
        case 4:
            if(res.b == BYTE)
            {
                reg[n] -= 1;
                if(n == PC || n == SP)
                {
                    reg[n] -= 1;
                }
                res.a = reg[n];
                res.val = b_read(res.a); 
                if(res.val >> 7 == 1)
                    res.val = res.val | 0xFF00;
                else
                    res.val = res.val | 0;                    
            }
            else
            {
                reg[n] -= 2;
                res.a = reg[n];
                res.val = w_read(res.a);
            }
            printf("-(R%d) ", n);
            res.space = MEM;
            break;
        case 5:
            res.a = w_read(reg[n]);
            if(res.b == BYTE)
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
            res.space = MEM;
            break;
        case 6:
            nn = w_read(reg[PC]);
            reg[PC] += 2;
            res.a = reg[n] + nn;
            res.val = w_read(res.a);
            printf(" %d(R%d) res.a = %o val : %o ", nn, n, res.a, res.val);
            /*nn = w_read(reg[PC]);
            reg[PC] += 2;
            res.a = reg[n];
            res.a += nn;
            res.val = w_read(res.a);
            printf("PC : %o  loop : %o", reg[PC], res.a);*/
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

void do_rts(struct PARAM res)
{
    reg[PC] = reg[res.r];
    reg[res.r] = w_read(reg[SP]); //!!!
    reg[SP] += 2;
}

void do_jsr(struct PARAM res)
{
    reg[res.r] = reg[PC];
    w_write(reg[SP] - 2, reg[res.r]);
    reg[SP] -= 2;
    reg[PC] = res.dd.a;
}

void do_br(struct PARAM res)
{
    reg[PC] = reg[PC] + 2 * res.xx;
}

void do_tst(struct PARAM res)
{
    if(((res.dd.val >> 7) & 1) == 1)
    {
        PSW[N] = 1;
    } 
    else
    {
        PSW[N] = 0;
    }
}

void do_bpl(struct PARAM res)
{
    if(PSW[N] == 0)
        do_br(res);
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

void change_Z(word res)
{
    if(res == 0)
        PSW[Z] = 1;
    else
        PSW[Z] = 0;
}

void change_N(word res, word b)
{
    if(b == BYTE)
        res = res >> 7;
    else
        res = res >> 15;
    if(res = 1)
        PSW[N] = 1;
    else
        PSW[N] = 0;
}

void do_mov(struct PARAM res)
{
    if(res.dd.a == ODATA || res.dd.a == ODATA_1)
    {
        printf("~~%c~~", res.ss.val);
    }
    if(res.dd.space == REG)
    {
        reg[res.dd.a] = res.ss.val;
        change_Z(res.ss.val);        
        change_N(res.ss.val, res.ss.b);
    }
    else if(res.dd.space == MEM)
    {
        if(res.ss.b == BYTE)
            b_write(res.dd.a, res.ss.val);
        else
            w_write(res.dd.a, res.ss.val);
        change_Z(res.ss.val);        
        change_N(res.ss.val, res.ss.b);
    }
   // printf(" reg[1] = %o ", reg[1]);
}

void do_add(struct PARAM res)
{
    reg[res.dd.a] = res.dd.val + res.ss.val;
    change_Z(reg[res.dd.a]);
    change_N(reg[res.dd.a], res.ss.b);
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