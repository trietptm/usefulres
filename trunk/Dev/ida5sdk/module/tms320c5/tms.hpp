/*
 *      Interactive disassembler (IDA).
 *      Version 3.05
 *      Copyright (c) 1990-95 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              FIDO:   2:5020/209
 *                              E-mail: ig@estar.msk.su
 *
 */

#ifndef _TMS_HPP
#define _TMS_HPP

#include "../idaidp.hpp"
#include "ins.hpp"
#include <srarea.hpp>

//------------------------------------------------------------------------
// customization of cmd structure:
#define o_bit           o_idpspec0
#define o_bitnot        o_idpspec1
#define o_cond          o_idpspec2

#define sib     specflag1
#define Cond    reg

extern int nprc;        // processor number
#define PT_TMS320C5     0
#define PT_TMS320C2     1

inline bool isC2(void) { return nprc == PT_TMS320C2; }


//------------------------------------------------------------------------
enum TMS_registers { rAcc,rP,rBMAR,rAr0,rAr1,rAr2,rAr3,rAr4,rAr5,rAr6,rAr7,rVcs,rVds,rDP };

enum TMS_bits { bit_intm,bit_ovm,bit_cnf,bit_sxm,bit_hm,bit_tc,bit_xf,bit_c };

//------------------------------------------------------------------------
typedef struct
{
  uchar addr;
  char *name;
  char *cmt;
} predefined_t;

extern predefined_t iregs[];

ea_t prevInstruction(ea_t ea);
int   find_ar(ea_t *res);
//------------------------------------------------------------------------
void    header(void);
void    footer(void);

void    segstart(ea_t ea);

int     ana(void);
int     emu(void);
void    out(void);
bool    outop(op_t &op);
void    tms_assumes(ea_t ea);

#endif
