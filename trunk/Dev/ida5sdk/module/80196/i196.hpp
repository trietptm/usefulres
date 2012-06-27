/*
 *  Interactive disassembler (IDA).
 *  Intel 80196 module
 *
 */

#ifndef _I196_HPP
#define _I196_HPP

#include "../idaidp.hpp"
#include "ins.hpp"
#include <srarea.hpp>

//------------------------------------------------------------------------
// customization of cmd structure:

#define o_indirect      o_idpspec0      // [addr]
#define o_indirect_inc  o_idpspec1      // [addr]+
#define o_indexed       o_idpspec2      // addr[value]
#define o_bit           o_idpspec3

extern ulong intmem;
extern ulong sfrmem;

extern int extended;

//------------------------------------------------------------------------

enum i196_registers { rVcs, rVds, WSR, WSR1 };

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

//void i196_data(ulong ea);

#endif
