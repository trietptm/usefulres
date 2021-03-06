/*
 *      Interactive disassembler (IDA).
 *      Version 3.05
 *      Copyright (c) 1990-95 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              FIDO:   2:5020/209
 *                              E-mail: ig@estar.msk.su
 *
 */

#ifndef _M65_HPP
#define _M65_HPP

#include "../idaidp.hpp"
#include "ins.hpp"

extern int is_cmos;     // is CMOS (otherwise, NMOS)

// Is indirect memory reference?

#define indirect        auxpref

#define UAS_SECT        0x0002          // Segments are named .SECTION
#define UAS_NOSEG       0x0004          // No 'segment' directives
#define UAS_SELSG       0x0010          // Segment should be selected by its name
#define UAS_CDSEG       0x0080          // Only DSEG,CSEG,XSEG
#define UAS_NOENS       0x0200          // don't specify start addr in the .end directive
//------------------------------------------------------------------------
enum M65_registers { rA,rX,rY,rVcs,rVds,riX,riY,zX,zY };

//------------------------------------------------------------------------
void    header(void);
void    footer(void);

void    segstart(ea_t ea);

int     ana(void);
int     emu(void);
void    out(void);
bool    outop(op_t &op);
void    assumes(ea_t ea);

#endif
