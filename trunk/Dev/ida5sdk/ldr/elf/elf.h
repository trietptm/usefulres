#ifndef __ELF_H__
#define __ELF_H__

// gcc does not allow to initialize indexless arrays for some reason
// put an arbitrary number here
#ifdef __GNUC__
#define MAXRELSYMS 64
#else
#define MAXRELSYMS
#endif

struct proc_def
{
  ea_t    gtb_ea;        // address of the GOT (set by loader)
  char * (*proc_rel)(uchar type, ea_t fixaddr, ea_t value, uval_t symbase); // pointer to relocator function
#define E_RELOC_PATCHING  (char *)1
#define E_RELOC_UNKNOWN   (char *)2
#define E_RELOC_UNIMPL    (char *)3
#define E_RELOC_UNTESTED  (char *)4
  size_t (*proc_patch)(Elf64_Shdr *plt, void *gotps);  // pointer to patcher function
  char * (*proc_flag)(uint32 &e_flags); // pointer flag_proc interpretator
  char   *stubname;
  int    (*proc_sec_ext)(Elf64_Shdr *sh);
  int    (*proc_sym_ext)(Elf64_Sym *st, const char *name);
  char * (*proc_dyn_ext)(Elf64_Dyn *dyn);
  bool   (*proc_file_ext)(ushort filetype);
  void   (*proc_start_ext)(ushort type, uint32 &flags, ea_t &entry);
  ushort patch_mode;
  uchar  r_drop;
  uchar  r_gotset;      // relocation type: GOT
  uchar  r_err;         // relocation type: usually R_xxx_JUMP_SLOT
  uchar  r_chk;         // relocation type: usually R_xxx_GLOB_DAT
  uchar  got_cnt;       // number of GOT related relocations in the following array:
  uchar  relsyms[MAXRELSYMS]; // relocation types which must be to loaded symbols
};

// relocation speed
struct sym_rel {
  uval_t   value;       //absolute value or addr
  uchar    bind;        //binding
  char     type;        //type (-1 - not defined,
                        //-2 temp for rename NOTYPE)
                        //-3 temp for comment unloaded
  ushort   sec;         //index in section header table
  char     *name;       // temporary for NOTYPE only
};
// ids-loading
struct implib_name {
  char        *name;
  implib_name *prev;
};

void set_reloc(ea_t fixaddr, ea_t toea, uval_t value,
                                        adiff_t displ, int type, uval_t offbase=0);
void set_reloc_cmt(ea_t ea, int cmt);
#define RCM_PIC   0
#define RCM_ATT   1
#define RCM_COPY  2
#define RCM_TLS   3
void set_thunk_name(ea_t ea, ea_t name_ea);
void overflow(ea_t fixaddr, ea_t ea);
bool handle_arm_special_funcs(ea_t ea, const char *name);
ea_t get_tls_ea_by_offset(uint32 offset);

extern proc_def elf_pc,    elf_arm,  elf_ppc,  elf_mips, elf_sparc,
                elf_alpha, elf_ia64, elf_mc12, elf_i960, elf_arc,
                elf_m32r,  elf_st9,  elf_fr,   elf_x64,  elf_h8,
                elf_hp;

extern uint32 e_flags;     // ELF flags
extern bool unpatched;
extern bool elf64;
extern ushort e_type;      // ELF file type
extern ea_t prgend;

// user parameter
//!!! ��᫥����⥫쭮��� ��⮢ �ਢ易�� � �ଥ!!!
//    �᫨ ������� �ଠ ������ ����� ��।������
#define ELF_RPL_PLP   0x0001u   // convert plt PIC => noPIC
#define ELF_RPL_PLD   0x0002u   // convert plt (any) => indirect jump
#define ELF_RPL_GL    0x0004u   // patching GOT loading
#define ELF_RPL_UNL   0x0008u   // discard auxiliary values in plt/got
#define ELF_RPL_NPR   0x0010u   // 'natural' form in object-file GOT load
#define ELF_RPL_UNP   0x0020u   // 'unpatched' form in object-file GOT refer
#define ELF_AT_LIB    0x0040u   // mark 'allocated' as library (MIPS)
#define ELF_BUG_GOT   0x0080u   // covert all .got to offsets (MIPS)

// noform bits
#define ELF_DIS_GPLT  0x4000u   // disable search got reference in plt
#define ELF_DIS_OFFW  0x8000u   // can present offset bypass segment's

#define ELF_RPL_PTEST  (ELF_RPL_PLP | ELF_RPL_PLD | ELF_RPL_UNL)
#define ELF_RPL_NOTRN  (ELF_RPL_NPR | ELF_RPL_UNP)

#define FLAGS_CMT(bit, text)  if(e_flags & bit) { \
                               e_flags &= ~bit;   \
                               return(text);    }

//--------------------------------------------------------------------------
inline uval_t make64(uval_t oldval, uval_t newval, uval_t mask)
{
  return (oldval & ~mask) | (newval & mask);
}

inline ulong make32(ulong oldval, ulong newval, ulong mask)
{
  return (oldval & ~mask) | (newval & mask);
}

#define MASK(x) ((1 << x) - 1)

static const uval_t M32 = ulong(-1);
static const uval_t M24 = MASK(24);
static const uval_t M16 = MASK(16);
static const uval_t M8  = MASK(8);

inline uval_t extend_sign(uval_t value, size_t bits)
{
  return (value & (1 << (bits-1)))
        ? value | ~MASK(bits)
        : value & MASK(bits);
}

#endif
