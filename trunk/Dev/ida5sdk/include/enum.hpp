/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-99 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 *      Enums and bitfields
 *      Bitfields will be abbreviated as "bf".
 *
 *  06.03.2001: now we allow multiple symbolic constants with the same
 *              value in an enumeration. The multiple constans will be
 *              distinguished by their "serial" number. There can be
 *              up to 256 constants with the same value in an enumeration.
 *
 */

#ifndef _ENUM_HPP
#define _ENUM_HPP
#pragma pack(push, 1)

#include <stdio.h>

#include <help.h>
#include <nalt.hpp>

//--------------------------------------------------------------------------
// enumerations and bitfields are represented as enum_t.

// internal declaration, you shouldn't use this:
// (except of type names like enum_t, const_t, bmask_t)

idaman netnode ida_export_data enums;   // main netnode to access all enums
                                        // should not be used directly!
                                        // altval(idx)-1 -> enum_t
                                        // altval('R',enum_t)-1 -> idx
                                        // altval(-1)  -> qty
#define ENUM_REVERSE    'Y'
#define ENUM_SELMEMS    'Z'     // temporary array for choose_enum_by_value
#define ENUM_QTY_IDX    uval_t(-1)
#define ENUM_FLG_IDX    uval_t(-3)
#define ENUM_FLAGS      uval_t(-5)
#define ENUM_FLAGS_IS_BF  0x00000001
#define ENUM_FLAGS_HIDDEN 0x00000002

typedef tid_t enum_t;   // enum_t is simple netnode
                        // name(), cmt() and rptcmt() are kept in the usual form
                        // memqty is kept in altval(-1)
                        // Bitfields have altval(-5) set to 1.
                        // enums: all members are kept in altval('E',member_value)-1
                        // bf: altval('M') keeps ptrs to nodes by bitmasks
typedef uval_t bmask_t; // a bit mask is 32/64 bits
                        //     if a mask is one-bit, then ptr to const_t is kept immediately in altval('M')
#define DEFMASK (bmask_t(-1))

                        //     otherwise, masknode.altval('E') keeps ptrs to const_t by values
                        //     (the same as with enums)
#define ENUM_MASKS      'm' // indexed by masks
#define ENUM_MEMBERS    'E' // indexed by values
typedef uval_t const_t; // members of enums are kept as netnodes
                        // name(), cmt() and rptcmt() are kept in the usual form
                        // altval(-2) is enum_t
#define CONST_ENUM      uval_t(-2)
#define CONST_VALUE     uval_t(-3)
#define CONST_BMASK     uval_t(-6)
#define CONST_SERIAL    uval_t(-7) // the serial number of the constant
#define CONST_SERIALS   's' // indexed by serials (1..n)
                            // the serials array is present only at the constant
                            // with the minimal serial number (usually 0)

const uchar MAX_ENUM_SERIAL = 255;

inline void     init_enums(void) { enums.create("$ enums"); }
inline void     save_enums(void) {}
inline void     term_enums(void) {}

bool set_enum_flag(enum_t id, ulong bit, bool hidden);  // internal function
//--------------------------------------------------------------------------
// PUBLIC DECLARATIONS START HERE

// QUERIES

// get number of declared enum_t types
inline size_t   get_enum_qty(void)      { return size_t(enums.altval(ENUM_QTY_IDX)); }

// get enum by its ordinal number (0..n)
inline enum_t   getn_enum(size_t n)     { return (int(n)==ENUM_QTY_IDX) ? BADNODE : enums.altval(n)-1; }

// get serial number of enum.
// the serial number determines the place of the enum in the enum window
inline uval_t   get_enum_idx(enum_t id) { return enums.altval(id,ENUM_REVERSE)-1; }

// get enum by name
idaman enum_t ida_export get_enum(const char *name);

// is enum a bitfield?
// (otherwise - plain enum, no bitmasks except of DEFMASK are allowed)
inline bool     is_bf(enum_t id)                        { return (netnode(id).altval(ENUM_FLAGS) & ENUM_FLAGS_IS_BF) != 0; }

// is enum collapsed?
inline bool     is_enum_hidden(enum_t id)               { return (netnode(id).altval(ENUM_FLAGS) & ENUM_FLAGS_HIDDEN) != 0; }
inline bool     set_enum_hidden(enum_t id, bool hidden) { return set_enum_flag(id, ENUM_FLAGS_HIDDEN, hidden); }

// get name of enum
inline ssize_t  get_enum_name(enum_t id, char *buf, size_t bufsize) { return id > MAXADDR ? netnode(id).name(buf, bufsize) : -1;  }

// get enum comment
inline ssize_t  get_enum_cmt(enum_t id, bool repeatable, char *buf, size_t bufsize) { return id > MAXADDR ? netnode(id).supstr(repeatable != 0, buf, bufsize) : -1; }

// get the number of the members of the enum
inline size_t   get_enum_size(enum_t id)                { return id > MAXADDR ? size_t(netnode(id).altval(ENUM_QTY_IDX)) : 0; }

// get flags determining the representation of the enum
// (currently they define the numeric base: octal, decimal, hex, bin) and signness
inline flags_t  get_enum_flag(enum_t id)                { return id > MAXADDR ? flags_t(netnode(id).altval(ENUM_FLG_IDX)) : 0; }


// get symbolic constant (a enum member) by its name
idaman const_t ida_export get_const_by_name(const char *name);

// get value of a constant
inline uval_t   get_const_value(const_t id)             { return id > MAXADDR ? netnode(id).altval(CONST_VALUE) : 0; }

// get the parent enum of a constant
inline enum_t   get_const_enum(const_t id)              { return id > MAXADDR ? netnode(id).altval(CONST_ENUM)-1 : BADNODE; }

// get bitmask of a constant
inline bmask_t  get_const_bmask(const_t id)             { return id > MAXADDR ? netnode(id).altval(CONST_BMASK)-1 : BADNODE; }

// find a constant by enum, value and bitmask
idaman const_t ida_export get_const(enum_t id, uval_t value, uchar serial, bmask_t mask);

// get access to all used bitmasks in the enum
inline bmask_t  get_first_bmask(enum_t id)              { return netnode(id).alt1st(ENUM_MASKS); }
inline bmask_t  get_last_bmask(enum_t id)               { return netnode(id).altlast(ENUM_MASKS); }
inline bmask_t  get_next_bmask(enum_t id,bmask_t bmask) { return netnode(id).altnxt(bmask,ENUM_MASKS); }
inline bmask_t  get_prev_bmask(enum_t id,bmask_t bmask) { return netnode(id).altprev(bmask,ENUM_MASKS); }

// get access to all constants with the specified bitmask in the enum
// these function return values, not const_t!
idaman uval_t ida_export get_first_const(enum_t id, bmask_t bmask=DEFMASK);
idaman uval_t ida_export get_last_const(enum_t id, bmask_t bmask=DEFMASK);
idaman uval_t ida_export get_next_const(enum_t id, uval_t value, bmask_t bmask=DEFMASK);
idaman uval_t ida_export get_prev_const(enum_t id, uval_t value, bmask_t bmask=DEFMASK);

// get name of constant by const_t
inline ssize_t  get_const_name(const_t id, char *buf, size_t bufsize) { return netnode(id).name(buf, bufsize);  }

// get comment of constant
inline ssize_t  get_const_cmt(const_t id, bool repeatable, char *buf, size_t bufsize) { return netnode(id).supstr(repeatable != 0, buf, bufsize); }


// get access to all constants with the specified value and mask
// a sample loop looks like this:
//      const_t main_cid;
//      uchar serial;
//      for ( const_t cid=main_cid=get_first_serial_const(id, v, &serial, mask);
//            cid != BADNODE;
//            cid = get_next_serial_const(main_cid, &serial) )
//      {
//        ...
//      }
//
// The 'serial' argument of get_first_const/get_last_const() can be
// specified as NULL.
// It is required for the other functions.

idaman const_t ida_export get_first_serial_const(enum_t id, uval_t value, uchar *serial, bmask_t bmask);
idaman const_t ida_export get_last_serial_const(enum_t id, uval_t value, uchar *serial, bmask_t bmask);
idaman const_t ida_export get_next_serial_const(const_t first_cid, uchar *serial);
idaman const_t ida_export get_prev_serial_const(const_t first_cid, uchar *serial);


// get serial number of a constant
inline uchar get_const_serial(const_t cid) { return uchar(netnode(cid).altval(CONST_SERIAL)); }
//--------------------------------------------------------------------------
// MANIPULATION

// add new enum type
// if idx==BADADDR then add as the last idx
// if name==NULL then generate a unique name "enum_%d"
idaman enum_t ida_export add_enum(size_t idx, const char *name, flags_t flag);
idaman void   ida_export del_enum(enum_t id);
idaman bool   ida_export set_enum_idx(enum_t id, size_t idx);

// set 'bitfield' bit of enum (i.e. convert it to a bitfield)
idaman bool ida_export set_enum_bf(enum_t id, bool bf);

idaman bool ida_export set_enum_name(enum_t id,const char *name);
idaman bool ida_export set_enum_cmt(enum_t id,const char *cmt,bool repeatable);
inline bool     set_enum_flag(enum_t id,flags_t flag)     { return netnode(id).altset(ENUM_FLG_IDX,flag) != 0; }

// this function returns error code (0 is ok)
idaman int ida_export add_const(enum_t id,const char *name, uval_t value, bmask_t bmask=DEFMASK);
#define CONST_ERROR_NAME  1     // already have member with this name (bad name)
#define CONST_ERROR_VALUE 2     // already have 256 members with this value
#define CONST_ERROR_ENUM  3     // bad enum id
#define CONST_ERROR_MASK  4     // bad bmask
#define CONST_ERROR_ILLV  5     // bad bmask and value combination (~bmask & value != 0)

idaman bool ida_export del_const(enum_t id, uval_t value, uchar serial, bmask_t bmask);
inline bool     set_const_name(const_t id, const char *name)                { return set_enum_name(id,name);            }
inline bool     set_const_cmt(const_t id, const char *cmt, bool repeatable) { return set_enum_cmt(id, cmt, repeatable); }

// kernel helper functions:

inline uval_t   get_selected_enum(size_t n) { return enums.altval(n,ENUM_SELMEMS); }
inline void     add_selected_enum(size_t *idx, enum_t id) { enums.altset((*idx)++,id,ENUM_SELMEMS); }
inline void     unmark_selected_enums(void){ enums.altdel_all(ENUM_SELMEMS); }

// can we use the specified bitmask in the enum?
bool is_good_bmask(enum_t id, bmask_t bmask);

inline bool is_one_bit_mask(bmask_t mask)
{
  return (mask & (mask-1)) == 0;
}

// get bitmask node if bf-scheme is used
// otherwise returns BADNODE
inline netnode get_bmask_node(enum_t id, bmask_t bmask)
{
  if ( !is_bf(id) ) return BADNODE;
  return netnode(id).altval(bmask, ENUM_MASKS)-1;
}

inline bool set_enum_flag(enum_t id, ulong bit, bool set)
{
  if ( id == BADNODE ) return false;
  netnode n(id);
  ulong f = ulong(n.altval(ENUM_FLAGS));
  setflag(f, bit, set);
  return n.altset(ENUM_FLAGS, f) != 0;
}

// work with the bitmask name & comment
inline bool set_bmask_name(enum_t id,bmask_t bmask, const char *name)
  { return get_bmask_node(id,bmask).rename(name); }
inline ssize_t get_bmask_name(enum_t id,bmask_t bmask, char *buf, size_t bufsize)
  { return get_bmask_node(id,bmask).name(buf, bufsize);       }
inline bool set_bmask_cmt(enum_t id,bmask_t bmask, const char *cmt, bool repeatable)
  { return set_enum_cmt(get_bmask_node(id,bmask), cmt, repeatable); }
inline ssize_t get_bmask_cmt(enum_t id,bmask_t bmask, bool repeatable, char *buf, size_t bufsize)
  { return get_bmask_node(id,bmask).supstr(repeatable != 0, buf, bufsize); }

// get enum id by the bitmask name. optionally find the bitmask value too.
enum_t get_bmask_enum(const char *mask_name, bmask_t *value);

// enum id and const serial number encoding/decoding
#define NBITS (sizeof(uval_t)*8-8)
#define MASK (uval_t(0xFF)<<NBITS)

inline uval_t enum_encode(enum_t id, uchar serial)
{
  return id + (uval_t(serial) << NBITS);
}

inline enum_t enum_decode(uval_t code, uchar *serial)
{
  uval_t delta = (code - MAXADDR) & MASK;
  if ( serial != NULL )
    *serial = uchar(delta >> NBITS);
  return code - delta;
}
#undef MASK
#undef NBITS

#ifndef ENUM_SOURCE
#define enums   dont_use_enums_directly         // prohibit access to 'enums'
                                                // netnode
#undef ENUM_REVERSE
#undef ENUM_SELMEMS
#undef ENUM_QTY_IDX
#undef ENUM_FLG_IDX
#undef ENUM_FLAGS
#undef ENUM_FLAGS_IS_BF
#undef ENUM_FLAGS_HIDDEN
#undef ENUM_MASKS
#undef ENUM_MEMBERS
#undef CONST_ENUM
#undef CONST_VALUE
#undef CONST_BMASK
#undef CONST_SERIAL
#undef CONST_SERIALS

#endif

#pragma pack(pop)
#endif // _ENUM_HPP
