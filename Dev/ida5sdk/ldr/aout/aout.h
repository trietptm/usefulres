#ifndef __AOUT_H__
#define __AOUT_H__

struct exec {
  ulong   a_info;     // Use macros N_MAGIC, etc for access
  ulong   a_text;     // length of text, in bytes
  ulong   a_data;     // length of data, in bytes
  ulong   a_bss;      // length of bss area for file, in bytes
  ulong   a_syms;     // length of symbol table data in file, in bytes
  ulong   a_entry;    // start address
  ulong   a_trsize;   // length of relocation info for text, in bytes
  ulong   a_drsize;   // length of relocation info for data, in bytes
// Added for i960
//  ulong   a_tload;    // Text runtime load adderr
//  ulong   a_dload;    // Data runtime load address
//  uchar   a_talign;   // Alignment of text segment
//  uchar   a_dalign;   // Alignmrnt of data segment
//  uchar   a_balign;   // Alignment of bss segment
//  char    a_relaxable;// Enough info for linker relax
};
//====================
#define N_TRSIZE(a)   ((a).a_trsize)
#define N_DRSIZE(a)   ((a).a_drsize)
#define N_SYMSIZE(a)    ((a).a_syms)

#define N_DYNAMIC(exec) ((exec).a_info & 0x80000000ul)

#define N_MAGIC(exec)    ((exec).a_info & 0xffff)
#define N_MACHTYPE(exec) ((enum machine_type)(((exec).a_info >> 16) & 0xff))
#define N_FLAGS(exec)    (((exec).a_info >> 24) & 0xff)
//====================
enum machine_type {
//  M_OLDSUN2 = 0,
  M_UNKNOWN       = 0,
  M_68010         = 1,
  M_68020         = 2,
  M_SPARC         = 3,
  /*-----------------11.07.98 04:09-------------------
   * skip a bunch so we don't run into any of suns numbers */
  /*-----------------11.07.98 04:09-------------------
   * make these up for the ns32k*/
  M_NS32032       = (64),           /* ns32032 running ? */
  M_NS32532       = (64 + 5),       /* ns32532 running mach */
  M_386           = 100,
  M_29K           = 101,            /* AMD 29000 */
  M_386_DYNIX     = 102,            /* Sequent running dynix */
  M_ARM           = 103,            /* Advanced Risc Machines ARM */
  M_SPARCLET      = 131,            /* SPARClet = M_SPARC + 128 */
  M_386_NETBSD    = 134,            /* NetBSD/i386 binary */
  M_68K_NETBSD    = 135,            /* NetBSD/m68k binary */
  M_68K4K_NETBSD  = 136,            /* NetBSD/m68k4k binary */
  M_532_NETBSD    = 137,            /* NetBSD/ns32k binary */
  M_SPARC_NETBSD  = 138,            /* NetBSD/sparc binary */
  M_PMAX_NETBSD   = 139,            /* NetBSD/pmax (MIPS little-endian) binary */
  M_VAX_NETBSD    = 140,            /* NetBSD/vax binary */
  M_ALPHA_NETBSD  = 141,            /* NetBSD/alpha binary */
  M_ARM6_NETBSD   = 143,            /* NetBSD/arm32 binary */
  M_SPARCLET_1    = 147,            /* 0x93, reserved */
  M_MIPS1         = 151,            /* MIPS R2000/R3000 binary */
  M_MIPS2         = 152,            /* MIPS R4000/R6000 binary */
  M_SPARCLET_2    = 163,            /* 0xa3, reserved */
  M_SPARCLET_3    = 179,            /* 0xb3, reserved */
  M_SPARCLET_4    = 195,            /* 0xc3, reserved */
  M_HP200         = 200,            /* HP 200 (68010) BSD binary */
  M_HP300         = (300 % 256),    /* HP 300 (68020+68881) BSD binary */
  M_HPUX          = (0x20c % 256),  /* HP 200/300 HPUX binary */
  M_SPARCLET_5    = 211,            /* 0xd3, reserved */
  M_SPARCLET_6    = 227,            /* 0xe3, reserved */
  M_SPARCLET_7    = 243             /* 0xf3, reserved */
};
//====================
#define OMAGIC 0407   // object file or impure executable
#define NMAGIC 0410   // pure executeable
#define ZMAGIC 0413   // demand-paged executable
#define BMAGIC 0415   // Used by a b.out object
#define QMAGIC 0314   // demand-paged executable with the header in the text.
                      // The first page is unmapped to help trap NULL pointer
                      // referenced
#define CMAGIC 0421   // core file
//====================
// Flags:
#define EX_PIC          0x80    /* contains position independent code */
#define EX_DYNAMIC      0x40    /* contains run-time link-edit info */
#define EX_DPMASK       0xC0    /* mask for the above */
//====================

#define N_BADMAG(x)   (N_MAGIC(x) != OMAGIC && \
                       N_MAGIC(x) != NMAGIC && \
                       N_MAGIC(x) != ZMAGIC && \
                       N_MAGIC(x) != QMAGIC)

#define _N_HDROFF(x) (1024 - sizeof(struct exec))

#define N_TXTOFF(x)                                              \
 (N_MAGIC(x) == ZMAGIC ? _N_HDROFF((x)) + sizeof (struct exec) : \
                        (N_MAGIC(x) == QMAGIC ? 0 : sizeof (struct exec)))

#define N_DATOFF(x) (N_TXTOFF(x) + (x).a_text)
#define N_TRELOFF(x) (N_DATOFF(x) + (x).a_data)
#define N_DRELOFF(x) (N_TRELOFF(x) + N_TRSIZE(x))
#define N_SYMOFF(x) (N_DRELOFF(x) + N_DRSIZE(x))
#define N_STROFF(x) (N_SYMOFF(x) + (x).a_syms)

// Address of text segment in memory after it is loaded
#define PAGE_SIZE (1UL << 12)
#define N_TXTADDR(x) (N_MAGIC(x) == QMAGIC ? PAGE_SIZE : 0)

#define PAGE_SIZE_ARM 0x8000UL
#define N_TXTADDR_ARM(x) (N_MAGIC(x) == QMAGIC ? 0 : PAGE_SIZE_ARM)

// Address of data segment in memory after it is loaded. (for linux)
/*
#define SEGMENT_SIZE  1024
#define _N_SEGMENT_ROUND(x) (((x) + SEGMENT_SIZE - 1) & ~(SEGMENT_SIZE - 1))
#define _N_TXTENDADDR(x)    (N_TXTADDR(x)+(x).a_text)
#define N_DATADDR(x)                                           \
                     (N_MAGIC(x)==OMAGIC? (_N_TXTENDADDR(x)) : \
                     (_N_SEGMENT_ROUND (_N_TXTENDADDR(x))))
// Address of bss segment in memory after it is loaded
#define N_BSSADDR(x) (N_DATADDR(x) + (x).a_data)
*/
//========================
struct nlist {
  union {
    char  *n_name;
    nlist *n_next;
    long  n_strx;
  } n_un;
  uchar n_type;
  char  n_other;
  short n_desc;
  ulong n_value;
};

#define N_UNDF    0     // Undefined symbol
#define N_ABS     2     // Absolute symbol -- addr
#define N_TEXT    4     // Text sym -- offset in text segment
#define N_DATA    6     // Data sym -- offset in data segment
#define N_BSS     8     // BSS sym  -- offset in bss segment
#define N_COMM    0x12  // Common symbol (visible after shared)
#define N_FN      0x1F  // File name of .o file
#define N_FN_SEQ  0x0C  // N_FN from Sequent compilers

#define N_EXT     1     // External (ORed wits UNDF, ABS, TEXT, DATA or BSS)
#define N_TYPE    0x1E
#define N_STAB    0xE0  // If present - debug symbol

#define N_INDR    0xA   // symbol refernced to another symbol

#define N_SETA    0x14  // Absolute set element symbol
#define N_SETT    0x16  // Text set element symbol
#define N_SETD    0x18  // Data set element symbol
#define N_SETB    0x1A  // Bss set element symbol

#define N_SETV    0x1C  // Pointer to set vector in data area. (from LD)

#define N_WARNING 0x1E  // Text has warnings

// Weak symbols
#define N_WEAKU   0x0D  // Weak undefined
#define N_WEAKA   0x0E  // Weak Absolute
#define N_WEAKT   0x0F  // Weak Text
#define N_WEAKD   0x10  // Weak Data
#define N_WEAKB   0x11  // Weak BSS

//=======================

#ifndef __DOS16__       // the following structure can't be compiled in 16bit

struct relocation_info {
  long  r_address;      // Adress (within segment) to be relocated
  ulong r_symbolnum:24; //The meaning of r_symbolnum depends on r_extern
  ulong r_pcrel:1;      // Nonzero means value is a pc-relative offset
  ulong r_length:2;     // Length (exp of 2) of the field to be relocated.
  ulong r_extern:1;     // 1 => relocate with value of symbol.
                        //      r_symbolnum is the index of the symbol
                        //      in file's the symbol table.
                        // 0 => relocate with the address of a segment.
                        //      r_symbolnum is N_TEXT, N_DATA, N_BSS or N_ABS
  ulong r_bsr:1;
  ulong r_disp:1;
  ulong r_pad:2;
};

#endif  // __DOS16__

//============================

#endif
