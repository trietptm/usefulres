/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-99 by Ilfak Guilfanov, <ig@datarescue.com>
 *      ALL RIGHTS RESERVED.
 *
 */

#ifndef _XREF_HPP
#define _XREF_HPP
#pragma pack(push, 1)           // IDA uses 1 byte alignments!

//
//      This file contains functions that deal with cross-references.
//      There are 2 types of xrefs: CODE and DATA references.
//      All xrefs are kept in the bTree except ordinary execution flow
//      to the next instrction. Ordinary execution flow to
//      the next instruction is kept in flags (see bytes.hpp)
//      Cross-references are automatically sorted.
//

class func_t;   // #include <funcs.hpp>
class member_t; // #include <struct.hpp>

//---------------------------------------------------------------------------
//      C R O S S - R E F E R E N C E   T Y P E S
//---------------------------------------------------------------------------

// IDA handles the xrefs automatically and may delete an xref
// added by the user if it does not contain the fl_USER bit

// The following CODE xref types are defined:

enum cref_t
{
  fl_U,                         // unknown -- for compatibility with old
                                // versions. Should not be used anymore.
  fl_CF = 16,                   // Call Far
                                // This xref creates a function at the
                                // referenced location
  fl_CN,                        // Call Near
                                // This xref creates a function at the
                                // referenced location
  fl_JF,                        // Jump Far
  fl_JN,                        // Jump Near
  fl_USobsolete,                // User specified (obsolete)
  fl_F,                         // Ordinary flow: used to specify execution
                                // flow to the next instruction.
};



// The following DATA xref types are defined:

enum dref_t
{
  dr_U,                         // Unknown -- for compatibility with old
                                // versions. Should not be used anymore.
  dr_O,                         // Offset
                                // The reference uses 'offset' of data
                                // rather than its value
                                //    OR
                                // The reference appeared because the "OFFSET"
                                // flag of instruction is set.
                                // The meaning of this type is IDP dependent.
  dr_W,                         // Write access
  dr_R,                         // Read access
  dr_T,                         // Text (for forced operands only)
                                // Name of data is used in manual operand
  dr_I,                         // Informational
                                // (a derived java class references its base
                                //  class informatonally)
};

#define XREF_USER       0x20    // User specified xref
                                // This xref will not be deleted by IDA
                                // This bit should be combined with
                                // the existing xref types (cref_t & dref_t)
#define XREF_TAIL       0x40    // refernce to tail byte in extrn symbols

idaman char ida_export xrefchar(char xrtype);     // get character by XRtype

//---------------------------------------------------------------------------
//      A D D / D E L E T E   C R O S S - R E F E R E N C E S
//---------------------------------------------------------------------------

// Create a code cross-reference
//      from    - linear address of referencing instruction
//      to      - linear address of referenced  instruction
//      type    - cross-reference type

idaman void ida_export add_cref(ea_t from,ea_t to,cref_t type);


// Delete a code cross-reference
//      from    - linear address of referencing instruction
//      to      - linear address of referenced  instruction
//      expand  - 1: plan to delete the referenced instruction if it has
//                   no more references.
//                0:-don't delete the referenced instruction even if no
//                   more cross-references point to it
// returns: 1-the referenced instruction will     be deleted
//          0-the referenced instruction will not be deleted

idaman int ida_export del_cref(ea_t from,ea_t to,int expand);


// Create a data cross-reference
//      from    - linear address of referencing instruction or data
//      to      - linear address of referenced  data
//      type    - cross-reference type

idaman void ida_export add_dref(ea_t from,ea_t to,dref_t type);


// Delete a data cross-reference
//      from    - linear address of referencing instruction or data
//      to      - linear address of referenced  data

idaman void ida_export del_dref(ea_t from,ea_t to);

//-------------------------------------------------------------------------
//      E N U M E R A T E   ( G E T )   C R O S S - R E F E R E N C E S
//-------------------------------------------------------------------------

// The following structure and its four member functions
// represent the easiest way to access cross-references from the given
// address.
// For example:
//
//      xrefblk_t xb;
//      for ( bool ok=xb.first_from(ea, XREF_ALL); ok; ok=xb.next_from() )
//      {
//         // xb.to - contains the referenced address
//      }
//
// or:
//
//      xrefblk_t xb;
//      for ( bool ok=xb.first_to(ea, XREF_ALL); ok; ok=xb.next_to() )
//      {
//         // xb.from - contains the referencing address
//      }
//
// First all code references will be returned, then all data references
// If you need only code references, stop calling next() as soon as you get a dref
// If you need only data references, pass XREF_DATA flag to first()
// You may not modify contents xrefblk_t structure! It is read only.

// Helper functions. Should not be called directly!
struct xrefblk_t;
idaman bool ida_export xrefblk_t_first_from(xrefblk_t *,ea_t from,int flags);
idaman bool ida_export xrefblk_t_next_from(xrefblk_t *);
idaman bool ida_export xrefblk_t_first_to(xrefblk_t *,ea_t to,int flags);
idaman bool ida_export xrefblk_t_next_to(xrefblk_t *);


struct xrefblk_t        // structure to enumerate all xrefs
{
  ea_t from;            // the referencing address - filled by first_to(),next_to()
  ea_t to;              // the referenced address - filled by first_from(), next_from()
  uchar iscode;         // 1-is code reference; 0-is data reference
  uchar type;           // type of the last retured reference (cref_t & dref_t)
  uchar user;           // 1-is used defined xref, 0-defined by ida

// The following functions return: 1-ok, 0-no (more) xrefs
// They first return code references, then data references.
// If you need only code references, you need to check 'iscode' after each call.
// If you need only data references, use XREF_DATA bit.
// flags may be any combination of the following bits:
#define XREF_ALL        0x00            // return all references
#define XREF_FAR        0x01            // don't return ordinary flow xrefs
#define XREF_DATA       0x02            // return data references only
  bool first_from(ea_t from,int flags)   // get first reference from...
    { return xrefblk_t_first_from(this, from, flags); }
  bool next_from(void)                   // get next reference from...
    { return xrefblk_t_next_from(this); }
  bool first_to(ea_t to,int flags)       // get first reference to...
    { return xrefblk_t_first_to(this, to, flags); }
  bool next_to(void)                     // get next reference to....
    { return xrefblk_t_next_to(this); }
  bool next_from(ea_t from, ea_t _to, int flags)
  {
    if ( first_from(from, flags) )
    {
      to = _to;
      return next_from();
    }
    return false;
  }
  bool next_to(ea_t _from, ea_t to, int flags)
  {
    if ( first_to(to, flags) )
    {
      from = _from;
      return next_to();
    }
    return false;
  }
};

//-------------------------------------------------------------------------

// This variable will contains type of the last xref returned
// by the following functions. It is not exported, so if you need to know
// the cross reference type, please use the xrefblk_t structure to enumerate
// the cross references.

extern char lastXR;


// Get first data referenced from the specified address
//      from    - linear address of referencing instruction or data
// returns: linear address of first (lowest) data referenced from
//            the specified address.
//            The 'lastXR' variable contains type of the reference
//          BADADDR if the specified instruction/data doesn't reference
//            to anything.

idaman ea_t ida_export get_first_dref_from(ea_t from);


// Get next data referenced from the specified address
//      from    - linear address of referencing instruction or data
//      current - linear address of current referenced data
//                This value is returned by get_first_dref_from() or
//                previous call to get_next_dref_from() functions.
// returns: linear address of next data or BADADDR
//          The 'lastXR' variable contains type of the reference

idaman ea_t ida_export get_next_dref_from(ea_t from,ea_t current);


// Get address of instruction/data referencing to the specified data
//      to      - linear address of referencing instruction or data
// returns: BADADDR if nobody refers to the specified data
//          The 'lastXR' variable contains type of the reference

idaman ea_t ida_export get_first_dref_to(ea_t to);


// Get address of instruction/data referencing to the specified data
//      to      - linear address of referencing instruction or data
//      current - current linear address.
//                This value is returned by get_first_dref_to() or
//                previous call to get_next_dref_to() functions.
// returns: BADADDR if nobody refers to the specified data
//          The 'lastXR' variable contains type of the reference

idaman ea_t ida_export get_next_dref_to(ea_t to,ea_t current);


// Get first instruction referenced from the specified instruction
// If the specified instruction passes execution to the next instruction
// then the next instruction is returned. Otherwise the lowest referenced
// address is returned (remember that xrefs are kept sorted!)
//      from    - linear address of referencing instruction
// returns: first referenced address.
//          The 'lastXR' variable contains type of the reference
// if the specified instruction doesn't reference to other instructions
// then returns BADADDR.

idaman ea_t ida_export get_first_cref_from(ea_t from);


// Get next instruction referenced from the specified instruction
//      from    - linear address of referencing instruction
//      current - linear address of current referenced instruction
//                This value is returned by get_first_cref_from() or
//                previous call to get_next_cref_from() functions.
// returns: next referenced address or BADADDR.
//          The 'lastXR' variable contains type of the reference

idaman ea_t ida_export get_next_cref_from(ea_t from,ea_t current);


// Get first instruction referencing to the specified instruction
// If the specified instruction may be executed immediately after its
// previous instruction
// then the previous instruction is returned. Otherwise the lowest referencing
// address is returned (remember that xrefs are kept sorted!)
//      to      - linear address of referenced instruction
// returns: linear address of the first referencing instruction or BADADDR.
//          The 'lastXR' variable contains type of the reference

idaman ea_t ida_export get_first_cref_to(ea_t to);


// Get next instruction referencing to the specified instruction
//      to      - linear address of referenced instruction
//      current - linear address of current referenced instruction
//                This value is returned by get_first_cref_to() or
//                previous call to get_next_cref_to() functions.
// returns: linear address of the next referencing instruction or BADADDR.
//          The 'lastXR' variable contains type of the reference

idaman ea_t ida_export get_next_cref_to(ea_t from,ea_t current);


// The following functions are similar to get_{first|next}_cref_{from|to}
// functions. The only difference is that they don't take into account
// ordinary flow of execution. Only jump and call xrefs are returned.
// (fcref means "far code reference")

idaman ea_t ida_export get_first_fcref_from(ea_t from);
idaman ea_t ida_export get_next_fcref_from (ea_t from,ea_t current);
idaman ea_t ida_export get_first_fcref_to  (ea_t to);
idaman ea_t ida_export get_next_fcref_to   (ea_t to,ea_t current);


// Has a location external to the function references?

idaman bool ida_export has_external_refs(func_t *pfn, ea_t ea);


//-------------------------------------------------------------------------
//      F U N C T I O N S   F O R   T H E   K E R N E L
//-------------------------------------------------------------------------

idaman int ida_export create_xrefs_from(ea_t ea);  // returns 0: no item at ea
void create_xrefs_from_data(ea_t ea);
idaman void ida_export delete_all_xrefs_from(ea_t ea,int expand);
void delete_data_xrefs_from(ea_t ea);
void delete_code_xrefs_from(ea_t ea,int expand);

int destroy_if_align(ea_t ea);   // 1-alignment is destroyed

#pragma pack(pop)
#endif // _XREF_HPP
