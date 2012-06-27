/*
 * 	Panasonic MN102 (PanaXSeries) processor module for IDA Pro.
 *	Copyright (c) 2000-2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#ifndef _PAN_HPP
#define _PAN_HPP

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

//-----------------------------------------------
// Вспомогательные биты 
#define URB_ADDR        0x1     // Непоср. аргумент - адрес

//------------------------------------------------------------------------
// список регистров процессора
enum mn102_registers { rNULLReg,
        rD0, rD1, rD2, rD3,
        rA0, rA1, rA2, rA3,
        rMDR,rPSW, rPC,
        rVcs, rVds};

#if IDP_INTERFACE_VERSION > 37
extern char deviceparams[];
extern char device[];
#endif

//------------------------------------------------------------------------
void    idaapi mn102_header(void);
void    idaapi mn102_footer(void);

void    idaapi mn102_segstart(ea_t ea);

int     idaapi mn102_ana(void);
int     idaapi mn102_emu(void);
void    idaapi mn102_out(void);
bool	idaapi mn102_outop(op_t &op);

void    idaapi mn102_data(ea_t ea);

#endif
