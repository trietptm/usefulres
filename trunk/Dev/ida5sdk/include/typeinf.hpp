/*
 *      Interactive disassembler (IDA)
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *                        E-mail: ig@datarescue.com
 *      Type Information.
 *      Copyright (c) 1995-2006 by Iouri Kharon.
 *                        E-mail: yjh@styx.cabel.net
 *
 *      ALL RIGHTS RESERVED.
 *
 */

#ifndef _TYPEINF_HPP
#define _TYPEINF_HPP
#include <idp.hpp>
#include <name.hpp>
#pragma pack(push, 1)
//
// This file describes the type information records in IDA
//
// The type information is kept as an array of bytes terminated by 0.
// (we chose this format to be able to use string functions on them)
//
// Numbers used in the type declarations are encoded so that no zero
// bytes will appear in the type string. We use the following encodings:
//
//       nbytes  get function    set function   value range                comment
//
//   db  1       x = *ptr++;     *ptr++ = x;    1-0xFF                     very small nonzeros
//   dt  1..2    get_dt          set_dt         0-0xFFFE                   16bit numbers
//   da  1..9    get_da          set_da         0-0x7FFFFFFF, 0-0xFFFFFFFF arrays
//   de  1..5    get_de          set_de         0-0xFFFFFFFF               enum deltas
//
// p_string: below we use p_string type. p_string is a pascal-like string:
//                   dt length, db characters
// p_list:   one or more p_strings concatenated make a p_list
//
// Items in brackets [] are optional and sometimes are omitted.
// type_t... means a sequence type_t bytes which defines a type.

// NOTE: to work with the types of instructions or data in the database,
// use get/set_ti() and similar functions.
//
// The type string has been designed by Yury Haron <yjh@styx.cabel.net>

typedef uchar type_t;
typedef uchar p_string;   // pascal-like string: dt length, characters
typedef uchar p_list;     // several p_strings
struct til_t;             // type information library
class lexer_t;            // lexical analyzer

//------------------------------------------------------------------------
#define RESERVED_BYTE 0xFF  // multifunctional purpose
//------------------------------------------------------------------------
const type_t TYPE_BASE_MASK  = 0x0F;  // the low 4 bits define the basic type
const type_t TYPE_FLAGS_MASK = 0x30;  // type flags (they have different
                                      // meaning for each basic type)
const type_t TYPE_MODIF_MASK = 0xC0;  // modifiers
                                      // for BT_ARRAY see ATT3 below
                                      // BT_VOID can have them ONLY in 'void *'

const type_t TYPE_FULL_MASK = (TYPE_BASE_MASK | TYPE_FLAGS_MASK);

//----------------------------------------
// BASIC TYPES: unknown & void
const type_t  BT_UNK         = 0x00;    // unknown
const type_t  BT_VOID        = 0x01;    // void
// ATT1: BT_UNK and BT_VOID with non-zero type flags can be used in function
// (and struct) declarations to describe the function arguments or structure
// fields if only their size is known. They may be used in ida to describe
// the user input. For struct used also as 'single-field-alignment-suffix'
// [__declspec(align(x))] with TYPE_MODIF_MASK == TYPE_FULL_MASK.
const type_t    BTMT_SIZE0   = 0x00;    // BT_VOID - normal void; BT_UNK - don't use
const type_t    BTMT_SIZE12  = 0x10;    // size = 1  byte  if BT_VOID; 2 if BT_UNK
const type_t    BTMT_SIZE48  = 0x20;    // size = 4  bytes if BT_VOID; 8 if BT_UNK
const type_t    BTMT_SIZE128 = 0x30;    // size = 16 bytes if BT_VOID; unknown if BT_UNK
                                        // (IN struct alignment - see below)

// convenience definitions of unknown types:
const type_t BT_UNK_BYTE  = (BT_VOID | BTMT_SIZE12);   // 1 byte
const type_t BT_UNK_WORD  = (BT_UNK  | BTMT_SIZE12);   // 2 bytes
const type_t BT_UNK_DWORD = (BT_VOID | BTMT_SIZE48);   // 4 bytes
const type_t BT_UNK_QWORD = (BT_UNK  | BTMT_SIZE48);   // 8 bytes
const type_t BT_UNK_QDWRD = (BT_VOID | BTMT_SIZE128);  // 16 bytes
const type_t BT_UNKNOWN   = (BT_UNK  | BTMT_SIZE128);  // unknown size - for parameters

const type_t BTF_VOID = (BT_VOID | BTMT_SIZE0);

//----------------------------------------
// BASIC TYPES: integers
const type_t  BT_INT8        = 0x02;    // __int8
const type_t  BT_INT16       = 0x03;    // __int16
const type_t  BT_INT32       = 0x04;    // __int32
const type_t  BT_INT64       = 0x05;    // __int64
const type_t  BT_INT128      = 0x06;    // __int128 (for alpha & future use)
const type_t  BT_INT         = 0x07;    // natural int. (size provided by idp module)
const type_t    BTMT_UNKSIGN = 0x00;    // unknown signness
const type_t    BTMT_SIGNED  = 0x10;    // signed
const type_t    BTMT_USIGNED = 0x20;    // unsigned
const type_t    BTMT_CHAR    = 0x30;    // BT_INT8:          char
                                        // BT_INT:           segment register
                                        // others BT_INT(x): don't use

// convenience definition:
const type_t BT_SEGREG    = (BT_INT | BTMT_CHAR);      // segment register

//----------------------------------------
// BASIC TYPE: bool
const type_t  BT_BOOL        = 0x08;    // bool
const type_t    BTMT_DEFBOOL = 0x00;    // size is model specific or unknown(?)
const type_t    BTMT_BOOL1   = 0x10;    // size 1byte
const type_t    BTMT_BOOL2   = 0x20;    // size 2bytes
const type_t    BTMT_BOOL4   = 0x30;    // size 4bytes

//----------------------------------------
// BASIC TYPE: float
const type_t  BT_FLOAT       = 0x09;    // float
const type_t    BTMT_FLOAT   = 0x00;    // float (4 bytes)
const type_t    BTMT_DOUBLE  = 0x10;    // double (8 bytes)
const type_t    BTMT_LNGDBL  = 0x20;    // long double (for x86 10 byte)
const type_t    BTMT_SHRTFLT = 0x30;    // short float (2 bytes)



const type_t _BT_LAST_BASIC  = BT_FLOAT; // the last basic type

//----------------------------------------
// DERIVED TYPE: pointer
const type_t  BT_PTR         = 0x0A;    // *
                                        // has the following format:
                                        // [db sizeof(ptr)]; type_t...
// ATT2: pointers to undeclared yet BT_COMPLEX types are prohibited.
const type_t    BTMT_DEFPTR  = 0x00;    // default for model
const type_t    BTMT_NEAR    = 0x10;    // near
const type_t    BTMT_FAR     = 0x20;    // far
const type_t    BTMT_CLOSURE = 0x30;    // if ptr to BT_FUNC - __closure
                                        // in this case next byte MUST be
                                        // RESERVED_BYTE, and after it - BT_FUNC
                                        // else the next byte contains sizeof(ptr)
                                        // allowed values are 1-ph.max_ptr_size.
                                        // if value is bigger than ph.max_ptr_size
                                        // based_ptr_name_and_size() is called
                                        // (see below) to find out the typeinfo

//----------------------------------------
// DERIVED TYPE: array
const type_t  BT_ARRAY       = 0x0B;    // []
// ATT3: for BT_ARRAY the BTMT_... flags must be equivalent to BTMT_... of elements
const type_t    BTMT_NONBASED= 0x10;    // if set
                                        //    array base==0
                                        //    format: dt num_elem; type_t...
                                        //    if num_elem==0 then the array size is unknown
                                        // else
                                        //    format: da num_elem, base; type_t...
const type_t    BTMT_ARRESERV= 0x20;    // reserved bit


//----------------------------------------
// DERIVED TYPE: function
const type_t  BT_FUNC        = 0x0C;    // ()
                                        // format:
        //  cm_t... additional flags;
        //  type_t... return type;
        //  [argloc_t of returned value (if CM_CC_SPECIAL)];
        //  if !CM_CC_VOIDARG:
        //    dt N (N=number of parameters)
        //    if ( N == 0 )
        //      if CM_CC_ELLIPSIS
        //        func(...)
        //      else
        //        parameters are unknown
        //    else
        //      N records:
        //        type_t... (i.e. type of each parameter)
        //        [argloc_t (if CM_CC_SPECIAL)] (i.e. place of each parameter)

// Ellipsis is not taken into account in the number of parameters
// ATT4: the return type can not be BT_ARRAY or BT_FUNC

const type_t    BTMT_DEFCALL  = 0x00;   // call method - default for model or unknown
const type_t    BTMT_NEARCALL = 0x10;   // function returns by retn
const type_t    BTMT_FARCALL  = 0x20;   // function returns by retf
const type_t    BTMT_INTCALL  = 0x30;   // function returns by iret
                                        // in this case cc MUST be 'unknown'

//----------------------------------------
// DERIVED TYPE: complex types
const type_t  BT_COMPLEX     = 0x0D;    // struct/union/enum/typedef
                                        // format:
                                        //   [dt N (N=field count) if !BTMT_TYPEDEF]
                                        //   if N == 0:
                                        //     p_string name (unnamed types have names "anon_...")
                                        //   else
const type_t    BTMT_STRUCT  = 0x00;    //     struct:
                                        //       (N >> 3) records: type_t...
const type_t    BTMT_UNION   = 0x10;    //     union:
                                        //       (N >> 3) records: type_t...
                                        // for STRUCT & UNION (N & 7) - alignment
                                        // if(!(N & 7)) get_default_align()
                                        // else         (1 << ((N & 7) - 1))
                                        // ATTENTION: if(N>>3) == 0 - error
                                        // NOTE: for struct any type may be
                                        //       postfixed with sdacl-byte
const type_t    BTMT_ENUM    = 0x20;    //     enum:
                                        //       next byte bte_t (see below)
                                        //       N records: de delta(s)
                                        //                  OR
                                        //                  blocks (see below)
const type_t    BTMT_TYPEDEF = 0x30;    // named reference
                                        //   always p_string name

const type_t BT_BITFIELD     = 0x0E;    //bitfield (only in struct)
                                        //['bitmasked' enum see below]
                                        // next byte is dt
                                        //  ((size in bits << 1) | (unsigned ? 1 : 0))
const type_t BTMT_BFLDINT   = 0x00;     // int
const type_t BTMT_BFLDCHAR  = 0x10;     // char
const type_t BTMT_BFLDSHORT = 0x20;     // __int16
const type_t BTMT_BFLDLONG  = 0x30;     // __int32


const type_t BT_RESERVED     = 0x0F;        //RESERVED


const type_t BTF_STRUCT  = (BT_COMPLEX | BTMT_STRUCT);
const type_t BTF_UNION   = (BT_COMPLEX | BTMT_UNION);
const type_t BTF_ENUM    = (BT_COMPLEX | BTMT_ENUM);
const type_t BTF_TYPEDEF = (BT_COMPLEX | BTMT_TYPEDEF);
//------------------------------------------------------------------------
// TYPE MODIFIERS:

const type_t  BTM_CONST      = 0x40;    // const
const type_t  BTM_VOLATILE   = 0x80;    // volatile

//------------------------------------------------------------------------
// special enum definitions
typedef uchar bte_t;

const bte_t   BTE_SIZE_MASK = 0x07;   // storage size
                                        // if == 0 get_default_enum_size()
                                        // else 1 << (n -1) = 1,2,4...64
const bte_t   BTE_RESERVED    = 0x08; // reserved for future use
const bte_t   BTE_BITFIELD    = 0x10; // 'subarrays'. In this case ANY record
                                      // has the following format:
                                      //   'de' mask (has name)
                                      //   'dt' cnt
                                      //   cnt records of 'de' values
                                      //      (cnt CAN be 0)
                                      // ATT: delta for ALL subsegment is ONE
const bte_t   BTE_OUT_MASK  = 0x60;   // ouput style mask
const bte_t   BTE_HEX         = 0x00; // hex
const bte_t   BTE_CHAR        = 0x20; // char or hex
const bte_t   BTE_SDEC        = 0x40; // signed decimal
const bte_t   BTE_UDEC        = 0x60; // unsigned decimal
const bte_t   BTE_ALWAYS    = 0x80;   // this bit MUST be present

//------------------------------------------------------------------------
// convenience functions:

inline bool is_type_const(type_t t)   { return (t & BTM_CONST) != 0; }
inline bool is_type_volatile(type_t t){ return (t & BTM_VOLATILE) != 0; }

inline type_t get_base_type(type_t t) { return (t & TYPE_BASE_MASK); }
inline type_t get_type_flags(type_t t){ return (t & TYPE_FLAGS_MASK); }
inline type_t get_full_type(type_t t) { return (t & TYPE_FULL_MASK); }

// is the type_t the last byte of type declaration?
// (there is no additional bytes after a basic type)
inline bool is_typeid_last(type_t t)  { return(get_base_type(t) <= _BT_LAST_BASIC); }

inline bool is_type_unknown(type_t t) { return(get_base_type(t) == BT_UNKNOWN); }
inline bool is_type_void(type_t t)    { return(get_base_type(t) == BT_VOID); }
inline bool is_type_voiddef(type_t t) { return(get_full_type(t) == BTF_VOID); }

inline bool is_type_ptr(type_t t)     { return(get_base_type(t) == BT_PTR); }
inline bool is_type_complex(type_t t) { return(get_base_type(t) == BT_COMPLEX); }
inline bool is_type_func(type_t t)    { return(get_base_type(t) == BT_FUNC); }
inline bool is_type_array(type_t t)   { return(get_base_type(t) == BT_ARRAY); }

inline bool is_type_struct(type_t t)  { return(get_full_type(t) == BTF_STRUCT); }
inline bool is_type_union(type_t t)   { return(get_full_type(t) == BTF_UNION); }
inline bool is_type_enum(type_t t)    { return(get_full_type(t) == BTF_ENUM); }
inline bool is_type_typedef(type_t t) { return(get_full_type(t) == BTF_TYPEDEF); }

inline bool is_type_bitfld(type_t t)  { return(get_base_type(t) == BT_BITFIELD); }

inline bool is_type_int64(const type_t t)
{
  return get_full_type(t) == (BT_INT64|BTMT_UNKSIGN)
      || get_full_type(t) == (BT_INT64|BTMT_SIGNED);
}

inline bool is_type_long(const type_t t)
{
  return get_full_type(t) == (BT_INT32|BTMT_UNKSIGN)
      || get_full_type(t) == (BT_INT32|BTMT_SIGNED);
}

inline bool is_type_short(const type_t t)
{
  return get_full_type(t) == (BT_INT16|BTMT_UNKSIGN)
      || get_full_type(t) == (BT_INT16|BTMT_SIGNED);
}

inline bool is_type_char(const type_t t) // chars are signed by default(?)
{
  return get_full_type(t) == (BT_INT8|BTMT_CHAR)
      || get_full_type(t) == (BT_INT8|BTMT_SIGNED);
}

inline bool is_type_uint(const type_t t)   { return get_full_type(t) == (BT_INT|BTMT_USIGNED); }
inline bool is_type_uchar(const type_t t)  { return get_full_type(t) == (BT_INT8|BTMT_USIGNED); }
inline bool is_type_ushort(const type_t t) { return get_full_type(t) == (BT_INT16|BTMT_USIGNED); }
inline bool is_type_ulong(const type_t t)  { return get_full_type(t) == (BT_INT32|BTMT_USIGNED); }
inline bool is_type_uint64(const type_t t) { return get_full_type(t) == (BT_INT64|BTMT_USIGNED); }
inline bool is_type_ldouble(const type_t t){ return get_full_type(t) == (BT_FLOAT|BTMT_LNGDBL); }
inline bool is_type_double(const type_t t) { return get_full_type(t) == (BT_FLOAT|BTMT_DOUBLE); }
inline bool is_type_float(const type_t t)  { return get_full_type(t) == (BT_FLOAT|BTMT_FLOAT); }
inline bool is_type_floating(const type_t t){return get_base_type(t) == BT_FLOAT; }
inline bool is_type_bool(const type_t t)   { return get_base_type(t) == BT_BOOL; }

// function used ONLY within structures for sdacl-suffix: __declspec(align(x))
// if FIRST type byte is sdacl - ALL structure have sdacl-extension
inline bool is_type_sdacl(type_t t)
    { return(((t & ~TYPE_FLAGS_MASK) ^ TYPE_MODIF_MASK) <= BT_VOID); }
inline int sdacl_unpack(type_t t)
    { return(((t & TYPE_FLAGS_MASK) >> 3) | (t & 1)); }
inline int sdacl_pack(int algn)  // param<=MAX_SDACL_VALUE (MUST be checked before call)
    { return((((algn & 6) << 3) | (algn & 1)) | TYPE_MODIF_MASK); }
#define MAX_DECL_ALIGN   7

//---------------------------------------------------------------------------
// store argloc_t for CM_CC_SPECIAL
idaman type_t *ida_export set_argloc(type_t *pt, int reg, int reghi=-1, bool ret=false);

//-------------------------
// FUNCTIONS TO WORK WITH NUMBERS
//
// store 1-2 byte number. (0-32766)
idaman type_t *ida_export set_dt(type_t *pt, int value);

#define MAX_DT  0x7FFE


// store 2 long values (9 bytes) num_el=0-0x7FFFFFFF, base=0-0xFFFFFFFF
idaman type_t *ida_export set_da(type_t *pt, ulong num_el, ulong base = 0);


// store 1-5 byte number - for enum.
// usage:
//         pt = set_de(buff, val[0]);
//         for(int i = 1; i < valcnt; i++)
//                     pt = set_de(pt, val[i]-val[i-1]);
idaman type_t *ida_export set_de(type_t *pt, ulong val);


// functions to retrieve numbers:
idaman int  ida_export get_dt(const type_t * &pt);                             // returns < 0 - error
idaman bool ida_export get_da(const type_t * &pt, ulong *num_el, ulong *base); // returns false - error
idaman bool ida_export get_de(const type_t * &pt, ulong *val);                 // returns false - error

//------------------------------------------------------------------------
inline const type_t *skip_ptr_type_header(const type_t *type)
{
  if ( get_type_flags(*type++) == BTMT_CLOSURE
    && *type++ != RESERVED_BYTE ) type++; // skip reserved byte (and possibly sizeof(ptr))
  return type;
}

inline const type_t *skip_array_type_header(const type_t *type)
{
  if ( get_type_flags(*type++) & BTMT_NONBASED )
  {
    int n = get_dt(type);
    if ( n < 0 )
      type = NULL;
  }
  else
  {
    ulong num, base;
    if ( !get_da(type, &num, &base) )
      type = NULL;
  }
  return type;
}

#define DEFINE_NONCONST_SKIPPER(typename)                                       \
inline type_t *skip_ ## typename ## _type_header(type_t *type)                  \
  { return (type_t *)skip_ ## typename ## _type_header((const type_t *)type); }

DEFINE_NONCONST_SKIPPER(ptr)
DEFINE_NONCONST_SKIPPER(array)

inline type_t *typend(const type_t *ptr)  { return (type_t *)strchr((char *)ptr, '\0'); }
inline size_t typlen(const type_t *ptr)  { return strlen((const char *)ptr); }
inline type_t *typncpy(type_t *dst, const type_t *src, size_t size)
        { return (type_t *)qstrncpy((char *)dst, (const char *)src, size); }
inline type_t *tppncpy(type_t *dst, const type_t *src, size_t size)
        { return (type_t *)qstpncpy((char *)dst, (const char *)src, size); }
inline int     typcmp(const type_t *dst, const type_t *src)
        { return strcmp((const char *)dst, (const char *)src); }
inline type_t *typdup(const type_t *src)
        { return (type_t *)qstrdup((const char *)src); }

// compare two types for equality (take into account typedefs)
// returns true - types are equal
idaman bool ida_export equal_types(const til_t *ti, const type_t *t1, const type_t *t2);

// resolve typedef recursively if is_type_typedef(*p)
// fields will contains the field list if the type is resolved
idaman const type_t *ida_export resolve_typedef(const til_t *ti, const type_t *p, const p_list **fields=NULL);

idaman bool ida_export is_resolved_type_const  (const type_t *type);
idaman bool ida_export is_resolved_type_void   (const type_t *type);
idaman bool ida_export is_resolved_type_ptr    (const type_t *type);
idaman bool ida_export is_resolved_type_func   (const type_t *type);
idaman bool ida_export is_resolved_type_array  (const type_t *type);
idaman bool ida_export is_resolved_type_complex(const type_t *type);
idaman bool ida_export is_resolved_type_struct (const type_t *type);
idaman bool ida_export is_resolved_type_union  (const type_t *type);
idaman bool ida_export is_resolved_type_enum   (const type_t *type);
idaman bool ida_export is_resolved_type_bitfld (const type_t *type);

idaman bool ida_export is_castable(const type_t *from, const type_t *to);

idaman bool ida_export remove_constness(type_t *type); // remove const and const* modifiers
idaman bool ida_export remove_pointerness(const type_t **ptype, const char **pname);

idaman type_t ida_export get_int_type_bit(int size); // size should be 1,2,4,8,16
idaman type_t ida_export get_unk_type_bit(int size); // size should be 1,2,4,8,16

//------------------------------------------------------------------------
// type names (they can be replaced by ida)
struct _type_names {
  char
        *type_void,       // "void",
// int types
        *type_int8,       // "__int8",
        *type_int16,      // "__int16",
        *type_int32,      // "__int32",
        *type_int64,      // "__int64",
        *type_int128,     // "__int128",
// char if special flag set
        *type_char,       // "char",
// natural int
        *type_int,        // "int",
// any bool type
        *type_bool,       // "bool",
// float types
        *type_float,      // "float",
        *type_double,     // "double",
        *type_longdouble, // "long double",
        *type_shortfloat, // "short float",
// segment register
        *type_seg,        // "__seg",
// unknown input
        *type_unknown,    // "VOID"
// unknown types (only size is known)
        *type_byte,       // "BYTE"     1byte
        *type_word,       // "WORD"     2byte
        *type_dword,      // "DWORD"    8byte
        *type_qword,      // "QWORD"    4byte
        *type_big,        // "LONGLONG" 16byte
// prefixes (ATT5: see spaces!)
        *type_signed,     // "signed ",
        *type_unsigned,   // "unsigned ",
// model declarator for function prototypes
        *cc_cdecl,        // "__cdecl"
        *cc_stdcall,      // "__stdcall"
        *cc_pascal,       // "__pascal"
        *cc_fastcall,     // "__fastcall"
        *cc_thiscall,     // "__thiscall"
        *cc_manual,       // "" - compiler specific: __syscall/__fortran/vxdcall/...
// used for CM_CC_SPECIAL
        *cc_special;      // "__usercall"
};
extern _type_names  tns;


//------------------------------------------------------------------------
// Type Information Library
//------------------------------------------------------------------------

struct til_t
{
  char *name;           // short file name (without path and extension)
  char *desc;           // human readable til description
  int nbases;           // number of base tils
  struct til_t **base;  // tils that our til is based on
  ulong flags;
#define TIL_ZIP 0x0001  // pack buckets using zip
#define TIL_MAC 0x0002  // til has macro table
#define TIL_ESI 0x0004  // extended sizeof info (short, long, longlong)
  compiler_info_t cc;
  struct til_bucket_t *syms;
  struct til_bucket_t *types;
  struct til_bucket_t *macros;
  int nrefs;            // number of references to the til
};


// Initialize a til
idaman til_t *ida_export new_til(const char *name, const char *desc);

// Add a base til
// bases - comma separated list of til names
// returns: !=0-ok, otherwise the error message is in errbuf
int add_base_tils(til_t *ti, const char *tildir, const char *bases, char *errbuf, size_t bufsize);

#define TIL_ADD_FAILED  0
#define TIL_ADD_OK      1       // some tils were added
#define TIL_ADD_ALREADY 2       // the base til was already added


// Load til from a file
// returns: !NULL-ok, otherwise the error message is in errbuf
idaman til_t *ida_export load_til(const char *tildir, const char *name, char *errbuf, size_t bufsize);

// Sort til (use after modifying it)
// returns false - no memory or bad parameter
bool sort_til(til_t *ti);

// Store til to a file
idaman bool ida_export store_til(const til_t *ti, const char *tildir, const char *name);

// Free memory allocated by til
idaman void ida_export free_til(til_t *ti);

// Get human-readable til description
idaman til_t *ida_export load_til_header(const char *tildir, const char *name, char *errbuf, size_t bufsize);


// The following 2 functions are for special use only
void til_add_macro(til_t *ti, const char *name, const char *body, int nargs, bool isfunc);
bool til_next_macro(const til_t *ti, const char **current, const char **body, int *nargs, bool *isfunc);

//------------------------------------------------------------------------
// FUNCTIONS TO WORK WITH TYPE STRINGS


// FUNCTION: Get the type size
//      til - pointer to type information library.
//            if NULL, then the current IDA database is used
//      ptr - pointer to type string
//      lp  - pointer to variable which will get the natural alignment
//            for the type
// if this function returns BADSIZE
//   value of ptr is unknown but it is guaranteed
//   that ptr points somewhere within the string
//   (including the final zero byte)
// else
//   ptr points after the full description of the type.
//
// Hint: this function can be used to skip typeinfo descriptor

idaman size_t ida_export get_type_size(const til_t *ti, const type_t * &ptr, size_t *lp = NULL);
// return: 0 - unknown, BADSIZE error
// this variant of the function doesn't move the pointer:
inline size_t get_type_size0(const til_t *ti, const type_t *ptr, size_t *lp = NULL)
{ return get_type_size(ti, ptr, lp); }


inline const type_t *skip_type(const til_t *ti, const type_t *&ptr)
{
  get_type_size(ti, ptr);
  return ptr;
}

// get size of the object pointed by a pointer or an array element size
// the type should be a pointer or an array
// if error, return BADSIZE

idaman int ida_export get_pointer_object_size(const type_t *type);


const size_t BADSIZE = size_t(-1);


//------------------------------------------------------------------------
// FUNCTION: Unpack type string
// This function generates C/C++ representation of the type string
//      til     - pointer to type information library.
//                if NULL, then the current IDA database is used
//      pt      - pointer to type string
//      cb_func - callback to call for each field/argument.
//                it will be also called once for function/complex type
//      cd_data - data to pass to cb_func
//      name    - name of variable of this type
//      cmt     - a comment for the whole type
//      field_names - field/argument names (used for functions are complex types)
//      field_cmts  - field/argument comments (used for functions are complex types)
//
//             names/comments is fully 'synchronized' when it (or one for him)
//             length is 1 (asciz "") - skipped.
//
// The function will return one of the following codes:
#define T_CBBRKDEF  3   // !cb_func return from 'redefine' call
#define T_NONALL    2   // type string doesn't have a final zero byte.
#define T_CBBRK     1   // !cb_func return
#define T_NORMAL    0   // GOOD
#define T_BADDESCR  -1  // bad type string
#define T_SHORTSTR  -2  // buffer too small, or strlen(answer) > MAXSTR
#define T_BADNAMES  -3  // bad fldNames
#define T_BADCMTS   -4  // bad fldCmts
#define T_PARAMERR  -5  // parameter error
#define T_ALREADY   -6  // type already exists
#define T_NOTYPE    -7  // no such type

struct descr_t
{
  const   p_list *Names; // names for field/param
  const   p_list *Cmts;  // comment for field/param
};

// callback: if returns false - stop unpack_type()
typedef bool (idaapi*tcbfn)(void *cb_data,        // data from the caller
                      int level,                  // structure inclusion level
                      const char *str,            // C representation of type
                      const char *cmt);           // possible comment
   // INTERNAL:
   //           if function called for names [re]difiniton
   //           str   = NULL (for normall call this is not allowed)
   //           cmt   = (const char *)Descr (see below)
   //           level = offset in type_t for current element of type_t
   // ATTENTION:
   //           after *Descr->Names = '\0', Descr->Names set to NULL
   //           after *Descr->Cmts  = '\0', Descr->Cmts set to NULL
   //                    can be used for checks

idaman int ida_export unpack_type(
                const til_t *ti,                  // type information library
                const type_t *pt,                 // type descriptor string
                tcbfn cb_func,                    // callback
                void  *cb_data,                   // data for callback
                const char *name = NULL,          // var/func name
                const char *cmt = NULL,           // main comment
                const descr_t *Descr = NULL,      // field/args names & comments
                unsigned int flags = 0);          // combination of UNPFL_....
#define UNPFL_REDEFINE    0x00000001              // must call cb_func for
                                                  // name definitions too

idaman int ida_export print_type_to_one_line(            // make one-line description
                char  *buf,                       // output buffer
                size_t bufsize,                   // size of the output buffer
                const til_t *ti,                  // type information library
                const type_t *pt,                 // type descriptor string
                const char *name = NULL,          // var/func name
                const char *cmt = NULL,           // main comment
                const p_list *field_names = NULL, // field names
                const p_list *field_cmts = NULL); // field comments

idaman int ida_export print_type_to_many_lines(          // make manyline description
                bool (idaapi*printer)(void *cbdata, const char *buf),
                void *cbdata,                     // callback data
                const char *prefix,               // prefix of each line
                int indent,                       // structure level indent
                int cmtindent,                    // comment indents
                const til_t *ti,                  // type information library
                const type_t *pt,                 // type descriptor string
                const char *name = NULL,          // var/func name
                const char *cmt = NULL,           // main comment
                const p_list *field_names = NULL, // field names
                const p_list *field_cmts = NULL); // field comments

// Get type declaration for the specified address
idaman bool ida_export print_type(ea_t ea, char *buf, size_t bufsize, bool one_line);

// display the type string in its internal form:
void show_type(int (*print_cb)(const char *format, ...),
               const type_t *ptr);
void show_plist(int (*print_cb)(const char *format, ...),
                const char *header,
                const p_list *list);

//=========================================================================
// some examples:
//
// __int8  (*func(void))(__int16 (*)(char*), ...);
//    BT_FUNC | BTMT_DEFCALL, CM_UNKNOWN | CM_M_NN | CM_CC_UNKNOWN,
//      BT_PTR | BTMT_DEFPTR,
//      BT_FUNC | BTMT_DEFCALL, CM_UNKNOWN | CM_M_NN | CM_CC_ELLIPSIS,
//        BT_INT8 | BTMT_UNKSIGN,
//        2,
//        BT_PTR | BTMT_DEFPTR,
//        BT_FUNC | BTMT_DEFCALL, CM_UNKNOWN | CM_M_NN | CM_CC_UNKNOWN,
//          BT_INT16 | BTMT_UNKSIGN,
//          2,
//          BT_PTR | BTMT_DEFPTR,
//          BT_INT8 | BTMT_CHAR,
//      2,
//      BT_VOID,
//    0 // eof

//
//--------------
// __int8 (*funcS[1][2])(__int8(*)[1][2] ,...);
//    BT_FUNC | BTMT_DEFFUNC, CM_UNKNOWN | CM_M_NN | CM_CC_ELLIPSIS,
//      BT_PTR | BTMT_DEFPTR,
//        BT_ARRAY | BTMT_NONBASED,
//          2,
//        BT_ARRAY | BTMT_NONBASED,
//          3,
//        BT_INT8,
//      2,
//        BT_PTR | BTMT_DEFPTR,
//          BT_ARRAY | BTMT_NONBASED,
//            2,
//          BT_ARRAY | BTMT_NONBASED,
//            3,
//          BT_INT8,
//  0 // eof
//------------------------------------------------------------------------
// CM (calling convention & model)

// default size of pointers
const cm_t CM_MASK = 0x03;
const cm_t  CM_UNKNOWN   = 0x00;
const cm_t  CM_N8_F16    = 0x01;  // 1: near 1byte,  far 2bytes
                                  // if sizeof(int)>2 then ptr size is 8bytes
const cm_t  CM_N16_F32   = 0x02;  // 2: near 2bytes, far 4bytes
const cm_t  CM_N32_F48   = 0x03;  // 4: near 4bytes, far 6bytes
// model
const cm_t CM_M_MASK = 0x0C;
const cm_t  CM_M_NN      = 0x00;  // small:   code=near, data=near (or unknown if CM_UNKNOWN)
const cm_t  CM_M_FF      = 0x04;  // large:   code=far, data=far
const cm_t  CM_M_NF      = 0x08;  // compact: code=near, data=far
const cm_t  CM_M_FN      = 0x0C;  // medium:  code=far, data=near
// calling convention
const cm_t CM_CC_MASK = 0xF0;
const cm_t  CM_CC_INVALID  = 0x00;  // this value is invalid
const cm_t  CM_CC_UNKNOWN  = 0x10;  // unknown calling convention
const cm_t  CM_CC_VOIDARG  = 0x20;  // function without arguments
                                    // ATT7: if has other cc and argnum == 0,
                                    // represent as f() - unknown list
const cm_t  CM_CC_CDECL    = 0x30;  // stack
const cm_t  CM_CC_ELLIPSIS = 0x40;  // cdecl + ellipsis
const cm_t  CM_CC_STDCALL  = 0x50;  // stack, purged
const cm_t  CM_CC_PASCAL   = 0x60;  // stack, purged, reverse order of args
const cm_t  CM_CC_FASTCALL = 0x70;  // stack, first args are in regs (compiler-dependent)
const cm_t  CM_CC_THISCALL = 0x80;  // stack, first arg is in reg (compiler-dependent)
const cm_t  CM_CC_MANUAL   = 0x90;  // special case for compiler specific
const cm_t  CM_CC_RESERVE5 = 0xA0;
const cm_t  CM_CC_RESERVE4 = 0xB0;
const cm_t  CM_CC_RESERVE3 = 0xC0;
const cm_t  CM_CC_RESERVE2 = 0xD0;
const cm_t  CM_CC_RESERVE1 = 0xE0;
const cm_t  CM_CC_SPECIAL  = 0xF0;  // locations of all arguments and the return
                                    // value are present in the function declaration.
                                    // The locations are represented by argloc_t:
//
typedef ushort argloc_t;            // location of a function argument
                                    // ATT6: occupies 1 or 2 bytes!
                                    // if argloc_t == 0x80:
                                    //   the argument is in stack
                                    // else
                                    //   in register (argloc_t = regnum + 1)
                                    //   if the argument occupies 2 registers:
                                    //     first byte  = (hireg+1) | 0x80
                                    //     second byte = (loreg+1)
                                    // In ushort form we keep high reg in the most
                                    // significant byte of ushort (i.e. (high+1)<<8)
                                    // if argloc_t == 0x80, it is a stack parameter
//
// standard C-language models for x86
const cm_t C_PC_TINY    = (CM_N16_F32 | CM_M_NN);
const cm_t C_PC_SMALL   = (CM_N16_F32 | CM_M_NN);
const cm_t C_PC_COMPACT = (CM_N16_F32 | CM_M_NF);
const cm_t C_PC_MEDIUM  = (CM_N16_F32 | CM_M_FN);
const cm_t C_PC_LARGE   = (CM_N16_F32 | CM_M_FF);
const cm_t C_PC_HUGE    = (CM_N16_F32 | CM_M_FF);
const cm_t C_PC_FLAT    = (CM_N32_F48 | CM_M_NN);
//
inline cm_t get_cc (cm_t cm) { return(cm & CM_CC_MASK); }

/////////////////////////////////////////////////////////////////////////////
// get size of type which depends on the calling convention & model
// it is called for: BT_BOOL|BTMT_DEFBOOL, BT_INT, BT_PTR (not closure)
// this function is called to determine size of pointers and "int"
//      t         - type to determine the size of
//      cm        - current calling convention
//      isPodePtr - for pointers: is the pointer a code pointer?
// returns: -1  - error (incompatiable type/model)
//           0  - unknown size
//          otherwise returns the size of the type

size_t get_cc_type_size(type_t t, cm_t cm, bool isCodePtr = false);


// CC (compiler)
const comp_t COMP_MASK   = 0x0F;
const comp_t  COMP_UNK     = 0x00;      // Unknown
const comp_t  COMP_MS      = 0x01;      // Visual C++
const comp_t  COMP_BC      = 0x02;      // Borland C++
const comp_t  COMP_WATCOM  = 0x03;      // Watcom C++
//const comp_t  COMP_         = 0x04
//const comp_t  COMP_         = 0x05
const comp_t  COMP_GNU     = 0x06;      // GNU C++
const comp_t  COMP_VISAGE  = 0x07;      // Visual Age C++
const comp_t  COMP_BP      = 0x08;      // Delphi

inline comp_t get_comp(comp_t comp) { return(comp & COMP_MASK); }
idaman const char *ida_export get_compiler_name(comp_t id);

inline comp_t default_compiler(void) { return(inf.cc.id & COMP_MASK); }

//--------------------------------------------------------------------------
#define MAXFUNCARGCMT 64

#if (MAXNAMELEN + MAXFUNCARGCMT + 4) > MAXSTR
#error  "Illegal MAXFUNCARGCMT"
#endif

// extraction from name/comment string arrays

idaman bool ida_export extract_pstr(const p_list * &pt, char *buff, size_t buff_sz);

inline bool extract_name(const p_list *&pt, char *buff)   { return extract_pstr(pt, buff, MAXNAMELEN); }
inline bool skipName(const p_list *&pt)                   { return extract_name(pt, NULL); }
inline bool extract_comment(const p_list *&pt, char *buff){ return extract_pstr(pt, buff, MAXSTR); }
inline bool skipComment(const p_list *&pt)                { return extract_comment(pt, NULL); }
inline bool extract_fargcmt(const p_list *&pt, char *buff){ return extract_pstr(pt, buff, MAXFUNCARGCMT); }
inline void skip_argloc(const type_t *&ptr)               { if ( *ptr++ > 0x80 ) ptr++; }
inline void extract_argloc(const type_t *&ptr, int *p1, int *p2)
{
  type_t high = *ptr++;
  if ( high > 0x80 )
  {
    *p2 = (high & 0x7F) - 1;
    *p1 = *ptr++ - 1;
  }
  else
  {
    *p2 = -1;
    *p1 = int(high & 0x7F) - 1;
  }
}

//--------------------------------------------------------------------------
// CONVERT C/C++ DECLARATION(S) TO type_t*

enum abs_t    { abs_unk, abs_no, abs_yes };     // abstractness of declaration
enum sclass_t                                   // storage class
{
  sc_unk,       // unknown
  sc_type,      // typedef
  sc_ext,       // extern
  sc_stat,      // static
  sc_reg,       // register
  sc_auto,      // auto
  sc_friend,    // friend
  sc_virt       // virtual
};

#define HTI_CPP  0x0001                // C++ mode (not implemented)
#define HTI_INT  0x0002                // debug: print internal representation of types
#define HTI_EXT  0x0004                // debug: print external representation of types
#define HTI_LEX  0x0008                // debug: print tokens
#define HTI_UNP  0x0010                // debug: check the result by unpacking it
#define HTI_TST  0x0020                // test mode: discard the result
#define HTI_FIL  0x0040                // "input" is file name
                                       // otherwise "input" contains a C declaration
#define HTI_MAC  0x0080                // define macros from the base tils
#define HTI_NWR  0x0100                // no warning messages
#define HTI_NER  0x0200                // ignore all errors but display them
#define HTI_DCL  0x0400                // don't complain about redeclarations
#define HTI_NDC  0x0800                // don't decorate names

// this callback will be called for each type/variable declaration
// if it returns T_CBBRKDEF, the type declaration won't be saved in the til
typedef int idaapi h2ti_type_cb(
     const char *name,                 // var/func/type name
     const type_t *type,               // type descriptor string
     const char *cmt,                  // main comment
     const p_list *field_names,        // field names
     const p_list *field_cmts,         // field comments
     const ulong *value);              // symbol value

typedef int printer_t(const char *format, ...);

// convert descriptions to type_t*
// returns number of errors (they are displayed using print_cb)
// zero means ok
// This is a low level function - use parse_... functions below
idaman int ida_export h2ti(
         til_t *ti,
         lexer_t *lx,              // input lexer, may be NULL
                                   // always destroyed by h2ti()
         const char *input,        // file name or C declaration
         int flags=0,              // see HTI_... above
         h2ti_type_cb *type_cb=NULL,    // for each type
         h2ti_type_cb *var_cb=NULL,     // for each var
         printer_t *print_cb=NULL,      // may pass 'msg' here
         void *_cb_data=NULL,
         abs_t _isabs=abs_unk);

void h2ti_warning(void *parser, const char *format, ...);

// Parse ONE declaration
//      decl    - C declaration to parse
//      name    - the declared object name.
//                Allocated in the heap and should be freed by the caller
//      type    - the output type string.
//                Allocated in the heap and should be freed by the caller
//      fields  - the field names.
//                Allocated in the heap and should be freed by the caller
//      flags   - combination of PT_... constants
// NOTE: name & type & fields might be NULL after the call!
// Returns true-ok, false-declaration is bad, the error message is displayed
// If the input string contains more than one declaration, the last one
// is used.

idaman bool ida_export parse_type(const char *decl,
                                  char **name,
                                  type_t **type,
                                  p_list **fields,
                                  int flags=0);
#define PT_SIL       0x0001  // silent, no messages
#define PT_NDC       0x0002  // don't decorate names


// Parse many declarations and store them in the current database
//    input - isfile=true: input file name
//                  =false: C declarations string
//    printer - function to output error messages (use msg or NULL or your own)
// Returns number of errors, 0 means ok

idaman int ida_export parse_types(const char *input, bool isfile, printer_t *printer);


/////////////////////////////////////////////////////////////////////////////
//              WORK WITH NAMED TYPES
/////////////////////////////////////////////////////////////////////////////

// get named typeinfo
//      til       - pointer to type information library
//      name      - name of type
//      flags     - combination of NTF_... flags
//      type      - ptr to ptr to output buffer for the type info
//      fields    - ptr to ptr to the field/args names. may be NULL
//      cmt       - ptr to ptr to the main comment. may be NULL
//      fieldcmts - ptr to ptr to the field/args comments. may be NULL
// if name==NULL returns false
// returns: 0 - can't find the named type
//          1  - ok, the buffers are filled with information (if not NULL)
//          2  - ok, found it in a base til
// the returned pointers are pointers to static storage
// and are valid till free_til(), set_named_type(), del_named_type(), rename_named_type()
// (in other words, until til_t is changed)

idaman int ida_export get_named_type(
                const til_t *ti,
                const char *name,
                int ntf_flags,
                const type_t **type=NULL,
                const p_list **fields=NULL,
                const char **cmt=NULL,
                const p_list **fieldcmts=NULL,
                sclass_t *sclass=NULL,
                ulong *value=NULL);

#define NTF_TYPE     0x0001     // type name
#define NTF_SYMU     0x0008     // symbol, name is unmangled ('func')
#define NTF_SYMM     0x0000     // symbol, name is mangled ('_func')
                                // only one of NTF_TYPE and NTF_SYMU, NTF_SYMM can be used
#define NTF_NOBASE   0x0002     // don't inspect base tils (for get_named_type)
#define NTF_REPLACE  0x0004     // replace original type (for set_named_type)
#define NTF_UMANGLED 0x0008     // name is unmangled (don't use this flag)
#define NTF_NOIDB    0x0010     // don't inspect idb types (for get_named_type)


// set named typeinfo
//      til       - pointer to til.
//      name      - name of type (any ascii string)
//      flags     - combination of NTF_...
//      ptr       - pointer to typeinfo to save
//                  if ptr[0] == '\0' then the type is deleted
// if name==NULL or ptr==NULL returns false
// returns true if successfully saves the typeinfo

idaman bool ida_export set_named_type(
                til_t *ti,
                const char *name,
                int ntf_flags,
                const type_t *ptr,
                const p_list *fields=NULL,
                const char *cmt=NULL,
                const p_list *fieldcmts=NULL,
                const sclass_t *sclass=NULL,
                const ulong *value=NULL);


// get size of the named type
// returns: -1 - error (unknown name)
//           0 - unknown size
//          otherwise returns the size

idaman size_t ida_export get_named_type_size(
                const til_t *ti,
                const char *name,
                int ntf_flags,
                size_t *lp = NULL);


// del information about a symbol
// returns: success

idaman bool ida_export del_named_type(til_t *ti, const char *name, int ntf_flags);


// rename a type or a symbol
// return error code (see T_... constants above)

idaman int ida_export rename_named_type(til_t *ti, const char *from, const char *to, int ntf_flags);


// Enumerate types
// These functions return mangled names

idaman const char *ida_export first_named_type(const til_t *ti, int ntf_flags);
idaman const char *ida_export next_named_type(const til_t *ti, const char *name, int ntf_flags);


// Mangle/unmangle a C symbol name
//      ti        - pointer to til
//      name      - name of symbol
//      type      - type of symbol. If NULL then it will try to guess.
//      outbuf    - output buffer
//      bufsize   - size of the output buffer
//      mangle    - true-mangle, false-unmangle
//      cc        - real calling convention for VOIDARG functions
// returns true if success

inline bool decorate_name(const til_t *ti,
                          const char *name,
                          const type_t *type,
                          char *outbuf,
                          size_t bufsize,
                          bool mangle,
                          cm_t cc=0)
{
  if ( !ph.ti() ) return false;
  return ph.notify(ph.decorate_name, ti, name,
                        type, outbuf, bufsize, mangle, cc) != 0;
}

// Generic function for that (may be used in IDP modules):
idaman bool ida_export gen_decorate_name(
                const til_t *ti,
                const char *name,
                const type_t *type,
                char *outbuf,
                size_t bufsize,
                bool mangle,
                cm_t cc);

// Get undecorated or demangled name, the smallest possible form
//      name - original (mangled or decorated) name
//      type - name type if known, otherwise NULL
//      buf  - output buffer
//      bufsize - output buffer size
// Returns: true-name has been demangled/undecorated
//          false-name is the same as before

idaman bool ida_export calc_bare_name(const char *name,
                                      const type_t *type,
                                      char *buf,
                                      size_t bufsize);


// Choose a type from a type library
//      ti        - pointer to til
//      title     - title of listbox to display
//      ntf_flags - combination of NTF_... flags
//      func      - predicat to select types to display
// returns: NULL-nothing is chosen, otherwise a type name

typedef bool idaapi predicate_t(const char *name, const type_t *type, const p_list *fields);

idaman const char *ida_export choose_named_type(
                const til_t *ti,
                const char *title,
                int ntf_flags,
                predicate_t *func);

//--------------------------------------------------------------------------
// ALIGNMENT

// Get default alignment for structure fields
//      cm - the current calling convention and model
// returns: the default alignment for structure fields
//          (something like 1,2,4,8,...)

inline size_t get_default_align(cm_t) { return inf.cc.defalign; }

inline void align_size(size_t &size, size_t algn)
  { if(size && (int)--algn > 0) size = (size + algn) & ~algn; }

// Get alignment delta for a structure field
//      cur_tot_size - the structure size calculated so far
//      elem_size    - size of the current field
//                     the whole structure should be calculated
//      algn         - the structure alignment (1,2,4,8...)
inline void align_size(size_t &cur_tot_size, size_t elem_size, size_t algn)
    { align_size(cur_tot_size, qmin(elem_size, algn)); }

//--------------------------------------------------------------------------
// enums

// Get sizeof(enum)

inline size_t get_default_enum_size(cm_t cm)
  { return ph.ti() ? ph.notify(ph.get_default_enum_size, cm) : 0; }

//--------------------------------------------------------------------------
// POINTERS

// get maximal pointer size

inline int max_ptr_size(void) { return ph.notify(ph.max_ptr_size)-1; }

// get prefix and size of 'segment based' ptr type (something like char _ss *ptr)
//      ptrt  - the type of pointer to get information about
//              it is calculated as "size - max_ptr_size() - 1"
//      size  - the sizeof of the type will be returned here
// returns: NULL - error (unknown type == bad typeinfo string)
//          else - string in form "_ss",
//                 size contains sizeof of the type
// HINT: the returned value may be an empty string ("")

inline const char *based_ptr_name_and_size(unsigned ptrt, size_t &size)
{
  if ( !ph.ti() ) return NULL;
  char *ptrname;
  size = ph.notify(ph.based_ptr, ptrt, &ptrname);
  return ptrname;
}


// calculate function argument locations
//      type    - pointer to function type string
//      arglocs - the result array
//                each entry in the array will contain an offset from the stack pointer
//                for the first stack argument the offset will be zero
//                register locations are represented like this: the register number combined with ARGLOC_REG
//      maxn    - number of elements in arglocs
// returns: number_of_arguments. -1 means error.
//      type is advanced to the function argument types array

inline int calc_arglocs(const type_t *&type, ulong *arglocs, int maxn)
{
  if ( !ph.ti() || !is_type_func(*type) ) return -1;
  return ph.notify(ph.calc_arglocs, &type, arglocs, maxn);
}

#define ARGLOC_REG      0x80000000L
#define MAX_FUNC_ARGS   256             // max number of function arguments


// Get offset of the first stack argument

inline int get_stkarg_offset(void)
{
  if ( !ph.ti() ) return 0;
  return ph.notify(ph.get_stkarg_offset);
}


// Copy a named type from til to idb
//      idx   - the position of the new type in the list of types (structures or enums)
//              -1 means at the end of the list
//      tname - the type name
// Returns BADNODE - error

idaman tid_t ida_export til2idb(int idx, const char *name);


// Load a til file

idaman int ida_export add_til(const char *name);  // returns one of ADDTIL_... constants
#define ADDTIL_FAILED   0  // something bad, the warning is displayed
#define ADDTIL_OK       1  // ok, til is loaded
#define ADDTIL_COMP     2  // ok, but til is not compatible with the current compiler

// Unload a til file

idaman bool ida_export del_til(const char *name);


// Apply the specified named type to the address
//      ea - linear address
//      name - the type name, e.g. "FILE"
// returns: success

idaman bool ida_export apply_named_type(ea_t ea, const char *name);


// Apply the specified type to the address
//      ea - linear address
//      type - type string in the internal format
//      fields - field names if required by the type string
// This function sets the type and tries to convert the item at the specified
// address to conform the type.
// returns: success

idaman bool ida_export apply_type(ea_t ea, const type_t *type, const p_list *fields);


// Apply the specified type to the address
//      ea - linear address
//      decl - type declaration in C form
// This function parses the declaration and calls apply_type()
// returns: success

idaman bool ida_export apply_cdecl(ea_t ea, const char *decl);


// Apply the type of the called function to the calling instruction
//      caller - linear address of the calling instruction.
//               must belong to a function.
//      type - type string in the internal format
//      fields - field names if required by the type string
// This function will append parameter comments and rename the local
// variables of the calling function.

idaman void ida_export apply_callee_type(ea_t caller, const type_t *type, const p_list *fields);


// Apply the specified type and name to the address
//      ea - linear address
//      type - type string in the internal format
//      name - new name for the address
// This function checks if the address already has a type. If the old type
// does not exist or the new type is 'better' than the old type, then the
// new type will be applied. A type is considere better if it has more
// information (e.g.e BT_STRUCT is better than BT_INT).
// The same logic is with the name: if the address already have a meaningful
// name, it will be preserved. Only if the old name does not exist or it
// is a dummy name like byte_123, it will be replaced by the new name.
// Returns: success

idaman bool ida_export apply_once_type_and_name(ea_t ea, const type_t *type, const char *name);



// To retrieve the type information attach to an address, use get_ti() function
// (see nalt.hpp)

// generate a type string using information about the function
// from the disassembly. you could use guess_type() function instead of this function
idaman int ida_export guess_func_type(func_t *pfn,
                    type_t *type,
                    size_t tsize,
                    p_list *fields,
                    size_t fsize);
#define GUESS_FUNC_FAILED   0   // couldn't guess the function type
#define GUESS_FUNC_TRIVIAL  1   // the function type doesnt' have interesting info
#define GUESS_FUNC_OK       2   // ok, some non-trivial information is gathered


// generate a type string using information about the id from the disassembly
idaman int ida_export guess_type(
                    tid_t id,           // or address
                    type_t *type,
                    size_t tsize,
                    p_list *fields,
                    size_t fsize);

// Various parameters

inline void set_c_header_path(const char *incdir)           { RootNode.supset(RIDX_H_PATH, incdir); }
inline ssize_t get_c_header_path(char *buf, size_t bufsize) { return RootNode.supstr(RIDX_H_PATH, buf, bufsize); }
inline void set_c_macros(const char *macros)                { RootNode.supset(RIDX_C_MACROS, macros); }
inline ssize_t get_c_macros(char *buf, size_t bufsize)      { return RootNode.supstr(RIDX_C_MACROS, buf, bufsize); }

//------------------------------------------------------------------------
// HIGH LEVEL FUNCTIONS TO SUPPORT TILS IN THE IDA KERNEL
// This functions are mainly for the kernel only.

// Pointer to the local type library. This til is private for each
// IDB file.

idaman ida_export_data til_t *idati;

void init_til(bool newfile);
void save_til(void);
void term_til(void);

void determine_til(void);

idaman char *ida_export get_tilpath(char *tilbuf, size_t tilbufsize);
void autoload_til(const char *cfgfname, const char *sigfname);

idaman bool ida_export get_idainfo_by_type(
                       const type_t *&rtype,
                       const p_list *fields,
                       size_t *psize, flags_t *pflags, typeinfo_t *mt,
                       size_t *alsize=NULL);

void apply_callee_type(ea_t caller, ea_t callee);

// propagate stack argument information
void propagate_stkargs(void);

void build_anon_type_name(char *buf, size_t bufsize, const type_t *type, const p_list *fields);

// break down the function type into argument types
// returns the number of function parameters
// the caller should free the parameter types and names using free_funcarg_arrays()
idaman int ida_export build_funcarg_arrays(const type_t *type,
                         const p_list *fields,
                         ulong *arglocs,        // pointer to array of parameter locations
                         type_t **types,        // pointer to array of parameter types
                         char **names,          // pointer to array of parameter names
                         int maxargs,           // size of these arrays
                         bool remove_constness);// remove constness from all function argument types

idaman void ida_export free_funcarg_arrays(type_t **types, char **names, int n);

idaman type_t *ida_export extract_func_ret_type(const type_t *type, type_t *buf, int bufsize);

// calculate 'names' and 'cmts' lists for a new type
// the caller has to qfree names and cmts
idaman error_t ida_export calc_names_cmts(
                til_t *ti,
                const type_t *type,
                bool find_var(int level,
                              void *ud,
                              const char **name,
                              const char **cmt),
                void *ud,
                p_list **names,
                p_list **cmts);

// resolve BT_COMPLEX type
idaman bool ida_export resolve_complex_type(
                          const type_t **ptype,         // in/out
                          const p_list **fields,        // in/out
                          char *fname,                  // out: type name
                          size_t fnamesize,             // in: sizeof(fname)
                          type_t *bt,                   // out
                          int *N);                      // out: nfields and alignment


// process each field of a complex type
idaman int ida_export foreach_strmem(
                   const type_t *type, // points to field types
                   const p_list *fields,
                   int N,               // nfields and alignment
                   bool is_union,
                   int idaapi func(ulong offset,
                                   const type_t *type,
                                   const p_list *fields,
                                   const char *name,
                                   void *ud),
                   void *ud);

idaman bool ida_export is_type_scalar(const type_t *type);

typedef int type_sign_t;
const type_sign_t
  no_sign       = 0,    // or unknown
  type_signed   = 1,    // signed type
  type_unsigned = 2;    // unsigned type

idaman type_sign_t ida_export get_type_signness(const type_t *type);
inline bool is_type_signed  (const type_t *type) { return get_type_signness(type) == type_signed; }
inline bool is_type_unsigned(const type_t *type) { return get_type_signness(type) == type_unsigned; }


// get structure member at the specified offset
idaman bool ida_export get_struct_member(
                                  const type_t *type,   // in: type
                                  const p_list *fields, // in: fields. for typedefs may be NULL
                                  asize_t offset,       // in: offset
                                  asize_t *delta,       // out: offset from the member start
                                  char *name,           // out: buffer for the member name
                                  size_t namesize,      // in: sizeof(name)
                                  type_t *ftype,        // out: buffer for the member type
                                  size_t typesize,      // in: sizeof(ftype)
                                  p_list *ffields,      // out: buffer for the member field names
                                  size_t ffldsize,      // in: sizeof(ffields)
                                  char *sname,          // out: the structure type name
                                  size_t snamesize);    // in: sizeof(sname)

bool idb_type_to_til(const char *type_name, type_t *tbuf, int tsize, p_list *fbuf, int fsize);
bool get_idb_type(const char *type_name, const type_t **type, const p_list **field);

// helper function for the processor modules
// to be called from ph.use_stkarg_type
idaman bool ida_export apply_type_to_stkarg(op_t &x, uval_t v, const type_t *type, const char *name);

// helper function for the processor modules
// to be called from ph.use_arg_types() for the delay slots
// returns new value of 'rn'
// please use gen_use_arg_types() which is more high level
idaman int ida_export use_regarg_type_cb(ea_t ea,
                                         const type_t **rtypes,
                                         const char **rnames,
                                         ulong *rlocs,
                                         int rn,
                                         void *ud=NULL);

//------------------------------------------------------------------------
// helper function for the processor modules
// to be called from ph.use_arg_types() to do everything
// 3 callbacks should be provided:

// set the operand type as specified

typedef bool idaapi set_op_type_t(op_t &x, const type_t *type, const char *name);


// is the current insn a stkarg load?
// if yes, src - index of the source operand
//         dst - index of the destination operand

typedef bool idaapi is_stkarg_load_t(int *src, int *dst);


// the call instruction with a delay slot?

typedef bool idaapi has_delay_slot_t(ea_t caller);


// the main function using these callbacks:

idaman int ida_export gen_use_arg_types(ea_t caller,
                       const type_t * const *types,
                       const char * const *names,
                       const ulong *arglocs,
                       int n,
                       const type_t **rtypes,
                       const char **rnames,
                       ulong *rlocs,
                       int rn,
                       set_op_type_t *set_op_type,
                       is_stkarg_load_t *is_stkarg_load, // may be NULL
                       has_delay_slot_t *has_delay_slot=NULL);


#pragma pack(pop)
#endif // _TYPEINF_HPP
