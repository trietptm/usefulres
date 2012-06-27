/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-2001 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              FIDO:   2:5020/209
 *                              E-mail: ig@datarescue.com
 *
 */

#ifndef _INTEL_HPP
#define _INTEL_HPP

#include "../idaidp.hpp"
#include <struct.hpp>
#include <fixup.hpp>
#include <demangle.hpp>
#include <srarea.hpp>
#include <ieee.h>
#include <typeinf.hpp>
#include <ints.hpp>
#include <frame.hpp>
#include <idd.hpp>
#pragma hdrstop

//---------------------------------
// not in ash.uflag for kernel (out struc/union descriptions)
inline bool tasm_ideal(void)  { return (ash.flag2 & AS2_IDEALDSCR) != 0; }

//---------------------------------
// Intel 80x86 cmd.auxpref bits
#define aux_lock        0x0001
#define aux_rep         0x0002
#define aux_repne       0x0004
#define aux_use32       0x0008  // segment type is 32-bits
#define aux_use64       0x0010  // segment type is 64-bits
#define aux_large       0x0020  // offset field is 32-bit (16-bit is not enough)
#define aux_short       0x0040  // short (byte) displacement used
#define aux_sgpref      0x0080  // a segment prefix byte is not used
#define aux_oppref      0x0100  // operand size prefix byte is not used
#define aux_adpref      0x0200  // address size prefix byte is not used
#define aux_basess      0x0400  // SS based instruction
#define aux_natop       0x0800  // operand size is not overridden by prefix
#define aux_natad       0x1000  // addressing mode is not overridden by prefix
#define aux_fpemu       0x2000  // FP emulator instruction

//---------------------------------
// operand types and other customization:
#define o_trreg         o_idpspec0      // IDP specific type
#define o_dbreg         o_idpspec1      // IDP specific type
#define o_crreg         o_idpspec2      // IDP specific type
#define o_fpreg         o_idpspec3      // IDP specific type
#define o_mmxreg        o_idpspec4      // IDP specific type
#define o_xmmreg        o_idpspec5      // IDP specific type


// 04.10.97: For o_mem,o_near,o_far we keep segment information as
// segrg - number of segment register to use
// if it is == SEGREG_IMM, then the segment was specified as an immediate
// value, look at segsel.

#define segrg           specval_shorts.high
#define SEGREG_IMM      0xFFFF          // this value of segrg means that
                                        // segment selector value is in
                                        // "segsel":
#define segsel          specval_shorts.low
#define hasSIB          specflag1
#define sib             specflag2
#define rex             insnpref

const int REX_W = 8;               // 64-bit operand size
const int REX_R = 4;               // modrm reg field extension
const int REX_X = 2;               // sib index field extension
const int REX_B = 1;               // modrm r/m, sib base, or opcode reg fields extension

//---------------------------------
bool insn_default_opsize_64(void);

inline bool mode16(void){ return (cmd.auxpref & (aux_use32|aux_use64)) == 0; } // 16-bit mode?
inline bool mode32(void){ return (cmd.auxpref & aux_use32) != 0; } // 32-bit mode?
inline bool mode64(void){ return (cmd.auxpref & aux_use64) != 0; } // 64-bit mode?
inline bool natad(void) { return (cmd.auxpref & aux_natad) != 0; } // natural address size (no prefixes)?
inline bool natop(void) { return (cmd.auxpref & aux_natop) != 0; } // natural operand size (no prefixes)?

inline bool ad16(void)          // is current addressing 16-bit?
{
  int p = cmd.auxpref & (aux_use32|aux_use64|aux_natad);
  return p == aux_natad || p == aux_use32;
}

inline bool ad32(void)          // is current addressing 64-bit?
{
  int p = cmd.auxpref & (aux_use32|aux_use64|aux_natad);
  return p == (aux_natad|aux_use32)
      || p == 0
      || p == aux_use64;
}

inline bool ad64(void)          // is current addressing 64-bit?
{
  int p = cmd.auxpref & (aux_use32|aux_use64|aux_natad);
  return p == (aux_natad|aux_use64);
}

inline bool op16(void)          // is current operand size 16-bit?
{
  int p = cmd.auxpref & (aux_use32|aux_use64|aux_natop);
  return p == aux_natop                                 // 16-bit segment, no prefixes
      || p == aux_use32                                 // 32-bit segment, 66h
      || p == aux_use64 && (cmd.rex & REX_W) == 0;      // 64-bit segment, 66h, no rex.w
}

inline bool op32(void)          // is current operand size 32-bit?
{
  int p = cmd.auxpref & (aux_use32|aux_use64|aux_natop);
  return p == 0                                         // 16-bit segment, 66h
      || p == (aux_use32|aux_natop)                     // 32-bit segment, no prefixes
      || p == (aux_use64|aux_natop) && (cmd.rex & REX_W) == 0; // 64-bit segment, 66h, no rex.w
}

inline bool op64(void)          // is current operand size 64-bit?
{
  return mode64()
      && ((cmd.rex & REX_W) != 0
       || natop() && insn_default_opsize_64()); // 64-bit segment, rex.w or insns-64
}

inline int sib_base(const op_t &x)                    // get extended sib base
{
  int base = x.sib & 7;
  if ( cmd.rex & REX_B )
    base |= 8;
  return base;
}

inline int sib_index(const op_t &x)                   // get extended sib index
{
  int index = (x.sib >> 3) & 7;
  if ( cmd.rex & REX_X )
    index |= 8;
  return index;
}

inline char address_dtyp(void)
{
  return ad64() ? dt_qword : ad32() ? dt_dword : dt_word;
}

inline char operand_dtyp(void)
{
  return op64() ? dt_qword : op32() ? dt_dword : op16() ? dt_word : dt_byte;
}

//---------------------------------
extern netnode intel_node;              // supvals -- for vmm functions
                                        // altvals -- for int ah values
                                        //           also for register values
                                        //           of indirect register calls

extern ulong idpflags;

#define AFIDP_PUSH      0x0001          // push seg; push num; is converted to offset
#define AFIDP_NOP       0x0002          // db 90h after jmp is converted to nop

#define AFIDP_MOVOFF    0x0004          // mov     reg, numoff  <- convert to offset
                                        // mov     segreg, immseg

#define AFIDP_MOVOFF2   0x0008          // mov     z, numoff    <- convert to offset
                                        // mov     z, immseg
                                        // where z - o_mem, o_displ
#define AFIDP_ZEROINS   0x0010          // allow zero opcode instructions:
                                        //      add [bx+si], al  (16bit)
                                        //      add [eax], al    (32bit)

#define AFIDP_BRTTI     0x0020          // Advanced analysis of Borlands RTTI
#define AFIDP_UNKRTTI   0x0040          // -"- with 'unknown_libname'
#define AFIDP_EXPFUNC   0x0080          // for PE? bc(ms?) - expanding
                                        // function (exception subblock)
#define AFIDP_DIFBASE   0x0100          // Allow references with different segment bases
#define AFIDP_NOPREF    0x0200          // Don't display superfluous prefixes
#define AFIDP_NOVXD     0x0400          // Don't interpret int 20 as VxDcall
#define AFIDP_NOFPEMU   0x0800          // Disable FPU emulation instructions
#define AFIDP_SHOWRIP   0x1000          // Explicit RIP-addressing

inline bool should_af_push(void)   { return (idpflags & AFIDP_PUSH) != 0; }
inline bool should_af_nop(void)    { return (idpflags & AFIDP_NOP) != 0; }
inline bool should_af_movoff(void) { return (idpflags & AFIDP_MOVOFF) != 0; }
inline bool should_af_movoff2(void){ return (idpflags & AFIDP_MOVOFF2) != 0; }
inline bool should_af_zeroins(void){ return (idpflags & AFIDP_ZEROINS) != 0; }
inline bool should_af_brtti(void)  { return (idpflags & AFIDP_BRTTI) != 0; }
inline bool should_af_urtti(void)  { return (idpflags & AFIDP_UNKRTTI) != 0; }
inline bool should_af_fexp(void)   { return (idpflags & AFIDP_EXPFUNC) != 0; }
inline bool should_af_difbase(void){ return (idpflags & AFIDP_DIFBASE) != 0; }
inline bool should_af_nopref(void) { return (idpflags & AFIDP_NOPREF) != 0; }
inline bool should_af_vxd(void)    { return (idpflags & AFIDP_NOVXD) == 0; }
inline bool should_af_fpemu(void)  { return (idpflags & AFIDP_NOFPEMU) == 0; }
inline bool should_af_showrip(void){ return (idpflags & AFIDP_SHOWRIP) != 0; }

inline int indent_spaces(size_t sz) { int len = inf.indent - sz;
                                      return (len < 1) ? 1 : len; }

enum RegNo
{
  R_ax = 0,
  R_cx = 1,
  R_dx = 2,
  R_bx = 3,
  R_sp = 4,
  R_bp = 5,
  R_si = 6,
  R_di = 7,
  R_r8 = 8,
  R_r9 = 9,
  R_r10 = 10,
  R_r11 = 11,
  R_r12 = 12,
  R_r13 = 13,
  R_r14 = 14,
  R_r15 = 15,

  R_al = 16,
  R_cl = 17,
  R_dl = 18,
  R_bl = 19,
  R_ah = 20,
  R_ch = 21,
  R_dh = 22,
  R_bh = 23,

  R_spl = 24,
  R_bpl = 25,
  R_sil = 26,
  R_dil = 27,

  R_ip = 28,

  R_es = 29,     // 0
  R_cs = 30,     // 1
  R_ss = 31,     // 2
  R_ds = 32,     // 3
  R_fs = 33,
  R_gs = 34,

};

inline bool is_segreg(int r) { return r >= R_es && r <= R_fs; }

//------------------------------------------------------------------
inline sel_t getDS(ea_t EA) { return getSR(EA,R_ds); }
inline sel_t getES(ea_t EA) { return getSR(EA,R_es); }
inline sel_t getSS(ea_t EA) { return getSR(EA,R_ss); }
inline sel_t getFS(ea_t EA) { return getSR(EA,R_fs); }
inline sel_t getGS(ea_t EA) { return getSR(EA,R_gs); }

//------------------------------------------------------------------
void intel_header(void);
void intel_footer(void);

void intel_assumes(ea_t ea);
void intel_segstart(ea_t ea);
void intel_segend(ea_t ea);

void intel_out(void);
void gen_stkvar_def(char *buf, size_t bufsize, const member_t *mptr, sval_t v);
ssize_t get_type_name(flags_t flag, ea_t ea_or_id, char *buf, size_t bufsize);
int is_align_insn(ea_t ea);
ea_t get_segval(const op_t &x);
ea_t get_mem_ea(const op_t &x);

void pc_data(ea_t ea);
int pc_ana(void);
int pc_emu(void);
bool pc_outop(op_t &op);

#define F_R_AX 0                // Number of AX file
#define F_R_BX 1                // Number of BX file
#define F_R_CX 2                // Number of CX file
#define F_R_DX 3                // Number of DX file
#define F_R_BP 4                // Number of BP file
#define F_R_SI 5                // Number of SI file
#define F_R_DI 6                // Number of DI file

bool is_switch(switch_info_t *si);
bool equal_ops(const op_t &x, const op_t &y);
bool is_sp_based(const op_t &x);
bool create_func_frame(func_t *pfn);
int is_jump_func(func_t *pfn, ea_t *jump_target, ea_t *func_pointer);
bool is_alloca_probe(const char *name);
void check_for_alloca_renaming(va_list va);
bool is_stack_changing_func(ea_t ea);
int pc_calc_arglocs(const type_t *&type, ulong *arglocs, int maxn);
bool pc_use_stkvar_type(ea_t ea, const type_t *type, const char *name);
int pc_use_regvar_type(ea_t ea,
                       const type_t * const *types,
                       const char * const *names,
                       const ulong *regs,
                       int n);
int get_fastcall_regs(const int **pregs);
int get_thiscall_regs(const int **pregs);
int calc_cdecl_purged_bytes(ea_t ea);
void setup_til(void);
//------------------------------
struct vxd_info       // loaded from ida.int
{
  ushort sp_off;      // bytes purge upon return
  ushort prm_pos;     // offset in strings to params (0 if none)
  ushort cmt_pos;     // offset in strings to comment (0 if none)
  char   strings[MAXSTR]; // name [param] [comment]
};

bool vxd_information(vxd_info *vx);
void pc_move_segm(ea_t from, segment_t *s);

typedef const regval_t &idaapi getreg_t(const char *name, const regval_t *regvalues);

bool pc_get_operand_info(ea_t ea,
                         int n,
                         int tid,
                         getreg_t *getreg,
                         const regval_t *regvalues,
                         idd_opinfo_t *opinf);
const char *get_dbg_regname(int reg);
uval_t getr(int reg, char dtyp, getreg_t *getreg, const regval_t *rv);
//------------------------------
bool borland_template(va_list va);
int  borland_coagulate(va_list va);
void borland_signature(void);
//-------------------------------
void cover_func_finalize(va_list va);
void compiler_preset(bool newfile);
void compiler_finalize(void);
bool is_sane_insn(int reason);
int  may_be_func(void);
bool might_change_sp(ea_t ea);
//----------------------------------------------------------------------
bool is_segment_normal(ea_t ea);

enum compiler_t
{
  comp_unset = 0,
  comp_vc,  // ms VisualC
  comp_bc,  // Borland CPP
  comp_bd,  // Borland Delphi
  comp_wat, // Watcom
  comp_unknown  //LAST!
};

extern compiler_t Compiler;
extern bool       VCLpresent;
#define INFO_TAG  'B'

void postset_unknown(ea_t from, ea_t to);
void prepare_unknown(ea_t ea, size_t sz, ea_t *last);
bool has_Brtti(void);
void coagulate_exception(ea_t InitBlock_function, ea_t StruEx_function, bool vc7seh);
//----------------------------------------------------------------------

class mblock_t;
extern mvm_t mvm;
extern netnode ignore_micro;
int make_micro(insn_t &ins,mblock_t *_mb);      // in: cmd structure
inline bool should_ignore_micro(ea_t ea) { return ignore_micro.charval(ea, 0); }
inline void set_ignore_micro(ea_t ea) { ignore_micro.charset(ea, 1, 0); }

//-------------------------------------------------------------------------

//
//      Don't use the following define's with underscores at the start!
//

extern int procnum;     // internal processor number
extern ulong pflag;

#define _PT_486p        0x00000001
#define _PT_486r        0x00000002
#define _PT_386p        0x00000004
#define _PT_386r        0x00000008
#define _PT_286p        0x00000010
#define _PT_286r        0x00000020
#define _PT_086         0x00000040
#define _PT_586p        0x00000080      // Pentium real mode
#define _PT_586r        0x00000100      // Pentium protected mode
#define _PT_686r        0x00000200      // Pentium Pro real
#define _PT_686p        0x00000400      // Pentium Pro protected
#define _PT_mmx         0x00000800      // MMX extensions
#define _PT_pii         0x00001000      // Pentium II
#define _PT_3d          0x00002000      // 3DNow! extensions
#define _PT_piii        0x00004000      // Pentium III
#define _PT_k7          0x00008000      // AMD K7
#define _PT_p4          0x00010000      // Pentium 4

//
//   The following values mean 'is XXX processor or better?'
//

#define PT_p4            _PT_p4
#define PT_k7           ( PT_p4   | _PT_k7   )
#define PT_piii         ( PT_k7   | _PT_piii )
#define PT_k62          ( PT_piii | _PT_3d   )
#define PT_3d            _PT_3d
#define PT_pii          ( PT_piii | _PT_pii  )
#define PT_mmx          (_PT_mmx  | _PT_3d   )
#define PT_686p         ( PT_pii  | _PT_686p )
#define PT_686r         ( PT_686p | _PT_686r )
#define PT_586p         ( PT_686r | _PT_586p )
#define PT_586r         ( PT_586p | _PT_586r )
#define PT_486p         ( PT_586r | _PT_486p )
#define PT_486r         ( PT_486p | _PT_486r )
#define PT_386p         ( PT_486r | _PT_386p )
#define PT_386r         ( PT_386p | _PT_386r )
#define PT_286p         ( PT_386r | _PT_286p )
#define PT_286r         ( PT_286p | _PT_286r )
#define PT_086          ( PT_286r | _PT_086  )

//
//   The following values mean 'is exactly XXX processor?'
//

#define PT_ismmx        ( _PT_mmx             )
#define PT_is686        ( _PT_686r | _PT_686p )
#define PT_is586        ( _PT_586r | _PT_586p )
#define PT_is486        ( _PT_486r | _PT_486p )
#define PT_is386        ( _PT_386r | _PT_386p )
#define PT_is286        ( _PT_286r | _PT_286p )
#define PT_is086        ( _PT_086 )

//---------------------------------------------------------------------
inline bool isProtected(ulong type)
{
   return (type & (_PT_286p |
                   _PT_386p |
                   _PT_486p |
                   _PT_586p |
                   _PT_686p |
                   _PT_pii    )) != 0;
}

inline bool isAMD(ulong type)   { return (type & PT_k7  ) != 0; }
inline bool isp4 (ulong type)   { return (type & PT_p4  ) != 0; }
inline bool isp3 (ulong type)   { return (type & PT_piii) != 0; }
inline bool is3dnow(ulong type) { return (type & PT_3d  ) != 0; }
inline bool ismmx(ulong type)   { return (type & PT_mmx ) != 0; }
inline bool isp2 (ulong type)   { return (type & PT_pii ) != 0; }
inline bool is686(ulong type)   { return (type & PT_686r) != 0; }
inline bool is586(ulong type)   { return (type & PT_586r) != 0; }
inline bool is486(ulong type)   { return (type & PT_486r) != 0; }
inline bool is386(ulong type)   { return (type & PT_386r) != 0; } // is 386 or better ?
inline bool is286(ulong type)   { return (type & PT_286r) != 0; } // is 286 or better ?

#endif // _INTEL_HPP
