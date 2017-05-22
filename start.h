#ifndef START_H
#define START_H

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

#endif