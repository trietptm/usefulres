/*
 * 	National Semiconductor Corporation CR16 processor module for IDA Pro.
 *	Copyright (c) 2002-2006 Konstantin Norvatoff, <konnor@bk.ru>
 *	Freeware.
 */

#ifndef _CR16_HPP
#define _CR16_HPP

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
// ��ਠ��� ��⭮��� ����� ��� specflags1 (specflag2 - �� ��.)
//-----------------------------------------------
// �������⥫�� ���� � ⨯� �祩��
#define URR_PAIR        (0x01)  // ��ᢥ���, �१ ॣ����

//------------------------------------------------------------------------
// ᯨ᮪ ॣ���஢ ������
enum CR16_registers { rNULLReg,
        rR0,rR1,rR2,rR3,rR4,rR5,rR6,rR7,
        rR8,rR9,rR10,rR11,rR12,rR13,rRA,rSP,
        // ����ॣ�����
        rPC,rISP,rINTBASE,rPSR,rCFG,rDSR,rDCR,rCARL,rCARH,
        rINTBASEL,rINTBASEH,
        rVcs, rVds};


#if IDP_INTERFACE_VERSION > 37
extern char deviceparams[];
extern char device[];
#endif

//------------------------------------------------------------------------
void    idaapi CR16_header(void);
void    idaapi CR16_footer(void);

void    idaapi CR16_segstart(ea_t ea);

int     idaapi CR16_ana(void);
int     idaapi CR16_emu(void);
void    idaapi CR16_out(void);
bool    idaapi CR16_outop(op_t &op);

void    idaapi CR16_data(ea_t ea);

#endif
