/*
 * 	Rockwell C39 processor module for IDA Pro.
 *	Copyright (c) 2000-2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#ifndef _C39_HPP
#define _C39_HPP

#include <ida.hpp>
#include <idp.hpp>

#if IDP_INTERFACE_VERSION > 37
#include "../idaidp.hpp"
#define near
#define far
#else
#include <bytes.hpp>
#include <name.hpp>
#include <offset.hpp>
#include <segment.hpp>
#include <ua.hpp>
#include <auto.hpp>
#include <queue.hpp>
#include <lines.hpp>
#include <loader.hpp>
typedef unsigned long ea_t;
typedef int	bool;
#define false (0)
#define true (1)
#endif

#include "ins.hpp"

// ============================================================
// варианты битновых полей для specflags1 (specflag2 - не исп.)
//-----------------------------------------------
// дополнительные биты к типу ячейки
#define URR_IND         (0x01)  // косвенно, через регистр

//------------------------------------------------------------------------
// список регистров процессора
enum C39_registers { rNULLReg,
        rA, rX,rY,
        rVcs, rVds};

#if IDP_INTERFACE_VERSION > 37
extern char deviceparams[];
extern char device[];
#endif

//------------------------------------------------------------------------
void    idaapi C39_header(void);
void    idaapi C39_footer(void);

void    idaapi C39_segstart(ea_t ea);

int     idaapi C39_ana(void);
int     idaapi C39_emu(void);
void    idaapi C39_out(void);
bool    idaapi C39_outop(op_t &op);

void    idaapi C39_data(ea_t ea);

#endif
