/*
* .org
* .word
 .equ
* .end
* .ascii
* .byte
* .block

*+ IM     o_imm     12h
*  Ir     o_ind_reg @R1
*  r      o_reg     R1
*  Irr    o_ind_reg @RR1
*  RR     o_reg     RR1
*  cond   o_phrase
*+ IRR    o_ind_mem @INTMEM_12
*+ IR     o_ind_mem @INTMEM_12
*+ DA/RA  o_near    loc_1234
*+ R      o_mem     INTMEM_12
*+ X      o_displ   INTMEM_12(R1)

/*
 *  Interactive disassembler (IDA).
 *  Zilog Z8 module
 *
 */

#ifndef _Z8_HPP
#define _Z8_HPP

#include "../idaidp.hpp"
#include "ins.hpp"

//------------------------------------------------------------------------
// customization of cmd structure:

#define o_ind_mem   o_idpspec0      // @intmem
#define o_ind_reg   o_idpspec1      // @Rx

extern ea_t intmem;

//------------------------------------------------------------------------

enum z8_registers
{
  rR0,  rR1,  rR2,   rR3,   rR4,   rR5,   rR6,   rR7,
  rR8,  rR9,  rR10,  rR11,  rR12,  rR13,  rR14,  rR15,
  rRR0, rRR1, rRR2,  rRR3,  rRR4,  rRR5,  rRR6,  rRR7,
  rRR8, rRR9, rRR10, rRR11, rRR12, rRR13, rRR14, rRR15,
  rVcs, rVds
};

enum z8_phrases
{
  fF, fLT, fLE, fULE, fOV, fMI, fZ, fC,
  fTrue, fGE, fGT, fUGT, fNOV, fPL, fNZ, fNC
};

typedef struct
{
  uchar addr;
  char *name;
  char *cmt;
} predefined_t;

extern predefined_t iregs[];

//------------------------------------------------------------------------

void header( void );
void footer( void );

void segstart( ea_t ea );
void segend( ea_t ea );

int  ana( void );
int  emu( void );
void out( void );
bool outop( op_t &op );

void z8_data( ea_t ea );

#endif
