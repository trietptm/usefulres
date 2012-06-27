/*
 *      Interactive disassembler (IDA).
 *      Version 4.20
 *      Copyright (c) 2002 by Ilfak Guilfanov. (ig@datarescue.com)
 *      ALL RIGHTS RESERVED.
 *
 */

//
//      Intel OMF386
//

#ifndef INTELOMF_HPP
#define INTELOMF_HPP

#pragma pack(push, 1)

#define INTELOMF_MAGIC_BYTE     0xB0    // The first byte of the file
                                        // must have this value

//-----------------------------------------------------------------------
// Linkable Module Header
// A linkable object file contains one or more linkable modules
//-----------------------------------------------------------------------
struct lmh  /* linkable module header */
{
  ulong tot_length;     /* total length of the module on disk in bytes */
  short num_segs;       /* number of SEGDEF sections in the module */
  short num_gates;      /* number of GATDEF sections in the module */
  short num_publics;    /* number of PUBDEF sections in the module */
  short num_externals;  /* number of EXTDEF sections in the module */
  char linked;          /* linked = 0, if the module was produced by a translator */
  char date[8];         /* the creation date, written in the form MM/DD/YY */
  char time[8];         /* the creation time, written in the form HH:MM:SS */
  char mod_name[41];    /* name of the module, the first char is the string's length */
  char creator[41];     /* the name of the program which created the module */
  char src_path[46];    /* the path to the source file which produced the module */
  char trans_id;        /* translator id, mainly for debugger */
  char trans_vers[4];   /* translator version (ASCII) */
  char OMF_vers;        /* OMF version */
};

//-----------------------------------------------------------------------
struct toc_p1      /* Table of contents for first partition */
{
  long SEGDEF_loc; /* all the following _loc represents location of the first byte */
  long SEGDEF_len; /* of the section in current module, unit is byte; */
  long GATDEF_loc; /* all the following _len represents the length of the section */
  long GATDEF_len; /* also the unit is byte. */
  long TYPDEF_loc;
  long TYPDEF_len;
  long PUBDEF_loc;
  long PUBDEF_len;
  long EXTDEF_loc;
  long EXTDEF_len;
  long TXTFIX_loc;
  long TXTFIX_len;
  long REGINT_loc;
  long REGINT_len;
  long next_partition;
  long reserved;
};

//-----------------------------------------------------------------------
struct segdef  /* segment definition */
{
  short attributes;   /* need to be separated into bits to get bitwise info(cf. [1]) */
  long slimit;      /* the length of the segment minus one, in bytes */
  long dlength;     /* the number of data bytes in the segment, only for dsc seg*/
  long speclength;  /* the total number of bytes in the segment */
  short ldt_position; /* the position in LDT that this segment must occupy */
  char align;       /* alignment requirements of the segment */
  char combine_name[41]; /* first char is the length of the string in byte,
                       rest is name */
};

//-----------------------------------------------------------------------
// The GATDEF section defines an entry for each gate occurring in the module.
// There is a 1-byte field in the data structure which is used to identify type
// of gate from call gate, task gate, interrupt gate or trap gate. (cf. [1])

struct gatdef     /* Gate definition */
{
  char privilege; /* privilege of gate */
  char present;
  char gate_type;
  long GA_offset; /* gate entry GA consists of GA_offset and GA_segment */
  short GA_segment;
};

//-----------------------------------------------------------------------
// The TYPDEF section serves two purposes: to allow Relocation and Linkage
// software to check the validity of sharing data across external linkages,
// and to provide type information to debuggers to interpret data correct.
// [2] provides storage size equivalence tables and lists the syntactical
// constructs for high level languages PL/M, PASCAL, FORTRAN and C.

struct leaf
{
  char type;   /* an 8-bit number defines the type of the leaf */
  union        /* following are different kind of leaves */
  {
    char *string;
    short num_2;
    long num_4;
#ifdef __WATCOMC__
    uchar num_8[8];
    uchar s_8[8];
#else
    ulonglong num_8;
    longlong s_8;
#endif
    signed short s_2;
    signed long s_4;
  } content;
  struct leaf *next; /* points to next leaf */
};

struct typdef     /* type definition */
{
  char linkage;   /* is TRUE, if for public-external linkage; is FALSE, if only for debug symbols. */
  short length;   /* the length in bytes of all the leaves in it */
  struct leaf leaves; /* all different leaves format */
};

//-----------------------------------------------------------------------
// PUBDEF section contains a list of public names with their general
// addresses for the public symbols. The 2-byte field type_IN specifies
// an internal name for a segment, gate, GDT selector or the special
// CONST$IN. This section serves to define symbols to be exported to
// other modules.

struct pubdef   /* public definition */
{
  long PUB_offset; /* gen addr consists of PUB_offset and PUB_segment */
  short PUB_segment;
  short type_IN; /* internal name for the type of the public of symbol */
  char wordcount; /* the total # of 16-bit entities of stacked parameters */
  char sym_name[256];
};

//-----------------------------------------------------------------------
// EXTDEF section lists all external symbols, which are then referenced
// elsewhere in the module by means of their internal name. The 2-byte
// field seg_IN specifies the segment that is assumed to contain the
// matching public symbol and the 2-byte value of type_IN defines the
// type of the external symbol. (cf. [1])

struct extdef    /* external definition */
{
  short seg_IN;  /* internal name of segment having matched public symbol */
  short type_IN; /* internal name for the type of the external symbol */
  char allocate; /* not zero, if R&L needs allocate space for external symbol*/
  union
  {
    short len_2;
    long len_4;
  } allocate_len; /* number of bytes needed allocated for the external symbol */
  char sym_name[256]; /* the 1st char is length , the rest are name of the symbol*/
};

//-----------------------------------------------------------------------
// text block contains binaries for code segment and data segment.
// These segments are relocatable. Other than that, all the SLD information
// is also implemented in this block by a translator under debug option.
// Segment MODULES in the text block is designed with the purpose of
// providing general information about the current module. Segment MBOLS
// provides entries for each symbol used in the module, including stack
// symbols, local symbols and symbols that are used as procedure or block
// start entries. Segment LINES consists of line offset values, each line
// offset is the byte offset of the start of a line in the code segment.
// Segment SRCLINES consists of line offsets of the source files.

struct mod       /* MODULES segment */
{
  short ldt_sel;       /* a selector into the GDT for an LDT which contains the segments in this module */
  long code_offset;   /* code segment GA consists of code_offset and code_IN */
  short code_IN;
  long types_offset;  /* TYPES GA consists of types_offset and types_IN */
  short types_IN;
  long sym_offset;    /* MBOLS GA consists of sym_coffset and sym_IN */
  short sym_IN;
  long lines_offset;  /* LINES GA consists of lines_offset and lines_IN */
  short lines_IN;
  long pub_offset;    /* PUBLICS GA consists of pub_offset and pub_IN */
  short pub_IN;
  long ext_offset;    /* EXTERNAL GA consists of ext_offset and ext_IN */
  short ext_IN;
  long src_offset;    /* SRCLINES GA consists of src_offset and src_IN */
  short src_IN;
  short first_line;   /* first line number */
  char kind;          /* 0 value for 286, 1 value for 386 format */
  char trans_id;      /* same as lmh */
  char trans_vers[4]; /* same as lmh */
  char *mod_name;     /* same as lmh */
};

struct blk          /* block start entry */
{
  long offset;      /* offset in code segment */
  long blk_len;     /* block length */
  char *blk_name;   /* block name, note that first byte is the length of string */
};

struct proc         /* procedure start entry */
{
  long offset;      /* offset in code segment */
  short type_IN;      /* internal name of the typdef associated with the proc */
  char kind;        /* specifying 16-bit or 32-bit */
  long ebp_offset;  /* offset of return address from EBP */
  long proc_len;    /* procedure length */
  char *proc_name;  /* procedure name, as always, the 1st char is string length */
};

struct sbase        /* symbol base entry */
{
  long offset;
  short s_IN;
};

struct symbol       /* symbol entry */
{
  long offset;
  short type_IN;
  char *sym_name;
};

struct sym          /* MBOLS segment */
{
  char kind;        /* kind of entries */
  union
  {
    struct blk blk_start;  /* block start entry */
    struct proc prc_start; /* procedure start entry */
    struct sbase sym_base; /* symbol base entry */
    struct symbol s_ent;   /* symbol entry */
  } entry;
  struct sym *next;
};

struct line         /* LINES segment */
{
  long offset;
  struct lines *next;
};

struct src          /* SRCLINES segment */
{
  char *src_file;   /* source file name */
  short count;
  struct lines *src_line;
  struct srclines *next;
};

struct text             /* text block */
{
  long txt_offset;      /* gen addr consists of txt_offset and txt_IN */
  short txt_IN;           /* internal segment name */
  long length;          /* the length of the text content, in byte */
  union
  {
    char *code;         /* CODE segment */
    char *data;         /* DATA segment */
    struct mod modules; /* MODULES segment */
    struct sym symbols; /* MBOLS segment */
    struct line lines;  /* LINES segment */
    struct src srclines;/* SRCLINES segment */
  } segment;
};

//-----------------------------------------------------------------------
// block contains information that allows the binder or linker to resolve
// (fix up) and eventually relocate references between object modules.
// The attributes where_IN and where_offset in the following data structures
// make a generalized address specifying the target for the fixup. Similarly,
// the attributes what_IN and what_offset make a generalized address
// specifying the target to which the fixup is to be applied.

// There are four kinds of fixups for Intel linkable object modules.
// They are:
//      general fixup,
//      intra-segment fixup,
//      call fixup
//      addition fixup.
// The general fixup and the addition fixup have the same data structure,
// both provide general addresses for where_IN, where_offset, and what_IN,
// what_offset. The intra-segment fixup is equivalent to a general fixup
// with what_IN = where_IN, and the call fixup is also equivalent to a
// general fixup with what_offset = 0. (cf. [1])

struct gen        /* for general fixup */
{
  char kind;      /* specifying the kind of fixup */
  union
  {
    short num2;
    long num4;
  } where_offset; /* 2- or 4- byte where_offset */
  union
  {
    short num2;
    long num4;
  } what_offset;  /* 2- or 4- byte what_offset */
  short what_IN;    /* what_IN & what_offset specify the target for the fixup*/
  union fixups *next;
};

struct intra      /* for intra-segment fixup */
{
  char kind;      /* specifying the kind of fixup */
  union
  {
    short num2;
    long num4;
  } where_offset; /* 2- or 4- byte where_offset */
  union
  {
    short num2;
    long num4;
  } what_offset;  /* 2- or 4- byte what_offset */
  union fixups *next;
};

struct cal        /* for call fixup */
{
  char kind;      /* specifying the kind of fixup */
  union
  {
    short num2;
    long num4;
  } where_offset; /* 2- or 4- byte where-offset */
  short what_IN;
  union fixups *next;
};

struct ad         /* for addition fixup */
{
  char kind;      /* specifying the kind of fixup */
  union
  {
    short num2;
    long num4;
  } where_offset; /* specifying the target to which the fixup is to be applied */
  union
  {
    short num2;
    long num4;
  } what_offset;
  short what_IN;
  union fixups *next;
};

struct temp       /* for the text template in the iterated text block */
{
  long length;    /* the length, in bytes, of a single mem blk to be initialized */
  char *value;    /* the text or data to be used to initialize any single mem blk*/
};

struct iterat       /* for iterated text block */
{
  long it_offset;
  short it_segment; /* above two specify a gen addr to put 1st byte of the text */
  long it_count;    /* the # of times the text template is to be repeated */
  struct temp text; /* the text template */
};

struct fixup    /* fixup block */
{
  short where_IN; /* specifying the segment to which fixups should be applied*/
  short length;   /* the length in bytes of the fixups */
  union
  {
    struct gen general;  /* for general fixup */
    struct intra in_seg; /* for intra-segment fixup */
    struct cal call_fix; /* call fixup */
    struct ad addition;  /* addition fixup */
  } fixups;
};

//-----------------------------------------------------------------------
// The TXTFIX section consists of intermixed text block, fixup block and
// iterated text block. As one can see, it is the TXTFIX section that
// records the binaries for machine codes, initialized data and
// uninitialized data. TXTFIX section output by a translator under debug
// option will also contain SLD information.

struct txtfix           /* text, iterated text and fixup block */
{
  char blk_type;        /* 0 for text blk; 1 for fixup blk and 2 for iterated text blk */
  union
  {
    struct text text_blk; /* text block */
    struct fixup fixup_blk; /* fixup block */
    struct iterat it_text_blk; /* iterated text block */
  } block;
  struct txtfix *next;
};

// The file ends with a checksum byte

#pragma pack(pop)
#endif
