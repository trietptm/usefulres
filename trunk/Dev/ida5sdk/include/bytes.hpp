/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-97 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 */

#ifndef BYTES_HPP
#define BYTES_HPP

#include <nalt.hpp>
#include <lines.hpp>
#include <area.hpp>
#pragma pack(push, 1)   // IDA uses 1 byte alignments!

typedef tid_t enum_t;   // #include <enum.hpp>


// This file contains functions that deal with individual byte
// characteristics.
// Each byte of the disassembled program is represented by a 32-bit
// value. We will call this value 'flags'. The structure of the flags is
// represented below. You are not allowed to inspect individual bits
// of flags and modify them directly. Use special function to inspect
// and/or modify flags.
// Flags are kept in a virtual array file (*.id1)
// Addresses (ea) are all 32-bit (or 64-bit) quantities.
//
//--------------------------------------------------------------------------
// allocate flags for address range (startEA..endEA)
// startEA should be lower than endEA.
// endEA does not belong to the range.
// exit with an error message if not enough disk space
// return 0 if ok
// otherwise return error code

idaman error_t ida_export FlagsEnable(ea_t startEA, ea_t endEA);


// deallocate flags for address range (startEA..endEA)
// startEA should be lower than endEA.
// endEA does not belong to the range.
// exit with an error message if not enough disk space (this may occur too)
// return 0 if ok
// otherwise return error code

idaman error_t ida_export FlagsDisable(ea_t startEA, ea_t endEA);


// Get next address in the program. (i.e. address which has flags)
// Return BADADDR if no such address exist.

idaman ea_t ida_export nextaddr(ea_t ea);


// Get previous address in the program.
// Return BADADDR if no such address exist.

idaman ea_t ida_export prevaddr(ea_t ea);


// Get start of next contiguous addresses in the program.
// (a block of addresses without holes)
// Return BADADDR if next chunk doesn't exist.

idaman ea_t ida_export nextchunk(ea_t ea);


// Get start of previous contiguous addresses in the program.
// Return BADADDR if previous chunk doesn't exist.

idaman ea_t ida_export prevchunk(ea_t ea);


// Get start of the contiguous address block containing 'ea'.
// Return BADADDR if 'ea' doesn't belong to the program.

idaman ea_t ida_export chunkstart(ea_t ea);


// Get size of the contiguous address block containing 'ea'.
// Return 0 if 'ea' doesn't belong to the program.

idaman asize_t ida_export chunksize(ea_t ea);


// Search for a hole in the addressing space of the program.
// bottom - address to start searching
// size   - size of desired block
// step   - bit mask for the start of hole (0xF would align hole to a paragraph)
//          if 'step' is negative, the bottom address with be aligned
//          otherwise the kernel will try to use it as is and align it
//          only the hole is too small.
// Return start of the hole or BADADDR

idaman ea_t ida_export freechunk(ea_t bottom, asize_t size, long step);


// Find next address with a flag satisfying the function 'testf'.
// Start searching from address 'ea'+1 and inspect bytes up to 'maxea'.
// maxea is not included in the search range.
// ud means 'user data' and may point to anything. it will be passed to testf.
// Return the found address or BADADDR.

typedef bool idaapi testf_t(flags_t flags, void *ud);     // flag tester

idaman ea_t ida_export nextthat(
        ea_t ea,
        ea_t maxea,
        testf_t *testf,
        void *ud);


// Find previous address with a flag satisfying the function 'testf'.
// Start searching from address 'ea'+1 and inspect bytes up to 'minea'.
// minea is included in the search range.
// ud means 'user data' and may point to anything. it will be passed to testf.
// Return the found address or BADADDR.

idaman ea_t ida_export prevthat(
        ea_t ea,
        ea_t maxea,
        testf_t *testf,
        void *ud);


// Get start of previous defined item. Return BADADDR if none exist.
// minea is included in the search range

idaman ea_t ida_export prev_head(ea_t ea, ea_t minea);


// Get start of next defined item. Return BADADDR if none exist.
// maxea is not included in the search range

idaman ea_t ida_export next_head(ea_t ea, ea_t maxea);


// Get address of previous non-tail byte. Return BADADDR if none exist.

idaman ea_t ida_export prev_not_tail(ea_t ea);


// Get address of next non-tail byte. Return BADADDR if none exist.

idaman ea_t ida_export next_not_tail(ea_t ea);


// Adjust the address and get the nearest visible address
// (i.e. an adress which will appear in the disassembly)
// This function returns BADADDR only if no addresses are valid

ea_t adjust_visea(ea_t ea);


// Get previous visible address
// Returns BADADDR if none exists.

idaman ea_t ida_export prev_visea(ea_t ea);


// Get next visible address
// Returns BADADDR if none exists.

idaman ea_t ida_export next_visea(ea_t ea);


// Is an address the first visible address?

bool is_first_visea(ea_t ea);


// Is an address the last visible address?

bool is_last_visea(ea_t ea);


// Is the address visible on the screen (not hidden)?

bool is_visible_finally(ea_t ea); // do we need to show anything
                                  // at this address?

// this function is only for the kernel
idaman void ida_export invalidate_visea_cache(void);


// Get the start address of the item at 'ea'.
// If there is no current item, then 'ea' will be returned
// (see definition at the end of this file)

inline ea_t idaapi get_item_head(ea_t ea);


// Get the end address of the item at 'ea'. The returned address
// doesn't belong to the current item. Unexplored bytes are counted as
// 1 byte entities.

idaman ea_t ida_export get_item_end(ea_t ea);


// Calculate maximal reasonable end address of a new item.
// This function will limit the item with the current segment bounds.
//      ea      - linear address.
//      how     - when to stop the search. A combination of ITEM_END...
// returns: end of new item. If it is not possible to create an item,
//          it will return 'ea'.

#define ITEM_END_FIXUP  0x0001          // stop at the first fixup
#define ITEM_END_INITED 0x0002          // if  isLoaded(ea): stop if uninitialized byte is encountered
                                        // if !isLoaded(ea): stop if   initialized byte is encountered
#define ITEM_END_NAME   0x0004          // stop at the first named location
#define ITEM_END_XREF   0x0008          // stop at the first referenced location
idaman ea_t ida_export calc_max_item_end(ea_t ea, int how=15);


// Get size of item (instruction/data) in bytes. Unexplored bytes have
// length of 1 byte. This function never returns 0.

inline asize_t get_item_size(ea_t ea) { return get_item_end(ea) - ea; }


// Flush virtual array to disk

void fluFlags(void);


// Is the specified address 'ea' present in the program?

idaman bool ida_export isEnabled(ea_t ea);


// Get flags for the specified address, extended form

idaman flags_t ida_export get_flags_ex(ea_t ea, int how);

#define GFE_NOVALUE 0x0001      // get flags without FF_IVL & MS_VAL
                                // It is much faster under remote debugging
                                // because the kernel does not need to check
                                // if the memory is available and readable

inline flags_t idaapi get_flags_novalue(ea_t ea) { return get_flags_ex(ea, GFE_NOVALUE); }


// Get flags value for address 'ea'. If address is not present in the program,
// return 0.

inline flags_t idaapi getFlags(ea_t ea) { return get_flags_ex(ea, 0); }


// Get flag of the item at 'ea' even if it is a tail byte of some
// array or structure. This function is used to get flags of structure members
// or array elements.
//      from    - linear address of the instruction which refers to 'ea'
//      n       - number of operand which refers to 'ea'
//      ea      - the referenced address
//      appzero - meaningful only if the name refers to a structure.
//                append a struct field name if the field offset is zero?
// returns: flags or 0 (if failed)

idaman flags_t ida_export get_item_flag(ea_t from, int n, ea_t ea, bool appzero);


// Modify flags value for address 'ea'.  (ONLY KERNEL MAY USE THIS FUNCTION!)
// This function never changes FF_IVL and MS_VAL fields

idaman void ida_export setFlags(ea_t ea, flags_t flags);


// Set the specified bits of flags (ONLY KERNEL MAY USE THIS FUNCTION!)
// This function never changes FF_IVL and MS_VAL fields

idaman void ida_export setFlbits(ea_t EA, flags_t bits);


// Clear the specified bits of flags (ONLY KERNEL MAY USE THIS FUNCTION!)
// This function never changes FF_IVL and MS_VAL fields

idaman void ida_export clrFlbits(ea_t EA, flags_t bits);


//--------------------------------------------------------------------------
//
//      Here we define structure of flags.
//      Low 8 bits contain value of corresponding byte of the program.
//      The next bit is set if the byte is initialized
//

#define MS_VAL  0x000000FFL             // Mask for byte value
#define FF_IVL  0x00000100L             // Byte has value ?

// Do flags contain byte value?

inline bool idaapi hasValue(flags_t F)  { return (F & FF_IVL) != 0; }
inline bool idaapi f_hasValue(ulong f, void *) { return hasValue(f); }


// Delete byte value from flags. The corresponding byte becomes
// uninitialized.

idaman void ida_export delValue(ea_t ea);


// Does the specified address have a byte value (is initialized?)

idaman bool ida_export isLoaded(ea_t ea);


// Get one byte (8-bit) of the program at 'ea'
// This function works only for 8bit byte processors.

idaman uchar ida_export get_byte(ea_t ea);


// Get one word (16-bit) of the program at 'ea'
// This function takes into account order of bytes specified in inf.mf
// This function works only for 8bit byte processors.

idaman ushort ida_export get_word(ea_t ea);

// Get one 3byte (24-bit) of the program at 'ea'
// This function takes into account order of bytes specified in inf.mf
// This function works only for 8bit byte processors.

idaman ulong ida_export get_3byte(ea_t ea);


// Get one dword (32-bit) of the program at 'ea'
// This function takes into account order of bytes specified in inf.mf
// This function works only for 8bit byte processors.

idaman ulong ida_export get_long(ea_t ea);


// Get one qword (64-bit) of the program at 'ea'
// This function takes into account order of bytes specified in inf.mf
// This function works only for 8bit byte processors.

idaman ulonglong ida_export get_qword(ea_t ea);


// Get one wide byte of the program at 'ea'
// Some processors may access more than 8bit quantity at an address.
// These processors have 32-bit byte organization from the IDA's point of view.

idaman ulong ida_export get_full_byte(ea_t ea);


// Get one wide word (2 'byte') of the program at 'ea'
// Some processors may access more than 8bit quantity at an address.
// These processors have 32-bit byte organization from the IDA's point of view.
// This function takes into account order of bytes specified in inf.mf

idaman ulonglong ida_export get_full_word(ea_t ea);


// Get two wide words (4 'bytes') of the program at 'ea'
// Some processors may access more than 8bit quantity at an address.
// These processors have 32-bit byte organization from the IDA's point of view.
// This function takes into account order of bytes specified in inf.mf
// Note: this function works incorrectly if ph.nbits > 16

idaman ulonglong ida_export get_full_long(ea_t ea);


// Get 8 bits of the program at 'ea'
// The main usage of this function is to iterate range of bytes.
// Here is an example:
//      ulong v;
//      int nbit = 0;
//      for ( .... ) {
//        uchar byte = get_8bit(ea,v,nbit);
//        ...
//      }
// 'ea' is incremented each time when a new byte is read.
// In the above example, it will be incremented in the first loop iteration.

idaman uchar ida_export get_8bit(ea_t &ea,ulong &v,int &nbit);
uchar get_ascii_char(ea_t &ea, ulong &v, int &nb); // takes into account AS2_BYTE1CHAR


// Get 16bits of the program at 'ea'
// If the current processor has 16-bit byte, return 1 byte (getFullByte())
// Otherwise return getWord()

idaman ulong ida_export get_16bit(ea_t ea);


// Get not more than 32bits of the program at 'ea'
// Depending on the ph.nbits, returns 32 bit value
//   if ( nbits <= 8 ) return get_long(ea);
//   if ( nbits <= 16) return get_full_word(ea);
//   return get_full_byte(ea);

idaman ulong ida_export get_32bit(ea_t ea);


// Get not more than 64bits of the program at 'ea'
// Depending on the ph.nbits, returns 64 bit value
//   if ( nbits <= 8 ) return get_qword(ea);
//   if ( nbits <= 16) return get_full_long(ea);
//   return get_full_byte(ea);

idaman ulonglong ida_export get_64bit(ea_t ea);


// Get the value at of the item at 'ea'
// This function works with entities up to sizeof(ea_t)
// (bytes, word, etc)
//      ea - linear address
//      v  - pointer to the result. may be NULL
//      size - size of data to read. If 0, then the item
//             type at 'ea' will be used
// Returns: success

idaman bool ida_export get_data_value(ea_t ea, uval_t *v, asize_t size);


// Get original byte value (that was before patching)
// This function works for wide byte processors too.

idaman ulong ida_export get_original_byte(ea_t ea);


// Get original word value (that was before patching)
// This function works for wide byte processors too.
// This function takes into account order of bytes specified in inf.mf

idaman ulonglong ida_export get_original_word(ea_t ea);


// Get original long value (that was before patching)
// This function works for wide byte processors too.
// This function takes into account order of bytes specified in inf.mf

idaman ulonglong ida_export get_original_long(ea_t ea);


// Set value of one byte of the program.
// This function works for wide byte processors too.
//      ea - linear address
//      x  - byte value
// Note: The original value of the byte is completely lost and can't
// be recovered by the get_original_byte() function.
// returns: true-the database has been modified

idaman bool ida_export put_byte(ea_t ea, ulong x);

// Set value of one word of the program.
// This function takes into account order of bytes specified in inf.mf
// This function works for wide byte processors too.
//      ea - linear address
//      x  - word value
// Note: The original value of the word is completely lost and can't
// be recovered by the get_original_word() function.

idaman void ida_export put_word(ea_t ea, ulonglong x);


// Set value of one dword of the program.
// This function takes into account order of bytes specified in inf.mf
// This function works for wide byte processors too.
//      ea - linear address
//      x  - dword value
// Note: The original value of the dword is completely lost and can't
// be recovered by the get_original_long() function.

idaman void ida_export put_long(ea_t ea, ulonglong x);


// Set value of one qword (8 bytes) of the program.
// This function takes into account order of bytes specified in inf.mf
// This function works for wide byte processors too.
//      ea - linear address
//      x  - qword value

idaman void ida_export put_qword(ea_t ea, ulonglong x);


// Patch a byte of the program. The original value of the byte is saved
// and can be obtained by get_original_byte() function.
// This function works for wide byte processors too.
// returns: true-the database has been modified

idaman bool ida_export patch_byte(ea_t ea, ulong x);


// Patch a word of the program. The original value of the word is saved
// and can be obtained by get_original_word() function.
// This function works for wide byte processors too.
// This function takes into account order of bytes specified in inf.mf

idaman void ida_export patch_word(ea_t ea, ulonglong x);


// Patch a dword of the program. The original value of the dword is saved
// and can be obtained by get_original_long() function.
// This function DOESN'T work for wide byte processors.
// This function takes into account order of bytes specified in inf.mf

idaman void ida_export patch_long(ea_t ea, ulonglong x);


// Add a value to one byte of the program.
// This function works for wide byte processors too.
//      ea     - linear address
//      value  - byte value

idaman void ida_export add_byte(ea_t ea, ulong value);


// Add a value to one word of the program.
// This function works for wide byte processors too.
// This function takes into account order of bytes specified in inf.mf
//      ea     - linear address
//      value  - byte value

idaman void ida_export add_word(ea_t ea, ulonglong value);


// Add a value to one dword of the program.
// This function works for wide byte processors too.
// This function takes into account order of bytes specified in inf.mf
//      ea     - linear address
//      value  - byte value
// Note: this function works incrrectly if ph.nbits > 16

idaman void ida_export add_long(ea_t ea, ulonglong value);


// Get the specified number of bytes of the program into the buffer.
//       ea - linear address
//       buf - buffer to hold bytes
//       size - size of buffer in normal 8-bit bytes (sizeof(buf))
// returns: 1-ok
//          0-failure
//            (out of program address space or uninited bytes, for example)

idaman bool ida_export get_many_bytes(ea_t ea, void *buf, ssize_t size);


// Modify the specified number of bytes of the program.
// This function does not save the original values of bytes.
// (this function is for the kernel only)
//       ea - linear address
//       buf - buffer with new values of bytes
//       size - size of buffer in normal 8-bit bytes (sizeof(buf))
// See also patch_many_bytes() function.

idaman void ida_export put_many_bytes(ea_t ea, const void *buf, size_t size);


// Patch the specified number of bytes of the program.
// Original values of bytes are saved and are available with get_original...()
// functions.
// (this function is for the kernel only)
//       ea - linear address
//       buf - buffer with new values of bytes
//       size - size of buffer in normal 8-bit bytes (sizeof(buf))
// See also put_many_bytes() function.

idaman void ida_export patch_many_bytes(ea_t ea, const void *buf, size_t size);


//-------------------------------------------------------------------------
//      Each byte of the program may be in one of four states:
//        - unexplored
//        - start of instruction
//        - start of data
//        - second, third (tail) byte of instruction or data.
//      Initially all bytes of the program are unexplored.
//      IDA modifies flags and doing so converts bytes to instructions
//      and data.

#define MS_CLS  0x00000600L             // Mask for typing
#define FF_CODE 0x00000600L             // Code ?
#define FF_DATA 0x00000400L             // Data ?
#define FF_TAIL 0x00000200L             // Tail ?
#define FF_UNK  0x00000000L             // Unknown ?

// Does flag denote start of an instruction?

inline bool idaapi isCode(flags_t F)  { return (F & MS_CLS) == FF_CODE; }
inline bool idaapi f_isCode(flags_t F, void *) { return isCode(F); }


// Does flag denote start of data?

inline bool idaapi isData(flags_t F)  { return (F & MS_CLS) == FF_DATA; }
inline bool idaapi f_isData(flags_t F, void *) { return isData(F); }


// Does flag denote tail byte?

inline bool idaapi isTail(flags_t F)    { return (F & MS_CLS) == FF_TAIL; }
inline bool idaapi f_isTail(flags_t F, void *) { return isTail(F); }
inline bool idaapi isNotTail(flags_t F) { return !isTail(F); }
inline bool idaapi f_isNotTail(flags_t F, void *) { return isNotTail(F); }


// Does flag denote unexplored byte?

inline bool idaapi isUnknown(flags_t F){return (F & MS_CLS) == FF_UNK;  }
inline bool idaapi f_isUnknown(flags_t F, void *) { return isUnknown(F); }


// Does flag denote start of instruction OR data?

inline bool idaapi isHead(flags_t F)  { return (F & FF_DATA) != 0; }
inline bool idaapi f_isHead(flags_t F, void *) { return isHead(F); }


// Convert item (instruction/data) to unexplored bytes
// The whole item (including the head and tail bytes) will be destroyed
// It is allowed to pass any address in the item to this function
//      expand - propogate undefined items, for example removing an instruction
//               removes all references to the next instruction, then plan
//               to convert to unexplored the next instruction too.

idaman void ida_export do_unknown(ea_t ea, bool expand);

// Convert range of addresses to unexplored bytes
// The whole items (including the head and tail bytes) will be destroyed
//      ea     - starting linear address
//      size   - number of bytes in the area to be undefined
//      expand - propogate undefined items, for example removing an instruction
//               removes all references to the next instruction, then plan
//               to convert to unexplored the next instruction too.

idaman void ida_export do_unknown_range(ea_t ea, size_t size, bool expand);


//-------------------------------------------------------------------------
// Manual instructions (they are used to completely override an automatically
// generated instruction by a user specified string).

// Is the instruction overridden?
//      ea - linear address of the instruction or data item

idaman bool ida_export is_manual_insn(ea_t ea);        // Is the instruction overridden?


// Retrieve the user-specified string for the manual instruction
//      ea       - linear address of the instruction or data item
//      buf      - the input buffer for the string
//                 if NULL, the buffer will be allocated in heap
//      bufsize  - size of the buffer if buf != NULL
//                 otherwise bufsize should be 0
// returns: NULL - no user-specified string

idaman char *ida_export get_manual_insn(ea_t ea,char *buf, size_t bufsize);
                                // Get the user-specified instruction string


// Set manual instruction string
//      ea - linear address of the instruction or data item
//      manual_insn -   ""   - delete manual string
//                      NULL - do nothing
// If "manual_insn" is equal to an empty string, then the
// manual instruction string will be deleted

idaman void ida_export set_manual_insn(ea_t ea,const char *manual_insn); // Set user-speficied string


//-------------------------------------------------------------------------
//
//      Flags keep information common to all four states of bytes.
//      This information will not be automatically discarded during
//      transitions between different states.
//

#define MS_COMM  0x000FF800L            // Mask of common bits
#define FF_COMM  0x00000800L            // Has comment ?
#define FF_REF   0x00001000L            // has references
#define FF_LINE  0x00002000L            // Has next or prev lines ?
#define FF_NAME  0x00004000L            // Has name ?
#define FF_LABL  0x00008000L            // Has dummy name?
#define FF_FLOW  0x00010000L            // Exec flow from prev instruction
#define FF_SIGN  0x00020000L            // Inverted sign of operands
#define FF_BNOT  0x00040000L            // Bitwise negation of operands
#define FF_VAR   0x00080000L            // is variable byte?

// Does the previous instruction exist and pass execution flow to the current
// byte?

inline bool idaapi isFlow(flags_t F)     { return (F & FF_FLOW) != 0; }


// Is the current byte marked as variable? (this is not used by IDA itself and
// may become obsolete, please don't use it)

inline bool idaapi isVar(flags_t F)      { return (F & FF_VAR ) != 0; }


// Does the current byte have additional anterior or posterior lines?

inline bool idaapi hasExtra(flags_t F)   { return (F & FF_LINE) != 0; }


// Does the current byte have an indented comment?

inline bool idaapi has_cmt(flags_t F)    { return (F & FF_COMM) != 0; }


// Does the current byte have cross-references to it?

inline bool idaapi hasRef(flags_t F)     { return (F & FF_REF)  != 0; }
inline bool idaapi f_hasRef(ulong f, void *) { return hasRef(f); }


// Does the current byte have non-trivial (non-dummy) name?

inline bool idaapi has_name(flags_t F)   { return (F & FF_NAME) != 0; }
inline bool idaapi f_has_name(ulong f, void *) { return has_name(f); }


// Does the current byte have dummy (auto-generated, with special prefix) name?

#define FF_ANYNAME      (FF_LABL|FF_NAME)
inline bool idaapi has_dummy_name(flags_t F){ return (F & FF_ANYNAME) == FF_LABL; }
inline bool idaapi f_has_dummy_name(ulong f, void *) { return has_dummy_name(f); }


// Does the current byte have auto-generated (no special prefix) name?

inline bool idaapi has_auto_name(flags_t F){ return (F & FF_ANYNAME) == FF_ANYNAME; }


// Does the current byte have any name?

inline bool idaapi has_any_name(flags_t F){ return (F & FF_ANYNAME) != 0; }


// Does the current byte have user-specified name?

inline bool idaapi has_user_name(flags_t F){ return (F & FF_ANYNAME) == FF_NAME; }
inline bool idaapi f_has_user_name(flags_t F, void *){ return has_user_name(F); }


// signness deals with the form of operands of the current instruction/data.
// inverted sign means the following:
//    if the bit is clear       |then when the bit is set
//    and the output is         |the output should be:
//    ------------              |----------
//    unsigned                  |signed
//    signed                    |unsigned
//

// Should sign of n-th operand inverted during output?
// allowed values of n: 0-first operand, 1-other operands

idaman bool ida_export is_invsign(ea_t ea, flags_t F, int n);


// Toggle sign of n-th operand
// allowed values of n: 0-first operand, 1-other operands

idaman bool ida_export toggle_sign(ea_t ea, int n);


// get bits for signness (this function is used by kernel only)

inline flags_t idaapi signed_data_flag(void) { return FF_SIGN; }
inline bool idaapi is_signed_data(flags_t F) { return (F & FF_SIGN) != 0; }


// Work with binary negation of operands
// ash.bnot should be defined in the idp module in order to work
// with this functions

idaman bool ida_export is_bnot(ea_t ea, flags_t F, int n);           // should negate the operand?
idaman bool ida_export toggle_bnot(ea_t ea, int n);


// get bits for signness (this function is used by kernel only)

inline flags_t idaapi bnot_data_flag(void) { return FF_BNOT; }
inline bool idaapi is_bnot_data(flags_t F) { return (F & FF_BNOT) != 0; }


// Display leading zeroes in operands
// The global switch for the leading zeroes is in inf.s_genflags
// The leading zeroes doesn't work if the octal numbers start with 0

idaman bool ida_export is_lzero(ea_t ea, int n);          // display leading zeroes?
                                                          // (takes into account inf.s_genflags)
idaman bool ida_export set_lzero(ea_t ea, int n);         // set toggle lzero bit
idaman bool ida_export clr_lzero(ea_t ea, int n);         // clear toggle lzero bit
inline bool idaapi toggle_lzero(ea_t ea, int n)
{
  return (is_lzero(ea,n) ? clr_lzero : set_lzero)(ea,n);
}


// Check if leading zeroes are important

idaman bool ida_export leading_zero_important(ea_t ea, int n);


// Set the variableness of an address
// The variable addresses are marked with '*' in the disassembly
//      ea    - linear address
//      isvar - 0: clear 'variable' mark
//              1: set 'variable' mark

idaman void ida_export doVar(ea_t ea,bool isvar=true);


//-------------------------------------------------------------------------
//
//      Type of instruction/data operands is kept in flags too.
//      IDA keeps 2 type values:
//        - type of the first operand
//        - type of other operands (I will call this 'type of second operand'
//          althrough it is apllied to third, fourth operands too)
//      For data bytes, only the first type is used (i.e. all elements of
//      an array have the same type). Type of second operand is not used for
//      data bytes.
//

#define MS_0TYPE 0x00F00000L            // Mask for 1st arg typing
#define FF_0VOID 0x00000000L            // Void (unknown)?
#define FF_0NUMH 0x00100000L            // Hexadecimal number?
#define FF_0NUMD 0x00200000L            // Decimal number?
#define FF_0CHAR 0x00300000L            // Char ('x')?
#define FF_0SEG  0x00400000L            // Segment?
#define FF_0OFF  0x00500000L            // Offset?
#define FF_0NUMB 0x00600000L            // Binary number?
#define FF_0NUMO 0x00700000L            // Octal number?
#define FF_0ENUM 0x00800000L            // Enumeration?
#define FF_0FOP  0x00900000L            // Forced operand?
#define FF_0STRO 0x00A00000L            // Struct offset?
#define FF_0STK  0x00B00000L            // Stack variable?
#define FF_0FLT  0x00C00000L            // Floating point number?

#define MS_1TYPE 0x0F000000L            // Mask for 2nd arg typing
#define FF_1VOID 0x00000000L            // Void (unknown)?
#define FF_1NUMH 0x01000000L            // Hexadecimal number?
#define FF_1NUMD 0x02000000L            // Decimal number?
#define FF_1CHAR 0x03000000L            // Char ('x')?
#define FF_1SEG  0x04000000L            // Segment?
#define FF_1OFF  0x05000000L            // Offset?
#define FF_1NUMB 0x06000000L            // Binary number?
#define FF_1NUMO 0x07000000L            // Octal number?
#define FF_1ENUM 0x08000000L            // Enumeration?
#define FF_1FOP  0x09000000L            // Forced operand?
#define FF_1STRO 0x0A000000L            // Struct offset?
#define FF_1STK  0x0B000000L            // Stack variable?
#define FF_1FLT  0x0C000000L            // Floating point number?

// Is the first operand defined? Initially operand has no type (i.e. void)

inline bool idaapi isDefArg0(flags_t F) { return (F & MS_0TYPE) != FF_0VOID; }


// Is the second operand defined? Initially operand has no type (i.e. void)

inline bool idaapi isDefArg1(flags_t F) { return (F & MS_1TYPE) != FF_1VOID; }


// Is the first operand offset? (example: push offset xxx)

inline bool idaapi isOff0(flags_t F)    { return (F & MS_0TYPE) == FF_0OFF;  }


// Is the second operand offset? (example: mov ax, offset xxx)

inline bool idaapi isOff1(flags_t F)    { return (F & MS_1TYPE) == FF_1OFF;  }


// Is the first operand character constant? (example: push 'a')

inline bool idaapi isChar0(flags_t F)   { return (F & MS_0TYPE) == FF_0CHAR; }


// Is the second operand character constant? (example: mov al, 'a')

inline bool idaapi isChar1(flags_t F)   { return (F & MS_1TYPE) == FF_1CHAR; }


// Is the first operand segment selector? (example: push seg seg001)

inline bool idaapi isSeg0(flags_t F)    { return (F & MS_0TYPE) == FF_0SEG;  }


// Is the second operand segment selector? (example: mov dx, seg dseg)

inline bool idaapi isSeg1(flags_t F)    { return (F & MS_1TYPE) == FF_1SEG;  }


// Is the first operand a symbolic constant (enum member)?

inline bool idaapi isEnum0(flags_t F)   { return (F & MS_0TYPE) == FF_0ENUM;  }


// Is the second operand a symbolic constant (enum member)?

inline bool idaapi isEnum1(flags_t F)   { return (F & MS_1TYPE) == FF_1ENUM;  }


// Is the first operand manually defined? (for kernel only)

inline bool idaapi isFop0(flags_t F)    { return (F & MS_0TYPE) == FF_0FOP;  }


// Is the second operand manually defined? (for kernel only)

inline bool idaapi isFop1(flags_t F)    { return (F & MS_1TYPE) == FF_1FOP;  }


// Is the first operand an offset within a struct?

inline bool idaapi isStroff0(flags_t F) { return (F & MS_0TYPE) == FF_0STRO;  }


// Is the second operand an offset within a struct?

inline bool idaapi isStroff1(flags_t F) { return (F & MS_1TYPE) == FF_1STRO;  }


// Is the first operand a stack variable?

inline bool idaapi isStkvar0(flags_t F) { return (F & MS_0TYPE) == FF_0STK;  }


// Is the second operand a stack variable?

inline bool idaapi isStkvar1(flags_t F) { return (F & MS_1TYPE) == FF_1STK;  }


// Is the first operand a floating point number?

inline bool idaapi isFloat0(flags_t F) { return (F & MS_0TYPE) == FF_0FLT;  }


// Is the second operand a floating point number?

inline bool idaapi isFloat1(flags_t F) { return (F & MS_1TYPE) == FF_1FLT;  }


// Is the first or second operand a number (i.e. binary,octal,decimal or hex?)

idaman bool ida_export isNum0(flags_t F);
idaman bool ida_export isNum1(flags_t F);


inline flags_t get_optype_flags0(flags_t F) { return F & MS_0TYPE; }
inline flags_t get_optype_flags1(flags_t F) { return F & MS_1TYPE; }
//-------------------------------------------------------------------------
//
//      The following 2 masks are used with operand numbers
//
#define OPND_OUTER      0x80            // outer offset base (combined with
                                        //  operand number)
                                        // used only in set,get,del_offset()
                                        // functions
#define OPND_MASK       0x07            // mask for operand number
#define OPND_ALL        OPND_MASK       // all operands

// Examine flags for the specified operand type.
// 'n' may be 0         - first operand
//            1         - second operand
//            OPND_ALL  - both operands: function returns 1 if the first
//                        OR the second operand satisfies the condition

idaman bool ida_export isDefArg(flags_t F,int n);          // is defined?
idaman bool ida_export isOff(flags_t F,int n);             // is offset?
idaman bool ida_export isChar(flags_t F,int n);            // is character constant?
idaman bool ida_export isSeg(flags_t F,int n);             // is segment?
idaman bool ida_export isEnum(flags_t F,int n);            // is enum?
idaman bool ida_export isFop(flags_t F,int n);             // is forced operand? (use is_forced_operand())
idaman bool ida_export isStroff(flags_t F,int n);          // is struct offset?
idaman bool ida_export isStkvar(flags_t F,int n);          // is stack variable?
idaman bool ida_export isFltnum(flags_t F,int n);          // is floating point number?
idaman bool ida_export isNum(flags_t F,int n);             // is number (bin,oct,dec,hex)?
idaman bool ida_export isVoid(ea_t ea,flags_t F,int n);    // is 'void' operand?


// (internal function) change type of operand(s)
//      ea   - linear address
//      type - new flag value (should be got from charflag(), numflag() and
//             similar functions. see below.)
//      n    - number of operand (0,1,-1)
// returns: 1-ok, 0-failed (applied to a tail byte)

idaman bool ida_export set_op_type(ea_t ea,flags_t type,int n);


// (internal function) destroys old type information and returns
// new flags value. this function is called from the kernel only.
//      ea   - linear address
//      type - new flag value (should be got from ....flag() functions)
//      n    - number of operand (0,1,-1)

idaman flags_t ida_export typeflag(ea_t ea,flags_t oldflag,flags_t type,int n);


// set operand type 'segment'
// if applied to unexplored bytes, converts them to 16/32bit word data
//      ea   - linear address
//      n    - number of operand (0,1,-1)
// returns 1-ok, 0-failure

idaman bool ida_export op_seg(ea_t ea,int n);


// set operand type 'enum'
// if applied to unexplored bytes, converts them to 16/32bit word data
//      ea   - linear address
//      n    - number of operand (0,1,-1)
//      id   - id of enum
//      serial - the serial number of the constant in the enumeration
//               usually 0. the serial numbers are used if the enumeration
//               contains several constants with the same value

// returns 1-ok, 0-failure

idaman bool ida_export op_enum(ea_t ea,int n,enum_t id, uchar serial);


// set enum id of 'enum' operand
//      ea   - linear address
//      n    - number of operand (0,1,-1)
//      id   - enumeration id
//      serial - the serial number of the constant in the enumeration
//               usually 0. the serial numbers are used if the enumeration
//               contains several constants with the same value
// this is low level function and shouldn't be used. use op_enum().

idaman void ida_export set_enum_id(ea_t ea, int n, enum_t id, uchar serial);


// get enum id of 'enum' operand
//      ea   - linear address
//      n    - number of operand (0,1,-1)
//      serial - pointer to variable to hold the serial number of the
//               constant in the enumeration
// returns: id of enum or BADNODE

idaman enum_t ida_export get_enum_id(ea_t ea,int n, uchar *serial);


// set operand type 'struct offset'
// if applied to unexplored bytes, converts them to 16/32bit word data
//      ea   - linear address
//      n    - number of operand (0,1,-1)
//      path - structure path (strpath). see nalt.hpp for more info.
//      path_len - length of the structure path
//      delta- struct offset delta. usually 0. denotes the difference
//             between the structure base and the pointer into the structure.
// returns 1-ok, 0-failure

idaman bool ida_export op_stroff(ea_t ea, int n, const tid_t *path, int path_len, adiff_t delta);


// set struct path for operand
//      ea   - linear address
//      n    - number of operand (0,1,-1)
//      path - structure path (strpath). see nalt.hpp for more info.
//      path_len - length of the structure path
//      delta- struct offset delta
// returns 1-ok, 0-failure

idaman bool ida_export set_stroff_path(ea_t ea, int n, const tid_t *path, int plen, adiff_t delta);


// get struct path of operand
//      ea   - linear address
//      n    - number of operand (0,1,-1)
//      path - buffer for structure path (strpath). see nalt.hpp for more info.
//      delta- struct offset delta
// returns: length of strpath

idaman int ida_export get_stroff_path(ea_t ea, int n, tid_t *path, adiff_t *delta);


// delete struct path information
//      ea   - linear address
//      n    - number of operand (0,1,-1)
// returns: true

idaman bool ida_export del_stroff_path(ea_t ea, int n);


// set operand type 'stack variable'
//      ea   - linear address
//      n    - number of operand (0,1,-1)
// returns 1-ok, 0-failure
// should be applied to an instruction within a function
// should be applied after creating a stack var using ua_stkvar()

idaman bool ida_export op_stkvar(ea_t ea,int n);


// Set forced operand
//      ea - linear address
//      n  - number of operand (0,1,2)
//      op - text of operand
// returns: 1-ok, 0-failure

idaman bool ida_export set_forced_operand(ea_t ea,int n,const char *op);


// Get forced operand
//      ea - linear address
//      n  - number of operand (0,1,2)
//      buf - output buffer, may be NULL
//      bufsize - size of output buffer
// Returns: size of forced operand or -1

idaman ssize_t ida_export get_forced_operand(ea_t ea, int n, char *buf, size_t bufsize);


// Is operand manually defined?
//      ea - linear address
//      n  - number of operand (0,1,2)

idaman bool ida_export is_forced_operand(ea_t ea, int n);


//-------------------------------------------------------------------------
//      Type information bits for flags
//      Values of these functions are used as input to set_op_type() function

inline flags_t idaapi charflag(void) { return FF_1CHAR|FF_0CHAR; }
inline flags_t idaapi offflag (void) { return FF_1OFF |FF_0OFF ; }
inline flags_t idaapi enumflag(void) { return FF_1ENUM|FF_0ENUM; }
inline flags_t idaapi stroffflag(void) { return FF_1STRO|FF_0STRO; }
inline flags_t idaapi stkvarflag(void) { return FF_1STK|FF_0STK; }
inline flags_t idaapi fltflag(void)  { return FF_1FLT |FF_0FLT;  }
inline flags_t idaapi segflag (void) { return FF_1SEG |FF_0SEG;  }
idaman flags_t ida_export numflag(void); // returns number of default base (bin,oct,dec,hex)
                                // the following 4 functions return number
                                // flag of the specified base, regardless
                                // of the current default base for the current
                                // processor. Better to use numflag(), than to
                                // use these functions:
inline flags_t idaapi hexflag (void) { return FF_1NUMH|FF_0NUMH; }
inline flags_t idaapi decflag (void) { return FF_1NUMD|FF_0NUMD; }
inline flags_t idaapi octflag (void) { return FF_1NUMO|FF_0NUMO; }
inline flags_t idaapi binflag (void) { return FF_1NUMB|FF_0NUMB; }

// the following functions set operand type.
// if they are applied to unexplored bytes, they convert them
//                              no segment   : fail
//                              16bit segment: to 16bit word data
//                              32bit segment: to dword
//      ea   - linear address
//      n    - number of operand (0,1,-1)
// return 1-ok, 0-failure

inline bool idaapi op_chr(ea_t ea,int n) { return set_op_type(ea,charflag(),n); }
inline bool idaapi op_num(ea_t ea,int n) { return set_op_type(ea,numflag (),n); }
inline bool idaapi op_hex(ea_t ea,int n) { return set_op_type(ea,hexflag (),n); }
inline bool idaapi op_dec(ea_t ea,int n) { return set_op_type(ea,decflag (),n); }
inline bool idaapi op_oct(ea_t ea,int n) { return set_op_type(ea,octflag (),n); }
inline bool idaapi op_bin(ea_t ea,int n) { return set_op_type(ea,binflag (),n); }
inline bool idaapi op_flt(ea_t ea,int n) { return set_op_type(ea,fltflag (),n); }

// Remove type information about the specified operand from the database
//      ea   - linear address
//      n    - number of operand (0,1,-1)
// returns: 1

idaman bool ida_export noType(ea_t ea,int n);

// Get default base of number for the current processor
// returns: 2,8,10,16

idaman int ida_export getDefaultRadix(void);


// Get radix of the operand, in: flags
//      F    - flags
//      n    - number of operand (0,1,-1)
// returns: 2,8,10,16
// if the operand is not a number, returns getDefaultRadix()

idaman int ida_export getRadix(flags_t F,int n);


// Get radix of the operand, in: address
//      ea   - linear address
//      n    - number of operand (0,1,-1)
// returns: 2,8,10,16
// if the operand is not a number, returns getDefaultRadix()

idaman int ida_export getRadixEA(ea_t ea,int n);


//-------------------------------------------------------------------------
//
//      Bits for DATA bytes
//      They specify type of data (byte, word, dword, etc)
//

#define DT_TYPE 0xF0000000L             // Mask for DATA typing

#define FF_BYTE     0x00000000L         // byte
#define FF_WORD     0x10000000L         // word
#define FF_DWRD     0x20000000L         // double word
#define FF_QWRD     0x30000000L         // quadro word
#define FF_TBYT     0x40000000L         // tbyte
#define FF_ASCI     0x50000000L         // ASCII ?
#define FF_STRU     0x60000000L         // Struct ?
#define FF_OWRD     0x70000000L         // octaword (16 bytes)
#define FF_FLOAT    0x80000000L         // float
#define FF_DOUBLE   0x90000000L         // double
#define FF_PACKREAL 0xA0000000L         // packed decimal real
#define FF_ALIGN    0xB0000000L         // alignment directive
#define FF_3BYTE    0xC0000000L         // 3-byte data (only with support from
                                        // the processor module)

// The following functions return bit masks to be used as input for doData()

inline flags_t idaapi codeflag(void) { return FF_CODE; }
inline flags_t idaapi byteflag(void) { return FF_DATA|FF_BYTE; }
inline flags_t idaapi wordflag(void) { return FF_DATA|FF_WORD; }
inline flags_t idaapi dwrdflag(void) { return FF_DATA|FF_DWRD; }
inline flags_t idaapi qwrdflag(void) { return FF_DATA|FF_QWRD; }
inline flags_t idaapi owrdflag(void) { return FF_DATA|FF_OWRD; }
inline flags_t idaapi tbytflag(void) { return FF_DATA|FF_TBYT; }
inline flags_t idaapi asciflag(void) { return FF_DATA|FF_ASCI; }
inline flags_t idaapi struflag(void) { return FF_DATA|FF_STRU; }
inline flags_t idaapi alignflag(void) { return FF_DATA|FF_ALIGN; }
inline flags_t idaapi floatflag(void) { return FF_DATA|FF_FLOAT; }
inline flags_t idaapi doubleflag(void) { return FF_DATA|FF_DOUBLE; }
inline flags_t idaapi tribyteflag(void) { return FF_DATA|FF_3BYTE; }
inline flags_t idaapi packrealflag(void) { return FF_DATA|FF_PACKREAL; }

// Convert to data (byte,word,dword,etc)
//      ea   - linear address
//      dataflag - type of data. Value of function byteflag(), wordflag(), etc.
//      size - size of array in bytes. should be divisible by the size of
//             one item of the specified type.
//      tid  - type id. If the specified type is a structure,
//             then tid is structure id. Otherwise should be BADNODE.
// This function may be used to create arrays.
// return 1-ok, 0-failure

idaman bool ida_export do_data_ex(
        ea_t ea,
        flags_t dataflag,
        asize_t size,
        tid_t tid);


// Convert to specified data type. (convenience functions)
//      ea     - linear address
//      length - size of array in bytes. should be divisible by the size of
//               one item of the specified type.
// return 1-ok, 0-failure

inline bool idaapi doByte(ea_t ea, asize_t length)     { return do_data_ex(ea, FF_BYTE, length, BADNODE); }
inline bool idaapi doWord(ea_t ea, asize_t length)     { return do_data_ex(ea, FF_WORD, length, BADNODE); }
inline bool idaapi doDwrd(ea_t ea, asize_t length)     { return do_data_ex(ea, FF_DWRD, length, BADNODE); }
inline bool idaapi doQwrd(ea_t ea, asize_t length)     { return do_data_ex(ea, FF_QWRD, length, BADNODE); }
inline bool idaapi doOwrd(ea_t ea, asize_t length)     { return do_data_ex(ea, FF_OWRD, length, BADNODE); }
inline bool idaapi doTbyt(ea_t ea, asize_t length)     { return do_data_ex(ea, FF_TBYT, length, BADNODE); }
inline bool idaapi doFloat(ea_t ea, asize_t length)    { return do_data_ex(ea, FF_FLOAT, length, BADNODE); }
inline bool idaapi doDouble(ea_t ea, asize_t length)   { return do_data_ex(ea, FF_DOUBLE, length, BADNODE); }
inline bool idaapi doPackReal(ea_t ea, asize_t length) { return do_data_ex(ea, FF_PACKREAL, length, BADNODE); }
inline bool idaapi doASCI(ea_t ea, asize_t length)     { return do_data_ex(ea, FF_ASCI, length, BADNODE); }
inline bool idaapi do3byte(ea_t ea, asize_t length)    { return do_data_ex(ea, FF_3BYTE, length, BADNODE); }
inline bool idaapi doStruct(ea_t ea, asize_t length,tid_t tid) { return do_data_ex(ea, FF_STRU, length, tid); }

idaman bool ida_export doAlign(ea_t ea, asize_t length,int alignment);
                                                // alignment: 0 or 2..32
                                                // if it is 0, is will be calculated
idaman int  ida_export calc_min_align( asize_t length);    // returns: 1..32
idaman int  ida_export calc_max_align(ea_t endea);     // returns: 0..32
idaman int  ida_export calc_def_align(ea_t ea, int mina, int maxa); // calc default alignment
idaman bool ida_export do16bit(ea_t ea, asize_t length);  // convert to 16-bit quanztity
                                                // (depending on the byte size)


// Check flags for the specified data type

inline bool idaapi isByte  (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_BYTE; }
inline bool idaapi isWord  (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_WORD; }
inline bool idaapi isDwrd  (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_DWRD; }
inline bool idaapi isQwrd  (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_QWRD; }
inline bool idaapi isOwrd  (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_OWRD; }
inline bool idaapi isTbyt  (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_TBYT; }
inline bool idaapi isFloat (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_FLOAT; }
inline bool idaapi isDouble(flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_DOUBLE; }
inline bool idaapi isPackReal(flags_t F) { return isData(F) && (F & DT_TYPE) == FF_PACKREAL; }
inline bool idaapi isASCII (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_ASCI; }
inline bool idaapi isStruct(flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_STRU; }
inline bool idaapi isAlign (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_ALIGN; }
inline bool idaapi is3byte (flags_t F)   { return isData(F) && (F & DT_TYPE) == FF_3BYTE; }

inline bool idaapi f_isByte  (flags_t F, void *)   { return isByte(F); }
inline bool idaapi f_isWord  (flags_t F, void *)   { return isWord(F); }
inline bool idaapi f_isDwrd  (flags_t F, void *)   { return isDwrd(F); }
inline bool idaapi f_isQwrd  (flags_t F, void *)   { return isQwrd(F); }
inline bool idaapi f_isOwrd  (flags_t F, void *)   { return isOwrd(F); }
inline bool idaapi f_isTbyt  (flags_t F, void *)   { return isTbyt(F); }
inline bool idaapi f_isFloat (flags_t F, void *)   { return isFloat(F); }
inline bool idaapi f_isDouble(flags_t F, void *)   { return isDouble(F); }
inline bool idaapi f_isPackReal(flags_t F, void *) { return isPackReal(F); }
inline bool idaapi f_isASCII (flags_t F, void *)   { return isASCII(F); }
inline bool idaapi f_isStruct(flags_t F, void *)   { return isStruct(F); }
inline bool idaapi f_isAlign (flags_t F, void *)   { return isAlign(F); }
inline bool idaapi f_is3byte (flags_t F, void *)   { return is3byte(F); }

inline bool idaapi is_same_data_type(flags_t F1, flags_t F2) { return ((F1 ^ F2) & DT_TYPE) == 0; }


// determine maximum length of ascii string
//      ea           - starting address
//      strtype      - string type. ASCSTR_... constant (see nalt.hpp)
//      ignore_heads - 1: don't stop if another data item is encountered
//                        only the byte values will be used to determine
//                        the string length
//                     0: take into account other data items
//                        a defined data item or instruction will truncate
//                        the string
// Returns length of the string in bytes.

idaman size_t ida_export get_max_ascii_length(
        ea_t ea,
        long strtype,
        bool ignore_heads=false);


// Get contents of ascii string
// This function returns the displayed part of the string
// It works even if the string has not been created in the database yet.
//      ea      - linear address of the string
//      len     - length of the string in bytes
//      type    - type of the string
//      buf     - output buffer
//      bufsize - size of output buffer
// returns 1-ok, 0-ascii string is too long and was truncated

idaman bool ida_export get_ascii_contents(
        ea_t ea,
        size_t len,
        long type,
        char *buf,
        size_t bufsize);


// convert to ascii string and give a meaningful name
// 'start' may be higher than 'end', the kernel will swap them in this case
//      start - starting address
//      len   - length of the string in bytes.
//              if 0, then get_max_ascii_length() will be used
//              to determine the length
//      strtype - string type. ASCSTR_... constant (see nalt.hpp)
// returns: 1-ok,0-failure

idaman bool ida_export make_ascii_string(ea_t start, size_t len, long strtype);


// print the string type name
//      long - the string type
//      buf  - the output buffer
//      bufsize - sizeof(buf)
// returns: ptr to buf

idaman char *ida_export print_ascii_string_type(
        char *buf,
        size_t bufsize,
        long strtype);


// Get additional information about a type
//      ea      - linear address of item
//      n       - number of operand, 0 or 1
//      flags   - flags of the item
//      buf     - buffer to receive the result. may not be NULL
// Returns: NULL - no additional type information

idaman typeinfo_t *ida_export get_typeinfo(ea_t ea,
        int n,
        flags_t flags,
        typeinfo_t *buf);


// Set additional information about a data type
//      ea      - linear address of item
//      n       - number of operand, 0 or 1
//      flags   - flags of the item
//      ti      - additional type information
// This function is a low level one. Only the kernel should use it.
// Returns: 1-ok, 0-ti is NULL while it shouldn't be NULL

idaman bool ida_export set_typeinfo(ea_t ea,
        int n,
        flags_t flag,
        const typeinfo_t *ti);


// Delete additional information about a data type
//      ea      - linear address of item
//      n       - number of operand, 0 or 1
//      flags   - flags of the item
// This function is a low level one. Only the kernel should use it.

void del_typeinfo(ea_t ea, flags_t flag);
void del_operand_typeinfo(ea_t ea, int n, flags_t flag);


// get size of data type specified in flags 'F'.
//      F    - flags
//      ti   - additional information about the data type. For example,
//             if the current item is a structure instance,
//             then ti->tid is structure id. Otherwise is ignored (may be NULL).
// returns:     byte - 1
//              word - 2
//              etc.
// If flags doesn't specify a data, then return 1

idaman asize_t ida_export get_data_type_size(flags_t flags,const typeinfo_t *ti);
inline asize_t idaapi get_data_type_size_by_flags(ea_t ea, flags_t flags)
{
  typeinfo_t ti;
  return get_data_type_size(flags, get_typeinfo(ea, 0, flags, &ti));
}


// Can define item (instruction/data) of the specified 'length'
// starting at 'ea'?
//      flags - if not 0, then the kernel will ignore the data types
//              specified by the flags and destroy them. For example:
//                 1000 dw 5
//                 1002 db 5 ; undef
//                 1003 db 5 ; undef
//                 1004 dw 5
//                 1006 dd 5
//              can_define_item(1000,6,0) - false because of dw at 1004
//              can_define_item(1000,6,wordflag()) - true, word at 1004 is destroyed
// Returns: 1-yes, 0-no
// This function may return 0 if:
//      - a new item would cross segment boundaries
//      - a new item would overlap with existing items (except items specified by 'flags')
// NOTE: if there is an item starting at 'ea', this function ignores it
// NOTE: this function converts to unexplored all encountered data items
//       with fixup information. Should be fixed in the future.

idaman bool ida_export can_define_item(ea_t ea, asize_t length, flags_t flags);


//-------------------------------------------------------------------------
//
//      Bits for CODE bytes
//      They specify characteristics of instructions
//

#define MS_CODE 0xF0000000L
#define FF_FUNC 0x10000000L             // function start?
//              0x20000000L             // not used
#define FF_IMMD 0x40000000L             // Has Immediate value ?
#define FF_JUMP 0x80000000L             // Has jump table or switch_info?

// Convert to an instruction
// (internal function, should not be used in modules, never)
// use ua_code() instead.
//      ea     - linear address
//      length - length of instruction
// Returns: 1-ok, 0-failure

bool doCode(ea_t ea,int length);


// Has immediate value?

inline bool idaapi isImmd(flags_t F)      { return isCode(F) && (F & FF_IMMD) != 0; }


// Is function start?

inline bool idaapi isFunc(flags_t F)      { return isCode(F) && (F & FF_FUNC) != 0; }


// Set 'has immediate operand' flag

idaman void ida_export doImmd(ea_t ea);


// Clear 'has immediate operand' flag

idaman void ida_export noImmd(ea_t ea);


//-------------------------------------------------------------------------
//
//      Bits for TAIL bytes
//      Modules don't change or use tail bytes, so all the following
//      definitions are for the kernel only.
//

#define MS_TAIL 0xFFF00000LU            // Mask of tail byte bits
#define TL_TSFT (4*5)                   // number of bits to shift to get
                                        // tail offsets
#define TL_TOFF MS_TAIL                 // Offset to next NOT-tail byte
#define MAX_TOFF (TL_TOFF>>TL_TSFT)     // Max offset can be written to flags

inline ushort idaapi gettof(flags_t F) { return ushort((F & TL_TOFF) >> TL_TSFT); }

//--------------------------------------------------------------------------
//      I N D E N T E D  C O M M E N T S
//--------------------------------------------------------------------------

// Set an indented comment
//      ea     - linear address
//      comm   - comment string
//      rptble - is repeatable?
// returns: 1-ok, 0-failure

idaman bool ida_export set_cmt(ea_t ea, const char *comm, bool rptble);


// Get an indented comment
//      ea     - linear address. may point to tail byte, the function
//               will find start of the item
//      rptble - get repeatable comment?
//      buf - output buffer, may be NULL
//      bufsize - size of output buffer
// Returns: size of comment or -1

idaman ssize_t ida_export get_cmt(ea_t ea, bool rptble, char *buf, size_t bufsize);


// Get a repeatable comment of any type (indented or function comment)
// This function is used to display an indented comment if no regular
// (non-repeatable) comment is present.
//      ea     - linear address. may point to tail byte, the function
//               will find start of the item
// returns: comment or NULL. The caller must qfree() the result.

char *get_repeatable_cmt(ea_t ea);


// Get any indented comment (regular or repeatable indented or function)
// This function is used to display an indented comment for an item.
// It looks for a regular comment and calls get_rpt_cmt() if it is not found.
//      ea      - linear address
//      cmttype - will contain color of the comment. The color depends on
//                the type of the comment.
// returns: comment or NULL. The caller must qfree() the result.

char *get_any_indented_cmt(ea_t ea, color_t *cmttype);


// Append to an indented comment
// Creates a new comment if none exists.
// Appends a newline character and the specified string otherwise.
//      ea      - linear address
//      str     - comment string to append
//      rptble  - append to repeatable comment?
// returns: 1-ok, 0-failure

idaman bool ida_export append_cmt(ea_t ea, const char *str, bool rptble);


// Delete an indented comment attached to an instruction
// when the instruction is deleted

extern bool del_code_comments;


//--------------------------------------------------------------------------
//      S E A R C H  F U N C T I O N S
//--------------------------------------------------------------------------
// Find forward a byte with the specified value (only 8-bit value from the database)
//      ea         - linear address
//      size       - number of bytes to inspect
//      value      - value to find
//      sense_case - case sensitive search
// returns: address of byte or BADADDR
// example: ea=4 size=3 will inspect addresses 4, 5, and 6

idaman ea_t ida_export find_byte(ea_t sEA, asize_t size, uchar value, bool sense_case);


// Find reverse a byte with the specified value (only 8-bit value from the database)
//      ea         - linear address
//      size       - number of bytes to inspect
//      value      - value to find
//      sense_case - case sensitive search
// returns: address of byte or BADADDR
// example: ea=4 size=3 will inspect addresses 6, 5, and 4

idaman ea_t ida_export find_byter(ea_t sEA, asize_t size, uchar value, bool sense_case);


// search for a string in the program
//      startEA - linear address, start of area to search
//      endEA   - linear address, end of area to search (exclusive)
//      image   - stream of bytes to search
//      mask    - array of 1/0 bytes, it's length is 'len'. 1 means to perform
//                the comparision of the corresponding byte. 0 means not to perform.
//                if mask == NULL, then all bytes of 'image' will be compared.
//      len     - length of srchStr in bytes
//      step    - direction of search
//      flags   - search flags. combination of:
//                  BIN_SEARCH_CASE    - case sensitive
//                  BIN_SEARCH_NOCASE  - case insensitive
//                  BIN_SEARCH_NOBREAK - don't check for Ctrl-Break
// returns BADADDR (if pressed Ctrl-Break or not found) or string address.

idaman ea_t ida_export bin_search(
                 ea_t startEA,         // area to search
                 ea_t endEA,
                 const uchar *image,   // string to search
                 const uchar *mask,    // comparision mask
                 size_t len,           // length of string to search
                 int step,             // direction:
#define BIN_SEARCH_FORWARD      1
#define BIN_SEARCH_BACKWARD     (-1)
                 int flags);
#define BIN_SEARCH_CASE         0x01
#define BIN_SEARCH_NOCASE       0x00
#define BIN_SEARCH_NOBREAK      0x02


// Compare 'len' bytes of the program starting from 'ea' with 'image'.
//      ea         - linear address
//      image      - bytes to compare with
//      len        - length of block to compare in bytes.
//      sense_case - case-sensitive comparison?
//      mask       - array of 1/0 bytes, it's length is 'len'. 1 means to perform
//                   the comparision of the corresponding byte. 0 means not to perform.
//                   if mask == NULL, then all bytes of 'image' will be compared.
//                   if mask == SKIP_FF_MASK than 0xFF bytes will be skipped
// Returns: 1- equal, 0-no

idaman bool ida_export equal_bytes(
        ea_t ea,
        const uchar *image,
        const uchar *mask,
        size_t len,
        bool sense_case);
#define SKIP_FF_MASK  ((const uchar *)0xFF)


//--------------------------------------------------------------------------
idaman const ulong ida_export_data power2[32];    // Powers of 2, from 2^0 to 2^31
idaman const ulong ida_export_data lowbits[33];   // Low-order bits, from 0 to 32


//--------------------------------------------------------------------------
//      D E B U G G E R  M E M O R Y  F U N C T I O N S  F O R  U I
//--------------------------------------------------------------------------

// Set the memory information source for IDA kernel
// This function allows the kernel to use information coming from somewhere
// else than the database (from the debugger, for example)
//   dbg_get_memory_config - returns current memory configuration
//                           in the dynamic memory allocated by qalloc()
//                           The kernel will qfree() is automatically
//                           n - number of area_t elements in the answer
//                           If dbg_get_memory_config == NULL, then the debugged
//                           process memory is not used
//   memory_read           - read bytes from the debugged process memory
//   memory_write          - write bytes to the debugged process memory
//                           (don't forget to call invalidate_dbgmem_contents() from it)

idaman void ida_export set_dbgmem_source(
        area_t *(idaapi*dbg_get_memory_config)(int *n),
        int (idaapi*memory_read) (ea_t ea, void *buffer, int size),
        int (idaapi*memory_write)(ea_t ea, const void *buffer, int size));

// Invalidate the debugged process memory configuration
// Call this function if the debugged process might have changed its memory
// layout (allocated more memory, for example)

idaman void ida_export invalidate_dbgmem_config(void);


// Invalidate the debugged process memory contents
// Call this function each time the process has been stopped or the process
// memory is modified
// if ea == BADADDR, then the whole memory contents will be invalidated

idaman void ida_export invalidate_dbgmem_contents(ea_t ea, asize_t size);


// Is the debugger currently running?

idaman bool ida_export is_debugger_on(void);


//------------------------------------------------------------------------
//      H I D D E N  A R E A S
//------------------------------------------------------------------------

// Hidden areas - address ranges which can be replaced by their descriptions
// There is also a possibility to hide individual items completely (nalt.hpp, hide_item)

struct hidden_area_t : public area_t
{
  // After modifying any of these fields please call update_hidden_area()
  char *description;    // description to display if the area is collapsed
  char *header;         // header lines to display if the area is expanded
  char *footer;         // footer lines to display if the area is expanded
  bool visible;         // the area state
  bgcolor_t color;      // area color
};

idaman areacb_t ida_export_data hidden_areas;

// Update hidden area information in the database
// You can not use this function to change the area boundaries
//      ha - area to update
// returns: 1-ok,0-failure

inline bool idaapi update_hidden_area(hidden_area_t *ha)
  { invalidate_visea_cache(); return hidden_areas.update(ha) != 0; }

// Mark a range of addresses as hidden
//      ea1         - linear address of start of the address range
//      ea2         - linear address of end of the address range
//      description, header, footer - area parameters
// The area will be created in the invisible state with the default color
// returns: 1-ok,0-failure

idaman bool ida_export add_hidden_area(ea_t ea1,
                                       ea_t ea2,
                                       const char *description,
                                       const char *header,
                                       const char *footer,
                                       bgcolor_t color);


// Get pointer to hidden area structure, in: linear address
//      ea - any address in the hidden area

inline hidden_area_t *idaapi get_hidden_area(ea_t ea)
  { return (hidden_area_t *)hidden_areas.get_area(ea); }


// Get pointer to hidden area structure, in: number of hidden area
//      n - number of hidden area, is in range 0..get_hidden_area_qty()-1

inline hidden_area_t *idaapi getn_hidden_area(int n)
  { return (hidden_area_t *)hidden_areas.getn_area(n); }


// Get number of hidden areas

inline int idaapi get_hidden_area_qty(void)
  { return hidden_areas.get_area_qty(); }


// Get number of a hidden area
//      ea - any address in the hidden area
// Returns number of hidden area (0..get_hidden_area_qty()-1)

inline int idaapi get_hidden_area_num(ea_t ea)
  { return hidden_areas.get_area_num(ea); }


// Get pointer to previous hidden area
//      ea - any address in the program
// Returns ptr to hidden area or NULL if previous hidden area doesn't exist

inline hidden_area_t *idaapi get_prev_hidden_area(ea_t ea)
  { return getn_hidden_area(hidden_areas.get_prev_area(ea)); }


// Get pointer to next hidden area
//      ea - any address in the program
// Returns ptr to hidden area or NULL if next hidden area doesn't exist

inline hidden_area_t *idaapi get_next_hidden_area(ea_t ea)
  { return getn_hidden_area(hidden_areas.get_next_area(ea)); }


// Delete hidden area
//      ea - any address in the hidden area
// returns: 1-ok,0-failure

inline bool idaapi del_hidden_area(ea_t ea)
  { return hidden_areas.del_area(ea); }


//--------------------------------------------------------------------------
//      L O W  L E V E L  F U N C T I O N S (kernel only)
//--------------------------------------------------------------------------

inline void idaapi doFlow(ea_t ea)      { setFlbits(ea, FF_FLOW);  }
inline void idaapi noFlow(ea_t ea)      { clrFlbits(ea, FF_FLOW);  }

void doRef(ea_t ea);
void noRef(ea_t ea);

inline void idaapi doExtra(ea_t ea) { setFlbits(ea, FF_LINE); }
inline void idaapi noExtra(ea_t ea) { clrFlbits(ea, FF_LINE); }

//--------------------------------------------------------------------------
asize_t coagulate(ea_t ea);
bool coagulate_dref(ea_t From, ea_t To, flags_t F, bool may_define);

//--------------------------------------------------------------------------
inline ea_t idaapi get_item_head(ea_t ea)
{
  if ( isTail(get_flags_novalue(ea)) )
    ea = prev_not_tail(ea);
  return ea;
}

// init/save/term work with hidden areas
// These functions are called from the kernel at the start.

void init_hidden_areas(void);
void save_hidden_areas(void);
void term_hidden_areas(void);


// Move chunk of flags information to the new address (kernel only)
int check_move_args(ea_t from, ea_t to, asize_t size); // returns VAMOVE_...
int movechunk(ea_t from, ea_t to, asize_t size);

// internal kernel functions to lock the debugger memory configuration updates
void lock_dbgmem_config(void);
void unlock_dbgmem_config(void);


//--------------------------------------------------------------------------
// ida virtual memory configuration:
// (not reachable from modules)
extern ushort ida_vpagesize;    // Virtual memory page size       (*.id1)
extern ushort ida_vpages;       // Size of virtual memory window  (*.id1)
extern ushort ida_npagesize;    // Name pointers page size        (*.nam)
extern ushort ida_npages;       // Number of name pointer pages   (*.nam)


// initialize virtual array (*.id1)
//     input_size - input file size (used only if database didn't exist before)

void FlagsInit(ulong input_size);


// terminate virtual array

void FlagsTerm(void);


// make virtual array empty.
// returns: 1-ok, 0-failure(never should occur)

bool FlagsReset(void);


#ifndef BYTES_SOURCE    // undefined bit masks so no one can use them directly
#undef MS_VAL
#undef FF_IVL
#undef MS_CLS
#undef FF_CODE
#undef FF_DATA
#undef FF_TAIL
#undef FF_UNK
#undef MS_COMM
#undef FF_COMM
#undef FF_REF
#undef FF_LINE
#undef FF_NAME
#undef FF_LABL
#undef FF_ANYNAME
#undef FF_FLOW
#undef FF_SIGN
#undef FF_BNOT
#undef FF_VAR
#undef MS_0TYPE
#undef FF_0VOID
#undef FF_0NUMH
#undef FF_0NUMD
#undef FF_0CHAR
#undef FF_0SEG
#undef FF_0OFF
#undef FF_0NUMB
#undef FF_0NUMO
#undef FF_0ENUM
#undef FF_0FOP
#undef FF_0STRO
#undef FF_0STK
#undef FF_0HIGH
#undef MS_1TYPE
#undef FF_1VOID
#undef FF_1NUMH
#undef FF_1NUMD
#undef FF_1CHAR
#undef FF_1SEG
#undef FF_1OFF
#undef FF_1NUMB
#undef FF_1NUMO
#undef FF_1ENUM
#undef FF_1FOP
#undef FF_1STRO
#undef FF_1STK
#undef FF_1HIGH
#undef DT_TYPE
#undef FF_BYTE
#undef FF_WORD
#undef FF_DWRD
#undef FF_QWRD
#undef FF_FLOAT
#undef FF_DOUBLE
#undef FF_TBYT
#undef FF_PACKREAL
#undef FF_ASCI
#undef FF_STRU
#undef FF_ALIGN
#undef FF_3BYTE
#undef MS_CODE
#undef FF_FUNC
#undef FF_IMMD
//#undef FF_JUMP
#undef MS_TAIL
#undef TL_TSFT
#undef TL_TOFF
#undef MAX_TOFF
#endif // BYTES_SOURCE

#pragma pack(pop)
#endif // BYTES_HPP
