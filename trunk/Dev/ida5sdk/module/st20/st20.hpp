/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-2000 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com
 *
 *
 */

#ifndef _ST20_HPP
#define _ST20_HPP

#include "../idaidp.hpp"
#include <diskio.hpp>
#include "ins.hpp"

//------------------------------------------------------------------
enum regnum_t
{
  Areg,       // Evaluation stack register A
  Breg,       // Evaluation stack register B
  Creg,       // Evaluation stack register C
  Iptr,       // Instruction pointer register, pointing to the next instruction to be executed
  Status,     // Status register
  Wptr,       // Work space pointer, pointing to the stack of the currently executing process
  Tdesc,      // Task descriptor
  IOreg,      // Input and output register
  cs,
  ds,

};

//------------------------------------------------------------------
extern netnode helper;
extern int procnum;
#define PROC_C1 0
#define PROC_C4 1

inline bool isc4(void) { return procnum == PROC_C4; }

ea_t calc_mem(ea_t ea);         // map virtual to physical ea
const ioport_t *find_sym(int address);
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
void gen_stkvar_def(char *buf,const member_t *mptr,long v);
int is_sp_based(op_t &x);

int get_frame_retsize(func_t *);
int is_jump_func(const func_t *pfn, ea_t *jump_target);
int is_sane_insn(int nocrefs);
int may_be_func(void);           // can a function start here?

#endif // _ST20_HPP
