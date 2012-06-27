/*
 *      Interactive disassembler (IDA).
 *      Version 3.05
 *      Copyright (c) 1990-95 by Ilfak Guilfanov. (2:5020/209@fidonet)
 *      ALL RIGHTS RESERVED.
 *
 */

//
//      Portable Executable file format (MS Windows 95, MS Windows NT)
//

#ifndef _PE_H_
#define _PE_H_

#include <time.h>
#include <stddef.h>

#pragma pack(push, 1)           // IDA uses 1 byte alignments!
//-----------------------------------------------------------------------
//
// 32-bit Portable EXE Header
//
//-----------------------------------------------------------------------
struct petab_t
{
  ulong rva;            // relative virtual address
  ulong size;           // size
};                      // PE va/size array element

template <class pointer_t>
struct peheader_tpl
{
  long signature;       // 00 Current value is "PE/0/0".
                  // Thats PE followed by two zeros (nulls).
#define PEEXE_ID   0x4550
#define BPEEXE_ID  0x455042 // Borland's extenson for DPMI'host
  ushort machine;       // 04 This field specifies the type of CPU
                  // compatibility required by this image to run.
                  // The values are:
#define PECPU_UNKNOWN 0x0000    // unknown
#define PECPU_80386   0x014C    // 80386
#define PECPU_80486   0x014D    // 80486
#define PECPU_80586   0x014E    // 80586
#define PECPU_R3000   0x0162    // MIPS Mark I (R2000, R3000)
#define PECPU_R6000   0x0163    // MIPS Mark II (R6000)
#define PECPU_R4000   0x0166    // MIPS Mark III (R4000)
#define PECPU_R10000  0x0168    // MIPS Mark IV (R10000)
#define PECPU_MIPSFPU 0x0366    // MIPS with FPU
#define PECPU_MIPSFPU16 0x466   // MIPS16 with FPU
#define PECPU_ALPHA   0x0184    // DEC Alpha
#define PECPU_ALPHA64 0x0284    // Dec Alpha 64-bit
#define PECPU_SH3     0x01A2    // SH3
#define PECPU_SH4     0x01A6    // SH4
#define PECPU_ARM     0x01C0    // ARM
#define PECPU_ARMI    0x01C2    // ARM (with internetworking)
#define PECPU_PPC     0x01F0    // PowerPC
#define PECPU_IA64    0x0200    // Intel Itanium IA64
#define PECPU_MIPS16  0x0266    // MIPS16
#define PECPU_EPOC    0x0A00    // ARM EPOC
#define PECPU_AMD64   0x8664    // AMD64 (x64)
#define PECPU_M68K    0x0268    // Motorola 68000 series

  ushort nobjs;     // 06 This field specifies the number of entries
                    // in the Object Table.
  time_t datetime;  // 08 Used to store the time and date the file was
                    // created or modified by the linker.
  ulong symtof;     // 0C Symbol Table Offset
  ulong nsyms;      // 10 Number of Symbols in Symbol Table
  ushort hdrsize;   // 14 This is the number of remaining bytes in the NT
                    // header that follow the FLAGS field.
  ushort flags;     // 16 Flag bits for the image.  0000h Program image.
#define PEF_BRVHI 0x8000        // Big endian: MSB precedes LSB in memory
#define PEF_UP    0x4000        // File should be run only on a UP machine
#define PEF_DLL   0x2000        // Dynamic Link Library (DLL)
#define PEF_SYS   0x1000        // System file
#define PEF_0800  0x0800        // Reserved
#define PEF_SWAP  0x0400        // If image is on removable media,
                                // copy and run from swap file
#define PEF_NODEB 0x0200        // Debugging info stripped
#define PEF_32BIT 0x0100        // 32-bit word machine
#define PEF_BRVLO 0x0080        // Little endian: LSB precedes MSB in memory
#define PEF_16BIT 0x0040        // 16-bit word machine
#define PEF_2GB   0x0020        // App can handle > 2gb addresses
#define PEF_TMWKS 0x0010        // Aggressively trim working set
#define PEF_NOSYM 0x0008        // Local symbols stripped
#define PEF_NOLIN 0x0004        // Line numbers stripped
#define PEF_EXEC  0x0002        // Image is executable
#define PEF_NOFIX 0x0001        // Relocation info stripped

  long first_section_pos(long peoff) const
    { return peoff + offsetof(peheader_tpl, magic) + hdrsize; }

        // COFF fields:

  ushort magic;       // 18 Magic
#define MAGIC_ROM       0x107   // ROM image
#define MAGIC_P32       0x10B   // Normal PE file
#define MAGIC_P32_PLUS  0x20B   // 64-bit image
  bool is_pe_plus(void) const { return magic == MAGIC_P32_PLUS; }
  uchar vstamp_major; // 1A Major Linker Version
  uchar vstamp_minor; // 1B Minor Linker Version
  ulong tsize;        // 1C TEXT size (padded)
  ulong dsize;        // 20 DATA size (padded)
  ulong bsize;        // 24 BSS  size (padded)
  ulong entry;        // 28 Entry point
  ulong text_start;   // 2C Base of text
  union
  {
    struct
    {
      ulong data_start;   // 30 Base of data

        // Win32/NT extensions:

      ulong imagebase32;    // 34 Virtual base of the image.
    };
    ulong imagebase64[2];
  };
  uint64 imagebase() const
  {
    if ( is_pe_plus() )
      return make_ulonglong(imagebase64[0], imagebase64[1]);
    else
      return imagebase32;
  }
                      // This will be the virtual address of the first
                      // byte of the file (Dos Header).  This must be
                      // a multiple of 64K.
  ulong objalign;     // 38 The alignment of the objects. This must be a power
                      // of 2 between 512 and 256M inclusive. The default
                      // is 64K.
  ulong filealign;    // 3C Alignment factor used to align image pages.
                      // The alignment factor (in bytes) used to align the
                      // base of the image pages and to determine the
                      // granularity of per-object trailing zero pad.
                      // Larger alignment factors will cost more file space;
                      // smaller alignment factors will impact demand load
                      // performance, perhaps significantly. Of the two,
                      // wasting file space is preferable.  This value
                      // should be a power of 2 between 512 and 64K inclusive.
                      // Get the file position aligned:
  ulong align_up_in_file(ulong pos) const { return (pos+filealign-1) & ~(filealign-1); }
  ulong align_down_in_file(ulong pos) const { return pos & ~(filealign-1); }
  ushort osmajor;     // 40 OS version number required to run this image.
  ushort osminor;     // 42 OS version number required to run this image.
  ushort imagemajor;  // 44 User major version number.
  ushort imageminor;  // 46 User minor version number.
  ushort subsysmajor; // 48 Subsystem major version number.
  ushort subsysminor; // 4A Subsystem minor version number.
  ulong reserved;     // 4C
  ulong imagesize;    // 50 The virtual size (in bytes) of the image.
                      // This includes all headers.  The total image size
                      // must be a multiple of Object Align.
  ulong allhdrsize;   // 54 Total header size. The combined size of the Dos
                      // Header, PE Header and Object Table.
  ulong checksum;     // 58 Checksum for entire file.  Set to 0 by the linker.
  ushort subsys;      // 5C NT Subsystem required to run this image.
#define PES_UNKNOWN 0x0000      // Unknown
#define PES_NATIVE  0x0001      // Native
#define PES_WINGUI  0x0002      // Windows GUI
#define PES_WINCHAR 0x0003      // Windows Character
#define PES_OS2CHAR 0x0005      // OS/2 Character
#define PES_POSIX   0x0007      // Posix Character
#define PES_WINCE   0x0009      // Runs on Windows CE.
#define PES_EFI_APP 0x000A      // EFI application.
#define PES_EFI_BDV 0x000B      // EFI driver that provides boot services.
#define PES_EFI_RDV 0x000C      // EFI driver that provides runtime services.
  bool is_console_app(void) const
  {
    return subsys == PES_WINCHAR
        || subsys == PES_OS2CHAR
        || subsys == PES_POSIX;
  }

  ushort dllflags;    // 5E Indicates special loader requirements.
#define PEL_PINIT   0x0001      // Per-Process Library Initialization.
#define PEL_PTERM   0x0002      // Per-Process Library Termination.
#define PEL_TINIT   0x0004      // Per-Thread Library Initialization.
#define PEL_TTERM   0x0008      // Per-Thread Library Termination.
#define PEL_NO_BIND 0x0800      // Do not bind image
#define PEL_WDM_DRV 0x2000      // Driver is a WDM Driver
#define PEL_TSRVAWA 0x8000      // Image is Terminal Server aware

  pointer_t stackres; // 60 Stack size needed for image. The memory is
                      // reserved, but only the STACK COMMIT SIZE is
                      // committed. The next page of the stack is a
                      // 'guarded page'. When the application hits the
                      // guarded page, the guarded page becomes valid,
                      // and the next page becomes the guarded page.
                      // This continues until the RESERVE SIZE is reached.
  pointer_t stackcom; // 64 Stack commit size.
  pointer_t heapres;  // 68 Size of local heap to reserve.
  pointer_t heapcom;  // 6C Amount to commit in local heap.
  ulong loaderflags;  // 70 ?
  ulong nrvas;        // 74 Indicates the size of the VA/SIZE array
                      // that follows.
  petab_t expdir;     // 78 Export Directory
  petab_t impdir;     // 80 Import Directory
  petab_t resdir;     // 88 Resource Directory
  petab_t excdir;     // 90 Exception Directory
  petab_t secdir;     // 98 Security Directory
                      // The Certificate Table entry points to a table of
                      // attribute certificates. These certificates are not
                      // loaded into memory as part of the image. As such,
                      // the first field of this entry, which is normally
                      // an RVA, is a File Pointer instead
  petab_t reltab;     // A0 Relocation Table
  petab_t debdir;     // A8 Debug Directory
  petab_t desstr;     // B0 Description String
  petab_t cputab;     // B8 Machine Value
  petab_t tlsdir;     // C0 TLS Directory
  petab_t loddir;     // Load Configuration Directory
  petab_t bimtab;     // Bound Import Table address and size.
  petab_t iat;        // Import Address Table address and size.
  petab_t didtab;     // Address and size of the Delay Import Descriptor.
  petab_t comhdr;     // COM+ Runtime Header address and size
  petab_t x00tab;     // Reserved
};

typedef peheader_tpl<ulong>  peheader_t;
typedef peheader_tpl<uint64> peheader64_t;

const int total_rvatab_size = sizeof(peheader_t) - offsetof(peheader_t, expdir);
const int total_rvatab_count = total_rvatab_size / sizeof(petab_t);

//-----------------------------------------------------------------------
struct diheader_t
{
  ushort signature;     // Current value is "DI"
#define DBG_ID 0x4944
  ushort flags2;        // ?? pedump mentions about this
                        // I've never seen something other than 0
  ushort machine;       // This field specifies the type of CPU
                        // compatibility required by this image to run.
  ushort flags;         // Flag bits for the image.
  time_t datetime;      // Used to store the time and date the file was
                        // created or modified by the linker.
  ulong checksum;       // Checksum
  ulong imagebase;      // Virtual base of the image.
                        // This will be the virtual address of the first
                        // byte of the file (Dos Header).  This must be
                        // a multiple of 64K.
  ulong imagesize;      // The virtual size (in bytes) of the image.
                        // This includes all headers.  The total image size
                        // must be a multiple of Object Align.
  ulong n_secs;         // Number of sections
  ulong exp_name_size;  // Exported Names Size
  ulong dbg_dir_size;   // Debug Directory Size
  ulong reserved[3];    // Reserved fields
};

//-------------------------------------------------------------------------
//
//      S E C T I O N S
//
struct pesection_t
{
  char   s_name[8];   /* section name */
  ulong  s_vsize;     /* virtual size */
  ulong  s_vaddr;     /* virtual address */
  ulong  s_psize;     /* physical size */
  long   s_scnptr;    /* file ptr to raw data for section */
  long   s_relptr;    /* file ptr to relocation */
  long   s_lnnoptr;   /* file ptr to line numbers */
  ushort s_nreloc;    /* number of relocation entries */
  ushort s_nlnno;     /* number of line number entries */
  long   s_flags;     /* flags */
#define PEST_REG     0x00000000 // obsolete: regular: allocated, relocated, loaded
#define PEST_DUMMY   0x00000001 // obsolete: dummy: not allocated, relocated, not loaded
#define PEST_NOLOAD  0x00000002 // obsolete: noload: allocated, relocated, not loaded
#define PEST_GROUP   0x00000004 // obsolete: grouped: formed of input sections
#define PEST_PAD     0x00000008 // obsolete: padding: not allocated, not relocated, loaded
#define PEST_COPY    0x00000010 // obsolete: copy: for decision function used
                                //    by field update; not
                                //    allocated, not relocated,
                                //    loaded; reloc & lineno
                                //    entries processed normally */
#define PEST_TEXT    0x00000020L// section contains text only
#define PEST_DATA    0x00000040L// section contains data only
#define PEST_BSS     0x00000080L// section contains bss only
#define PEST_EXCEPT  0x00000100L// obsolete: Exception section
#define PEST_INFO    0x00000200L// Comment: not allocated, not relocated, not loaded
#define PEST_OVER    0x00000400L// obsolete: Overlay: not allocated, relocated, not loaded
#define PEST_LIB     0x00000800L// ".lib" section: treated like PEST_INFO

#define PEST_LOADER  0x00001000L// Loader section: COMDAT
#define PEST_DEBUG   0x00002000L// Debug section:
#define PEST_TYPCHK  0x00004000L// Type check section:
#define PEST_OVRFLO  0x00008000L// obsolete: RLD and line number overflow sec hdr
#define PEST_F0000   0x000F0000L// Unknown
#define PEST_ALIGN   0x00F00000L// Alignment 2^(x-1):
  ulong get_sect_alignment(void) const
  {
    int align = ((s_flags >> 20) & 15);
    if ( align == 0 ) return 0;
    return 1 << (align-1);
  }
#define PEST_1000000 0x01000000L// Unknown
#define PEST_DISCARD 0x02000000L// Discardable
#define PEST_NOCACHE 0x04000000L// Not cachable
#define PEST_NOPAGE  0x08000000L// Not pageable
#define PEST_SHARED  0x10000000L// Shareable
#define PEST_EXEC    0x20000000L// Executable
#define PEST_READ    0x40000000L// Readable
#define PEST_WRITE   0x80000000L// Writable
};

//-------------------------------------------------------------------------
//
//      E X P O R T S
//
struct peexpdir_t
{
  ulong flags;      // Currently set to zero.
  time_t datetime;      // Time/Date the export data was created.
  ushort major;     // A user settable major/minor version number.
  ushort minor;
  ulong dllname;    // Relative Virtual Address of the Dll asciiz Name.
                    // This is the address relative to the Image Base.
  ulong ordbase;    // First valid exported ordinal. This field specifies
                    // the starting ordinal number for the export
                    // address table for this image. Normally set to 1.
  ulong naddrs;     // Indicates number of entries in the Export Address
                    // Table.
  ulong nnames;     // This indicates the number of entries in the Name
                    // Ptr Table (and parallel Ordinal Table).
  ulong adrtab;     // Relative Virtual Address of the Export Address
                    // Table. This address is relative to the Image Base.
  ulong namtab;     // Relative Virtual Address of the Export Name Table
                    // Pointers. This address is relative to the
                    // beginning of the Image Base. This table is an
                    // array of RVA's with # NAMES entries.
  ulong ordtab;     // Relative Virtual Address of Export Ordinals
                    // Table Entry. This address is relative to the
                    // beginning of the Image Base.
};

//-------------------------------------------------------------------------
//
//      I M P O R T S
//
struct peimpdir_t
{
  ulong table1;
  time_t datetime;  // Time/Date the import data was pre-snapped or
                    // zero if not pre-snapped.
  ulong fchain;     // Forwarder chain
  ulong dllname;    // Relative Virtual Address of the Dll asciiz Name.
                    // This is the address relative to the Image Base.
  ulong looktab;    // This field contains the address of the start of
                    // the import lookup table for this image. The address
                    // is relative to the beginning of the Image Base.
#define hibit(type) ((type(-1)>>1) ^ type(-1))
#define IMP_BY_ORD32 hibit(ulong)   // Import by ordinal, otherwise by name
#define IMP_BY_ORD64 hibit(uint64)  // Import by ordinal, otherwise by name

  peimpdir_t(void) { memset(this, 0, sizeof(peimpdir_t)); }
};

struct dimpdir_t    // delayed load import table
{
  ulong attrs;      // Attributes.
#define DIMP_NOBASE 0x0001  // pe.imagebase was not added to addresses
  ulong dllname;    // Relative virtual address of the name of the DLL
                    // to be loaded. The name resides in the read-only
                    // data section of the image.
  ulong handle;     // Relative virtual address of the module handle
                    // (in the data section of the image) of the DLL to
                    // be delay-loaded. Used for storage by the routine
                    // supplied to manage delay-loading.
  ulong diat;       // Relative virtual address of the delay-load import
                    // address table. See below for further details.
  ulong dint;       // Relative virtual address of the delay-load name
                    // table, which contains the names of the imports
                    // that may need to be loaded. Matches the layout of
                    // the Import Name Table, Section 6.4.3. Hint/Name Table.
  ulong dbiat;      // Bound Delay Import Table. Relative virtual address
                    // of the bound delay-load address table, if it exists.
  ulong duiat;      // Unload Delay Import Table. Relative virtual address
                    // of the unload delay-load address table, if it exists.
                    // This is an exact copy of the Delay Import Address
                    // Table. In the event that the caller unloads the DLL,
                    // this table should be copied back over the Delay IAT
                    // such that subsequent calls to the DLL continue to
                    // use the thunking mechanism correctly.
  time_t datetime;  // Time stamp of DLL to which this image has been bound.
};


// Bound Import Table format:

struct BOUND_IMPORT_DESCRIPTOR
{
  time_t TimeDateStamp;
  ushort OffsetModuleName;
  ushort NumberOfModuleForwarderRefs;
// Array of zero or more IMAGE_BOUND_FORWARDER_REF follows
};

struct BOUND_FORWARDER_REF
{
  time_t TimeDateStamp;
  ushort OffsetModuleName;
  ushort Reserved;
};


//-------------------------------------------------------------------------
//
//      T H R E A D  L O C A L  D A T A
//

struct image_tls_directory64
{
  uint64 StartAddressOfRawData;
  uint64 EndAddressOfRawData;
  uint64 AddressOfIndex;         // PDWORD
  uint64 AddressOfCallBacks;     // PIMAGE_TLS_CALLBACK *;
  ulong  SizeOfZeroFill;
  ulong  Characteristics;
};

struct image_tls_directory32
{
  ulong  StartAddressOfRawData;
  ulong  EndAddressOfRawData;
  ulong  AddressOfIndex;         // PDWORD
  ulong  AddressOfCallBacks;     // PIMAGE_TLS_CALLBACK *
  ulong  SizeOfZeroFill;
  ulong  Characteristics;
};

//-------------------------------------------------------------------------
//
//      F I X U P S
//
struct pefixup_t
{
  ulong page;   // The image base plus the page rva is added to each offset
              // to create the virtual address of where the fixup needs to
              // be applied.
  ulong size;   // Number of bytes in the fixup block. This includes the
              // PAGE RVA and SIZE fields.
};

#define PER_OFF  0x0FFF
#define PER_TYPE 0xF000
#define PER_ABS         0x0000  // This is a NOP. The fixup is skipped.
#define PER_HIGH        0x1000  // Add the high 16-bits of the delta to the
                                // 16-bit field at Offset. The 16-bit field
                                // represents the high value of a 32-bit word.
#define PER_LOW         0x2000  // Add the low 16-bits of the delta to the
                                // 16-bit field at Offset. The 16-bit field
                                // represents the low half value of a
                                // 32-bit word.  This fixup will only be
                                // emitted for a RISC machine when the image
                                // Object Align isn't the default of 64K.
#define PER_HIGHLOW     0x3000  // Apply the 32-bit delta to the 32-bit field
                                // at Offset.
#define PER_HIGHADJUST  0x4000  // This fixup requires a full 32-bit value.
                                // The high 16-bits is located at Offset, and
                                // the low 16-bits is located in the next
                                // Offset array element (this array element
                                // is included in the SIZE field). The two
                                // need to be combined into a signed variable.
                                // Add the 32-bit delta. Then add 0x8000 and
                                // store the high 16-bits of the signed
                                // variable to the 16-bit field at Offset.
#define PER_MIPSJMPADDR  0x5000 // Fixup applies to a MIPS jump instruction.
#define PER_SECTION      0x6000 // Reserved for future use
#define PER_REL32        0x7000 // Reserved for future use
#define PER_MIPS_JMPADDR16 0x9000 // Fixup applies to a MIPS16 jump instruction.
#define PER_DIR64        0xA000 // This fixup applies the delta to the 64-bit
                                // field at Offset
#define PER_HIGH3ADJ     0xB000 // The fixup adds the high 16 bits of the delta
                                // to the 16-bit field at Offset. The 16-bit
                                // field represents the high value of a 48-bit
                                // word. The low 32 bits of the 48-bit value are
                                // stored in the 32-bit word that follows this
                                // base relocation. This means that this base
                                // relocation occupies three slots.

//-------------------------------------------------------------------------
//
//      DBG file debug entry format
//
struct debug_entry_t {
  ulong flags;                  // usually zero
  time_t datetime;
  ushort major;
  ushort minor;
  long type;
#define DBG_COFF          1
#define DBG_CV            2
#define DBG_FPO           3
#define DBG_MISC          4
#define DBG_OMAP_TO_SRC         7
#define DBG_OMAP_FROM_SRC       8
  ulong size;
  ulong rva;        // virtual address
  ulong seek;       // ptr to data in the file
};

//-------------------------------------------------------------------------
//
//      DBG file COFF debug information header
//
struct coff_debug_t {
  ulong NumberOfSymbols;
  ulong LvaToFirstSymbol;
  ulong NumberOfLinenumbers;
  ulong LvaToFirstLinenumber;
  ulong RvaToFirstByteOfCode;
  ulong RvaToLastByteOfCode;
  ulong RvaToFirstByteOfData;
  ulong RvaToLastByteOfData;
};

//-------------------------------------------------------------------------
//
//      DBG file FPO debug information
//
struct fpo_t {
  ulong address;
  ulong size;
  ulong locals;
  ushort params;
  uchar prolog;
  uchar regs;
#define FPO_REGS      0x07  // register number
#define FPO_SEH       0x08  //
#define FPO_BP        0x10  // has BP frame?
#define FPO_TYPE      0xC0
#define FPO_T_FPO     0x00
#define FPO_T_TRAP    0x40
#define FPO_T_TSS     0x80
#define FPO_T_NONFPO    0xC0
};


//      DBG file OMAP debug information

struct omap_t
{
  ulong a1;
  ulong a2;
};


//----------------------------------------------------------------------
// MS Windows CLSID, GUID
struct clsid_t
{
  ulong id1;
  ushort id2[2];
  uchar id3[8];
  bool operator == (const struct clsid_t &r) const
    { return memcmp(this, &r, sizeof(r)) == 0; }
};

//----------------------------------------------------------------------
// RSDS debug information
struct rsds_t
{
  ulong magic;
#define RSDS_MAGIC MC4('R','S','D','S')
  clsid_t guid;
  ulong age;
//  char name[];  // followed by a zero-terminated UTF8 file name
};

#define PE_NODE "$ PE header" // netnode name for PE header
                              // value()        -> peheader_t
                              // altval(segnum) -> s->startEA
#define PE_ALT_DBG_FPOS   -1  // altval() -> translated fpos of debuginfo
#define PE_ALT_IMAGEBASE  -2  // altval() -> loading address (usually pe.imagebase)
#define PE_ALT_PEHDR_OFF  -3  // altval() -> offset of PE header
#define PE_ALT_NEFLAGS    -4  // altval() -> neflags
#define PE_ALT_TDS_LOADED -5  // altval() -> tds already loaded(1) or invalid(-1)
                              // supval(segnum) -> pesection_t
                              // blob(0, PE_NODE_RELOC)  -> relocation info
#define PE_NODE_RELOC 'r'

#define TDS_AT_PE_LOAD  0x100AD  // 100==L => LOAD. Magic constant to pass to
                                // the TDS plugin

const char *get_pe_machine_name(ushort machine);
void print_pe_flags(ushort flags);

#pragma pack(pop)
#endif
