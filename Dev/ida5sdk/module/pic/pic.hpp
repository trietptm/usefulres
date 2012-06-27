/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-99 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com
 *
 *
 */

#ifndef _PIC_HPP
#define _PIC_HPP

#include "../idaidp.hpp"
#include <diskio.hpp>
#include "ins.hpp"

//------------------------------------------------------------------
enum regnum_t
{
  W, F,
  ACCESS,        // register for PIC18Cxx
  BANKED,        // register for PIC18Cxx
  FAST,          // register for PIC18Cxx
  FSR0,          // register for PIC18Cxx
  FSR1,          // register for PIC18Cxx
  FSR2,          // register for PIC18Cxx
  BANK,
  rVcs, rVds,    // virtual registers for code and data segments
  PCLATH,
  PCLATU         // register for PIC18Cxx
};

#define PIC16_FSR2L 0xFD9
#define PIC16_PLUSW2 0xFDB
#define PIC16_INDF2 0xFDF
#define PIC16_BANK 0xFE0
#define PIC16_FSR1L 0xFE1
#define PIC16_POSTINC1 0xFE6
#define PIC16_PCL 0xFF9
#define PIC16_PCLATH 0xFFA

//------------------------------------------------------------------
// specific device name

extern char device[MAXSTR];

//------------------------------------------------------------------
// processor types

typedef uchar proctype_t;

const proctype_t PIC12  = 0;
const proctype_t PIC14  = 1;
const proctype_t PIC16  = 2;

extern proctype_t ptype;        // contains processor type

extern ea_t dataseg;
//------------------------------------------------------------------
extern netnode helper;

#define IDP_MACRO    0x0001     // use instruction macros

extern ushort idpflags;

inline bool macro(void)         { return (idpflags & IDP_MACRO) != 0; }

inline bool is_bit_insn(void)
  { return cmd.itype >= PIC_bcf && cmd.itype <= PIC_btfss; }
bool conditional_insn(void);    // may instruction be skipped?

extern bool is_pcl(void);
extern bool is_bank(void);
extern bool is_pclath(void);

const char *find_sym(ea_t address);
const ioport_bit_t *find_bits(ea_t address);
const char *find_bit(ea_t address, int bit);
ea_t calc_code_mem(ea_t ea);
ea_t calc_data_mem(ea_t ea);
ea_t map_port(ea_t from);
//------------------------------------------------------------------
void interr(const char *module);

void header(void);
void footer(void);

void segstart(ea_t ea);
void segend(ea_t ea);
void assumes(ea_t ea);         // function to produce assume directives

void out(void);
int  outspec(ea_t ea,uchar segtype);

int  ana(void);
int  emu(void);
bool outop(op_t &op);
void data(ea_t ea);

int  is_align_insn(ea_t ea);
bool create_func_frame(func_t *pfn);
void out_rename(ea_t ea,int storage);
int  out_storage_class(ea_t ea);
bool is_sp_based(const op_t &x);

int PIC_get_frame_retsize(func_t *pfn);
int is_jump_func(const func_t *pfn, ea_t *jump_target);
int is_sane_insn(int nocrefs);
int may_be_func(void);           // can a function start here?

#endif // _PIC_HPP
