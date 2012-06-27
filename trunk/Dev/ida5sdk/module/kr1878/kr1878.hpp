
#ifndef _KR1878_HPP
#define _KR1878_HPP

#include "../idaidp.hpp"
#include "ins.hpp"

//------------------------------------------------------------------

#define amode     specflag2 // addressing mode
#define amode_x         0x10  // X:

//------------------------------------------------------------------
#define UAS_GNU 0x0001          // GNU assembler
//------------------------------------------------------------------
enum RegNo
{
  SR0,
  SR1,
  SR2,
  SR3,
  SR4,
  SR5,
  SR6,
  SR7,
  DSP,
  ISP,
  as,
  bs,
  cs,
  ds,
  vCS, vDS,       // virtual registers for code and data segments

};


//------------------------------------------------------------------

struct addargs_t
{
  ea_t ea;
  int nargs;
  op_t args[4][2];
};


//------------------------------------------------------------------
extern netnode helper;

#define IDP_SIMPLIFY 0x0001     // simplify instructions
#define IDP_PSW_W    0x0002     // W-bit in PSW is set

extern ushort idpflags;

inline bool dosimple(void)      { return (idpflags & IDP_SIMPLIFY) != 0; }
inline bool psw_w(void)         { return (idpflags & IDP_PSW_W) != 0; }

extern ea_t xmem;
ea_t calc_mem(op_t &x);
ea_t calc_data_mem(op_t &x, ushort segreg);

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

bool is_sp_based(const op_t &x);
int is_jump_func(const func_t *pfn, ea_t *jump_target);
int is_sane_insn(int nocrefs);
int may_be_func(void);           // can a function start here?

void init_analyzer(void);
void kr1878_data(ea_t ea);
const char *find_port(int address);

#endif // _KR1878_HPP
