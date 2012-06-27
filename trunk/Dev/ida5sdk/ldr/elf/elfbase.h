#ifndef __ELFBASE_H__
#define __ELFBASE_H__

//=========================================================================
//typedef void far *      Elf32_Addr;     // program address
typedef unsigned long   Elf32_Addr;    //force normal =
#ifdef __BORLANDC__
#if sizeof(Elf32_Addr) != sizeof(void *)
#error
#endif
#endif
typedef unsigned short  Elf32_Half;
typedef unsigned long   Elf32_Off;      // file offset
typedef signed long     Elf32_Sword;
typedef unsigned long   Elf32_Word;
//=========================================================================
#define     EI_MAG0     0
#define       ELFMAG0      0x7f
#define     EI_MAG1     1
#define       ELFMAG1      'E'
#define     EI_MAG2     2
#define       ELFMAG2      'L'
#define     EI_MAG3     3
#define       ELFMAG3      'F'
#define     EI_CLASS    4
#define       ELFCLASSNONE  0   // Invalid class
#define       ELFCLASS32    1   // 32bit object
#define       ELFCLASS64    2   // 64bit object
#define     EI_DATA     5
#define       ELFDATANONE    0   // Invalid data encoding
#define       ELFDATA2LSB    1   // low byte first
#define       ELFDATA2MSB    2   // high byte first
#define     EI_VERSION  6        // file version
#define     EI_PAD      7        // start of padding bytes
#define EI_NIDENT 16             // sizeof


typedef struct {
  unsigned char   e_ident[EI_NIDENT];   // see above
  Elf32_Half      e_type;               // enum ET
  Elf32_Half      e_machine;            // enum EM
  Elf32_Word      e_version;            // enum EV
  Elf32_Addr      e_entry;              // virtual start address
  Elf32_Off       e_phoff;              // off to program header table's (pht)
  Elf32_Off       e_shoff;              // off to section header table's (sht)
  Elf32_Word      e_flags;              // EF_machine_flag
  Elf32_Half      e_ehsize;             // header's size
  Elf32_Half      e_phentsize;          // size of pht element
  Elf32_Half      e_phnum;              // entry counter in pht
  Elf32_Half      e_shentsize;          // size of sht element
  Elf32_Half      e_shnum;              // entry count in sht
  Elf32_Half      e_shstrndx;           // sht index in name table
} Elf32_Ehdr;


enum elf_ET {
  ET_NONE = 0,    // No file type
  ET_REL  = 1,    // Relocatable file
  ET_EXEC = 2,    // Executable file
  ET_DYN  = 3,    // Share object file
  ET_CORE = 4,    // Core file
  ET_LOOS   = 0xfe00u,  // OS specific
  ET_HIOS   = 0xfeffu,  // OS specific
  ET_LOPROC = 0xff00u,  // Processor specific
  ET_HIPROC = 0xffffu   // Processor specific
};

enum elf_EM {
  EM_NONE  = 0,   // No machine
  EM_M32   = 1,   // AT & T WE 32100
  EM_SPARC = 2,   // Sparc
  EM_386   = 3,   // Intel 80386
  EM_68K   = 4,   // Motorola 68000
  EM_88K   = 5,   // Motorola 88000
  EM_486   = 6,
//ATTENTION!!! in documentation present next values
//  EM_860   = 6,   // Intel 80860
//  EM_MIPS  = 7,    // MIPS RS3000
//in linux RS3000 = 8, !!!
// дальше взято из linux
    EM_860	   =  7,
    EM_MIPS	   =  8,  // Mips 3000 (officialy, big-endian only)
    EM_S370    =  9,  // IBM System370
    EM_MIPS_RS3_BE = 10,  //MIPS R3000 Big Endian
//  EM_SPARC_64    = 11,  // SPARC v9
  EM_PARISC      = 15,  // HPPA
    EM_VPP550      = 17,  // Fujitsu VPP500
  EM_SPARC32PLUS = 18,  // Sun's v8plus
  EM_I960  = 19,  // Intel 960
  EM_PPC   = 20,  // Power PC
    EM_PPC64 = 21,  // 64-bit PowerPC
    EM_S390  = 22,  // IBM S/390
    EM_V800  = 36,  // NEC V800 series
    EM_FR20  = 37,  // Fujitsu FR20
    EM_RH32  = 38,  // TRW RH32
    EM_MCORE = 39,  // Motorola M*Core (May also be taken by Fujitsu MMA)
  EM_ARM   = 40,  // ARM
    EM_OLD_ALPHA = 41,  // Digital Alpha
  EM_SH    = 42,  // Hitachi SH
  EM_SPARC64= 43,  // Sparc 64
    EM_TRICORE = 44,  // Siemens Tricore embedded processor
  EM_ARC   = 45,  // ARC
  EM_H8300  = 46,  // Hitachi H8/300
  EM_H8300H = 47,  // Hitachi H8/300H
  EM_H8S    = 48,  // Hitachi H8S
  EM_H8500  = 49,  // Hitachi H8/500
  EM_IA64  = 50,  // Intel Itanium IA64
    EM_MIPS_X   = 51, // Stanford MIPS-X
    EM_COLDFIRE = 52, // Motorola Coldfire
  EM_6812  = 53,  // Motorla MC68HC12
    EM_MMA      = 54, // Fujitsu Multimedia Accelerator
    EM_PCP      = 55, // Siemens PCP
    EM_NCPU     = 56, // Sony nCPU embedded RISC processor
    EM_NDR1     = 57, // Denso NDR1 microprocesspr
    EM_STARCORE = 58, // Motorola Star*Core processor
    EM_ME16     = 59, // Toyota ME16 processor
    EM_ST100    = 60, // STMicroelectronics ST100 processor
    EM_TINYJ    = 61, // Advanced Logic Corp. TinyJ embedded processor
  EM_X86_64= 62,  // Advanced Micro Devices X86-64 processor
    EM_PDP10    = 64, // DEC PDP-10
    EM_PDP11    = 65, // DEC PDP-11
    EM_FX66     = 66, // Siemens FX66 microcontroller
  EM_ST9   = 67,  // ST9+
    EM_ST7      = 68, // STM ST7 8-bit microcontroller
    EM_68HC16   = 69, // MC68HC16
  EM_6811  = 70,  // Motorla MC68HC11
    EM_68HC08   = 71, // MC68HC08
    EM_68HC05   = 72, // MC68HC05
    EM_SVX      = 73, // Silicon Graphics SVx
    EM_ST19     = 74, // STMicroelectronics ST19 8-bit cpu
    EM_VAX      = 75, // VAX
    EM_CRIS     = 76, // Axis Communications 32-bit embedded processor
    EM_JAVELIN  = 77, // Infineon Technologies 32-bit embedded cpu
    EM_FIREPATH = 78, // Element 14 64-bit DSP processor
    EM_ZSP      = 79, // LSI Logic's 16-bit DSP processor
    EM_MMIX     = 80, // Donald Knuth's educational 64-bit processor
    EM_HUANY    = 81, // Harvard's machine-independent format
    EM_PRISM    = 82, // SiTera Prism
    EM_AVR      = 83, // Atmel AVR 8-bit microcontroller
  EM_FR    = 84,  // Fujitsu FR Family
    EM_D10V     = 85, // Mitsubishi D10V
    EM_D30V     = 86, // Mitsubishi D30V
    EM_V850     = 87, // NEC v850 */
  EM_M32R  = 88,  // Mitsubishi 32bit RISC
    EM_MN10300  = 89, // Matsushita MN10300
    EM_MN10200  = 90, // Matsushita MN10200
    EM_PJ       = 91, // picoJava
    EM_OPENRISC = 92, // OpenRISC 32-bit embedded processor
    EM_ARC_A5   = 93, // ARC Cores Tangent-A5
    EM_XTENSA   = 94, // Tensilica Xtensa Architecture
    EM_PJ_OLD   = 99, // picoJava
    EM_IP2K     =101, // Ubicom IP2022 micro controller
    EM_CR       =103, // National Semiconductor CompactRISC
    EM_MSP430   =105, // TI msp430 micro controller
    EM_CRX      =114, // National Semiconductor CRX
    EM_CYGNUS_POWERPC = 0x9025, // Cygnus PowerPC ELF backend
  EM_ALPHA = 0x9026 // DEC Alpha
};

enum elf_EV {
  EV_NONE    = 0, // None version
  EV_CURRENT = 1  // Current version
// in linux header
// EV_NUM      = 2
};
#define VER_FLG_BASE    0x1   // in vd_flags
#define VER_FLG_WEAK    0x2   // -"-

// special section indexed
enum elh_SHN {
  SHN_UNDEF     = 0,       // undefined/missing/...
#ifndef __DOS16__
  SHN_LORESERVE = 0xff00,
  SHN_LOPROC    = 0xff00,
  SHN_HIPROC    = 0xff1f,
  SHN_ABS       = 0xfff1,  // absolute value
  SHN_COMMON    = 0xfff2,  //common values (fortran/c)
  SHN_HIRESERVE = 0xffff
#else
#define  SHN_LORESERVE  0xff00ul
#define  SHN_LOPROC     0xff00ul
#define  SHN_HIPROC     0xff1ful
#define  SHN_ABS        0xfff1ul  // absolute value
#define  SHN_COMMON     0xfff2ul  //common values (fortran/c)
#define  SHN_HIRESERVE  0xfffful
#endif
};
//==========

typedef struct {
  Elf32_Word    sh_name;      // index in string table
  Elf32_Word    sh_type;      // enum SHT
  Elf32_Word    sh_flags;     // enum SHF
  Elf32_Addr    sh_addr;      // address in memmory (or 0)
  Elf32_Off     sh_offset;    // offset in file
  Elf32_Word    sh_size;      // section size in bytes
  Elf32_Word    sh_link;      // index in symbol table
  Elf32_Word    sh_info;      // extra information
  Elf32_Word    sh_addralign; // 0 & 1 => no alignment
  Elf32_Word    sh_entsize;   // size symbol table or eq.
} Elf32_Shdr;


enum elf_SHT {
  SHT_NULL      = 0,    // inactive - no assoc. section
  SHT_PROGBITS  = 1,    // internal program information
  SHT_SYMTAB    = 2,    // symbol table (static)
  SHT_STRTAB    = 3,    // string table
  SHT_RELA      = 4,    // relocation entries
  SHT_HASH      = 5,    // symbol hash table
  SHT_DYNAMIC   = 6,    // inf. for dynamic linking
  SHT_NOTE      = 7,    // additional info
  SHT_NOBITS    = 8,    // no placed in file
  SHT_REL       = 9,    // relocation entries without explicit address
  SHT_SHLIB     = 10,   // RESERVED
  SHT_DYNSYM    = 11,   // Dynamic Symbol Table
// abi 3
  SHT_INIT_ARRAY    = 14, // Array of ptrs to init functions
  SHT_FINI_ARRAY    = 15, // Array of ptrs to finish functions
  SHT_PREINIT_ARRAY = 16, // Array of ptrs to pre-init funcs
  SHT_GROUP         = 17, // Section contains a section group
  SHT_SYMTAB_SHNDX  = 18, // Indicies for SHN_XINDEX entries
//  SHT_NUM       = 12,
#ifndef __DOS16__
  SHT_LOOS      = 0x60000000ul,
  SHT_HIOS      = 0x6ffffffful,
  SHT_LOPROC    = 0x70000000ul,
  SHT_HIPROC    = 0x7ffffffful,
  SHT_LOUSER    = 0x80000000ul,
  SHT_HIUSER    = 0xfffffffful,
//
  SHT_GNU_LIBLIST = 0x6ffffff7ul, // List of prelink dependencies

// The next three section types are defined by Solaris, and are named SHT_SUNW*.  We use them in GNU code, so we also define SHT_GNU*
  SHT_SUNW_verdef   = 0x6ffffffd, // Versions defined by file
  SHT_SUNW_verneed  = 0x6ffffffe, // Versions needed by file
  SHT_SUNW_versym   = 0x6fffffff  // Symbol versions
#endif
};

// section by index 0 ==
// { 0, SHT_NULL, 0, 0, 0, 0, SHN_UNDEF, 0, 0, 0 };

enum elf_SHF {
  SHF_WRITE     = (1 << 0),     // writable data
  SHF_ALLOC     = (1 << 1),     // occupies memory
  SHF_EXECINSTR = (1 << 2),     // machine instruction

  SHF_MERGE     = (1 << 4),     // can be merged
  SHF_STRINGS   = (1 << 5),     // contains nul-terminated strings
  SHF_INFO_LINK = (1 << 6),     // sh_info contains SHT index
  SHF_LINK_ORDER= (1 << 7),     // preserve order after combining
  SHF_OS_NONCONFORMING = (1 << 8), // non-standard os specific handling required
  SHF_GROUP     = (1 << 9),     // section is memory of a group
  SHF_TLS       = (1 << 10),    // section holds thread-local data

  SHF_MASKOS    = 0x0ff00000,   // os specific
  SHF_MASKPROC  = 0xf0000000,   // processor specific
};

typedef struct {
  Elf32_Word    st_name;        //index in string table
  Elf32_Addr    st_value;       //absolute value or addr
  Elf32_Word    st_size;        //0-unknow or no, elsewere symbol size in bytes
  unsigned char st_info;        //type and attribute (thee below)
  unsigned char st_other;       //==0
  Elf32_Half    st_shndx;       //index in section header table
} Elf32_Sym;

#define ELF32_ST_BIND(i)    ((i)>>4)
#define ELF32_ST_TYPE(i)    ((i)&0xf)
#define ELF32_ST_INFO(b,t)  (((b)<<4)+((t)&0xf))

enum elf_ST_BIND {
  STB_LOCAL  = 0,
  STB_GLOBAL = 1,
  STB_WEAK   = 2,
  STB_LOOS   = 10,              //OS-
  STB_HIOS   = 12,              //   specific
  STB_LOPROC = 13,              //processor-
  STB_HIPROC = 15               //          specific
};

enum elf_ST_TYPE {
  STT_NOTYPE    = 0,
  STT_OBJECT  = 1,              // associated with data object
  STT_FUNC    = 2,              // associated with function or execut. code
  STT_SECTION = 3,
  STT_FILE    = 4,              // name of source file
  STT_COMMON  = 5,              // Uninitialized common section
  STT_TLS     = 6,              // TLS-data object
  STT_LOOS   = 10,              //OS-
  STT_HIOS   = 12,              //   spceific
  STT_LOPROC = 13,              //processor-
  STT_HIPROC = 15               //          specific
};

// relocation
typedef struct {
  Elf32_Addr    r_offset;       //virtual address
  Elf32_Word    r_info;         //type of relocation
}Elf32_Rel;

#define ELF32_R_SYM(i)    ((i)>>8)
#define ELF32_R_TYPE(i)   ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

typedef struct {
  Elf32_Addr    r_offset;
  Elf32_Word    r_info;
  Elf32_Sword   r_addend;       //constant to compute
}Elf32_Rela;

//=================Loading & dynamic linking========================
// program header
typedef struct {
  Elf32_Word    p_type;         //Segment type. see below
  Elf32_Off     p_offset;       //from beginning of file at 1 byte of segment resides
  Elf32_Addr    p_vaddr;        //virtual addr of 1 byte
  Elf32_Addr    p_paddr;        //reserved for system
  Elf32_Word    p_filesz;       //may be 0
  Elf32_Word    p_memsz;        //my be 0
  Elf32_Word    p_flags;        // for PT_LOAD access mask (PF_
#define PF_R 4
#define PF_W 2
#define PF_X 1
  Elf32_Word    p_align;        //0/1-no,
}Elf32_Phdr;

enum elf_SEGTYPE {
  PT_NULL    = 0,               //ignore entries in program table
  PT_LOAD    = 1,               //loadable segmen described in _filesz & _memsz
  PT_DYNAMIC = 2,               //dynamic linking information
  PT_INTERP  = 3,               //path name to interpreter (loadable)
  PT_NOTE    = 4,               //auxilarry information
  PT_SHLIB   = 5,               //reserved. Has no specified semantics
  PT_PHDR    = 6,               //location & size program header table
  PT_TLS     = 7,               //Thread local storage segment
#ifndef __DOS16__
  PT_LOOS    = 0x60000000ul,    // OS-
  PT_HIOS    = 0x6ffffffful,    //    specific
  PT_LOPROC  = 0x70000000ul,    // processor-
  PT_HIPROC  = 0x7ffffffful,    //           specific
//
  PT_GNU_EH_FRAME = (PT_LOOS + 0x474e550ul),
  PT_GNU_STACK    = (PT_LOOS + 0x474e551ul),
  PT_GNU_RELRO    = (PT_LOOS + 0x474e552) // Read-only after relocation
#endif
};

//=================Dynamic section===============================
typedef struct {
  Elf32_Sword   d_tag;          //see below
  union {
    Elf32_Word  d_val;          //integer value with various interpretation
    Elf32_Addr  d_ptr;          //programm virtual adress
  } d_un;
}Elf32_Dyn;
//extern Elf32_Dyn _DYNAMIC[];

enum elf_DTAG {
  DT_NULL     = 0,              //(-) end ofd _DYNAMIC array
  DT_NEEDED   = 1,              //(v) str-table offset name to needed library
  DT_PLTRELSZ = 2,              //(v) tot.size in bytes of relocation entries
  DT_PLTGOT   = 3,              //(p) see below
  DT_HASH     = 4,              //(p) addr. of symbol hash teble
  DT_STRTAB   = 5,              //(p) addr of string table
  DT_SYMTAB   = 6,              //(p) addr of symbol table
  DT_RELA     = 7,              //(p) addr of relocation table
  DT_RELASZ   = 8,              //(v) size in bytes of DT_RELA table
  DT_RELAENT  = 9,              //(v) size in bytes of DT_RELA entry
  DT_STRSZ    = 10,             //(v) size in bytes of string table
  DT_SYMENT   = 11,             //(v) size in byte of symbol table entry
  DT_INIT     = 12,             //(p) addr. of initialization function
  DT_FINI     = 13,             //(p) addr. of termination function
  DT_SONAME   = 14,             //(v) offs in str.-table - name of shared object
  DT_RPATH    = 15,             //(v) offs in str-table - search patch
  DT_SYMBOLIC = 16,             //(-) start search of shared object
  DT_REL      = 17,             //(p) similar to DT_RELA
  DT_RELSZ    = 18,             //(v) tot.size in bytes of DT_REL
  DT_RELENT   = 19,             //(v) size in bytes of DT_REL entry
  DT_PLTREL   = 20,             //(v) type of relocation (DT_REL or DT_RELA)
  DT_DEBUG    = 21,             //(p) not specified
  DT_TEXTREL  = 22,             //(-) segment permisson
  DT_JMPREL   = 23,             //(p) addr of dlt procedure (if present)
  DT_BIND_NOW         = 24,
  DT_INIT_ARRAY       = 25,
  DT_FINI_ARRAY       = 26,
  DT_INIT_ARRAYSZ     = 27,
  DT_FINI_ARRAYSZ     = 28,
  DT_RUNPATH          = 29,
  DT_FLAGS            = 30,
#define DF_ORIGIN         0x01
#define DF_SYMBOLIC       0x02
#define DF_TEXTREL        0x04
#define DF_BIND_NOW       0x08
#define DF_STATIC_TLS     0x10
  DT_ENCODING         = 31,
  DT_PREINIT_ARRAY    = 32,
  DT_PREINIT_ARRAYSZ  = 33,
#ifndef __DOS16__
  DT_LOOS       = 0x6000000dul, // OS-
  DT_HIOS       = 0x6fff0000ul, //    spcecific
//
  OLD_DT_LOOS   = 0x60000000ul,
  OLD_DT_HIOS   = 0x6ffffffful,
//
  DT_VALRNGLO       = 0x6ffffd00ul,   // solaris
  DT_GNU_PRELINKED  = 0x6ffffdf5ul,   // solaris
  DT_GNU_CONFLICTSZ = 0x6ffffdf6ul,   // solaris
  DT_GNU_LIBLISTSZ  = 0x6ffffdf7ul,   // solaris
  DT_CHECKSUM       = 0x6ffffdf8ul,
  DT_PLTPADSZ       = 0x6ffffdf9ul,
  DT_MOVEENT        = 0x6ffffdfaul,
  DT_MOVESZ         = 0x6ffffdfbul,
  DT_FEATURE        = 0x6ffffdfcul,
#define DTF_1_PARINIT	0x00000001
#define DTF_1_CONFEXP	0x00000002
  DT_POSFLAG_1      = 0x6ffffdfdul,
#define DF_P1_LAZYLOAD	0x00000001
#define DF_P1_GROUPPERM	0x00000002
  DT_SYMINSZ        = 0x6ffffdfeul,
  DT_SYMINENT       = 0x6ffffdfful,
  DT_VALRNGHI       = 0x6ffffdfful,
  DT_ADDRRNGLO      = 0x6ffffe00ul,
  DT_GNU_CONFLICT   = 0x6ffffef8ul,
  DT_GNU_LIBLIST    = 0x6ffffef9ul,
  DT_CONFIG         = 0x6ffffefaul,
  DT_DEPAUDIT       = 0x6ffffefbul,
  DT_AUDIT          = 0x6ffffefcul,
  DT_PLTPAD         = 0x6ffffefdul,
  DT_MOVETAB        = 0x6ffffefeul,
  DT_SYMINFO        = 0x6ffffefful,
  DT_ADDRRNGHI      = 0x6ffffefful,
  DT_RELACOUNT      = 0x6ffffff9ul,
  DT_RELCOUNT       = 0x6ffffffaul,
  DT_FLAGS_1        = 0x6ffffffbul,
#define DF_1_NOW        0x00000001
#define DF_1_GLOBAL     0x00000002
#define DF_1_GROUP      0x00000004
#define DF_1_NODELETE   0x00000008
#define DF_1_LOADFLTR   0x00000010
#define DF_1_INITFIRST	0x00000020
#define DF_1_NOOPEN     0x00000040
#define DF_1_ORIGIN     0x00000080
#define DF_1_DIRECT     0x00000100
#define DF_1_TRANS      0x00000200
#define DF_1_INTERPOSE	0x00000400
#define DF_1_NODEFLIB   0x00000800
#define DF_1_NODUMP     0x00001000
#define DF_1_CONLFAT    0x00002000
  DT_VERDEF         = 0x6ffffffcul,
  DT_VERDEFNUM      = 0x6ffffffdul,
  DT_VERNEED        = 0x6ffffffeul,
  DT_VERNEEDNUM     = 0x6ffffffful,
  DT_VERSYM         = 0x6ffffff0ul,   // solaris
//
  DT_LOPROC   = 0x70000000ul,   //(?) processor-
  DT_HIPROC   = 0x7ffffffful,   //(?)           specific
//
  DT_AUXILIARY    = 0x7fffffful,
  DT_USED         = 0x7ffffffeul,
  DT_FILTER       = 0x7ffffffful
#endif
};

//===============================elf64 types=============================
typedef struct {
  uint8     e_ident[EI_NIDENT]; // see above
  int16     e_type;
  uint16    e_machine;
  int32     e_version;
  uint64    e_entry;          // Entry point virtual address
  uint64    e_phoff;          // Program header table file offset
  uint64    e_shoff;          // Section header table file offset
  int32     e_flags;
  int16     e_ehsize;
  int16     e_phentsize;
  int16     e_phnum;
  int16     e_shentsize;
  int16     e_shnum;
  int16     e_shstrndx;
} Elf64_Ehdr;


typedef struct {
  uint32    sh_name;      // Section name, index in string tbl
  uint32    sh_type;      // Type of section
  uint64    sh_flags;     // Miscellaneous section attributes
  uint64    sh_addr;      // Section virtual addr at execution
  uint64    sh_offset;    // Section file offset
  uint64    sh_size;      // Size of section in bytes
  uint32    sh_link;      // Index of another section
  uint32    sh_info;      // Additional section information
  uint64    sh_addralign; // Section alignment
  uint64    sh_entsize;   // Entry size if section holds table
} Elf64_Shdr;

//
typedef struct {
  uint32    st_name;    // Symbol name, index in string tbl
  uint8     st_info;    // Type and binding attributes
  uint8     st_other;   // No defined meaning, 0
  uint16    st_shndx;   // Associated section index
  uint64    st_value;   // Value of the symbol
  uint64    st_size;    // Associated symbol size
} Elf64_Sym;


typedef struct {
  uint64    r_offset;  // Location at which to apply the action
  uint64    r_info;    // index and type of relocation
} Elf64_Rel;


typedef struct {
  uint64    r_offset;    // Location at which to apply the action
  uint64    r_info;      // index and type of relocation
   int64    r_addend;    // Constant addend used to compute value
} Elf64_Rela;


//#define ELF64_R_SYM(i)	   ((i) >> 32)
//#define ELF64_R_TYPE(i)    ((i) & 0xffffffff)
//#define ELF64_R_INFO(s,t)  (((bfd_vma) (s) << 32) + (bfd_vma) (t))
#define ELF64_R_SYM(i)     high(i)
#define ELF64_R_TYPE(i)    low(i)


typedef struct {
  int32     p_type;
  int32     p_flags;
  uint64    p_offset;   // Segment file offset
  uint64    p_vaddr;    // Segment virtual address
  uint64    p_paddr;    // Segment physical address
  uint64    p_filesz;   // Segment size in file
  uint64    p_memsz;    // Segment size in memory
  uint64    p_align;    // Segment alignment, file & memory
} Elf64_Phdr;

typedef struct
{
  uint64 d_tag;   // entry tag value
  uint64 d_un;
} Elf64_Dyn;
//extern Elf64_Dyn _DYNAMIC[];

#endif // __ELFBASE_H__
