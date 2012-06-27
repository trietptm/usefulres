/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com
 *                              FIDO:   2:5020/209
 *
 *      PEF Loader
 *      ----------
 *      This module allows IDA to load PEF files into
 *      its database and to disassemble them correctly.
 *
 */

#include "../idaldr.h"
#include "pef.hpp"
#include "../coff/syms.h"
#include "common.cpp"

static ea_t toc_ea;
static netnode toc;
//----------------------------------------------------------------------
static char *get_sec_share_name(uint8 share, char *buf, size_t bufsize)
{
  switch ( share )
  {
    case PEF_SH_PROCESS: return "Shared within process";
    case PEF_SH_GLOBAL : return "Shared between all processes";
    case PEF_SH_PROTECT: return "Shared between all processes but protected";
    default:
      qsnprintf(buf, bufsize, "Unknown code %d", share);
      return buf;
  }
}

//----------------------------------------------------------------------
static void process_vector(ulong ea, const char *name)
{
  set_offset(ea,0,0);
  set_offset(ea+4,0,0);
  ulong mintoc = get_long(ea+4);
  if ( segtype(mintoc) == SEG_DATA && mintoc < toc_ea )
  {
    toc_ea = mintoc;
    ph.notify(processor_t::idp_notify(ph.loader+1), toc_ea);
  }
  set_name(ea, name);
  char buf[MAXSTR];
  qsnprintf(buf, sizeof(buf), ".%s", name);
  ulong code = get_long(ea);
  add_entry(code, code, buf, 1);
  make_name_auto(code);
}

//----------------------------------------------------------------------
static void process_symbol_class(ulong ea, uchar sclass, const char *name)
{
  switch ( sclass )
  {
    case kPEFCodeSymbol :
    case kPEFGlueSymbol :
      add_entry(ea, ea, name, 1);
      break;
    case kPEFTVectSymbol:
      process_vector(ea, name);
      break;
    case kPEFTOCSymbol  :
      if ( segtype(ea) == SEG_DATA && ea < toc_ea )
      {
        toc_ea = ea;
        ph.notify(processor_t::idp_notify(ph.loader+1), toc_ea);
      }
      toc.charset(ea, XMC_TD+1, 1);
      /* fall thru */
    case kPEFDataSymbol :
      set_name(ea, name);
      break;
  }
}

//----------------------------------------------------------------------
static void fixup(ulong ea, ulong delta, int extdef)
{
  fixup_data_t fd;
  fd.type = FIXUP_OFF32;
  if ( extdef ) fd.type |= FIXUP_EXTDEF;
  segment_t *s = getseg(delta);
  fd.displacement = get_long(ea);
  if ( s == NULL ) {
    fd.sel = 0;
    fd.off = delta;
  } else {
    fd.sel = s->sel;
    fd.off = delta - get_segm_base(s);
  }
  set_fixup(ea, &fd);
  ulong target = get_long(ea) + delta;
  put_long(ea, target);
  set_offset(ea, 0, 0);
  cmd.ea = ea; ua_add_dref(0, target, dr_O); cmd.ea = BADADDR;
  if ( target != toc_ea
    && !has_name(get_flags_novalue(ea))
    && has_name(get_flags_novalue(target)) )
  {
    char buf[MAXSTR];
    if ( get_true_name(BADADDR, target, &buf[3], sizeof(buf)-3) != NULL )
    {
      buf[0] = 'T';
      buf[1] = 'C';
      buf[2] = '_';
      do_name_anyway(ea, buf);
      make_name_auto(ea);
    }
  }
//  toc.charset(ea,XMC_TC+1,1);
}

//----------------------------------------------------------------------
static void process_loader(char *ptr, pef_section_t *sec, int nsec) {
  int i;
  pef_loader_t &pl = *(pef_loader_t *)ptr;
  pef_library_t *pil = (pef_library_t *)(&pl + 1);
  swap_pef_loader(pl);
  uint32 *impsym = (uint32 *)(pil + pl.importLibraryCount);
  pef_reloc_header_t *prh =
                (pef_reloc_header_t *)(impsym + pl.totalImportedSymbolCount);
  uint16 *relptr = (uint16 *)(ptr + pl.relocInstrOffset);
  uint32 *hash = (uint32 *)(ptr + pl.exportHashOffset);
  ulong hashsize = (1 << pl.exportHashTablePower);
  uint32 *keytable = hash + hashsize;
  pef_export_t *pe = (pef_export_t *)(keytable + pl.exportedSymbolCount);
#if !__MF__
  for ( i=0; i < pl.importLibraryCount; i++ )
    swap_pef_library(pil[i]);
  for ( i=0; i < pl.relocSectionCount; i++ )
    swap_pef_reloc_header(prh[i]);
  for ( i=0; i < pl.exportedSymbolCount; i++ )
    swap_pef_export(pe[i]);
#endif
  char *stable = ptr + pl.loaderStringsOffset;

  if ( pl.totalImportedSymbolCount != 0 ) {
    ulong size = pl.totalImportedSymbolCount*4;
    ulong undef = freechunk(inf.maxEA, size, -0xF);
    ulong end = undef + size;
    set_selector(nsec+1, 0);
    if(!add_segm(nsec+1, undef, end, "IMPORT", "XTRN")) loader_failure();

    for ( i=0; i < pl.importLibraryCount; i++ ) {
      ulong ea = undef + 4*pil[i].firstImportedSymbol;
      add_long_cmt(ea, 1, "Imports from library %s", stable+pil[i].nameOffset);
      if ( pil[i].options & PEF_LIB_WEAK )
        add_long_cmt(ea, 1, "Library is weak");
    }

    inf.specsegs = 1;
    for ( i=0; i < pl.totalImportedSymbolCount; i++ ) {
      uint32 sym = mflong(impsym[i]);
      uchar sclass = uchar(sym >> 24);
      ulong ea = undef+4*i;
      set_name(ea, get_impsym_name(stable,impsym,i));
      if ( (sclass & kPEFWeak) != 0 ) make_name_weak(ea);
      doDwrd(ea,4);
      put_long(ea, 0);
      impsym[i] = ea;
    }
  }

  if ( pl.mainSection != -1 ) {
    ulong ea = sec[pl.mainSection].defaultAddress + pl.mainOffset;
    toc_ea = sec[1].defaultAddress + get_long(ea+4);
    ph.notify(processor_t::idp_notify(ph.loader+1), toc_ea);
  } else if ( pl.initSection != -1 ) {
    ulong ea = sec[pl.initSection].defaultAddress + pl.initOffset;
    toc_ea = sec[1].defaultAddress + get_long(ea+4);
    ph.notify(processor_t::idp_notify(ph.loader+1), toc_ea);
  }

  if ( getenv("IDA_NORELOC") != NULL ) goto EXPORTS;
  msg("Processing relocation information... ");
  for ( i=0; i < pl.relocSectionCount; i++ )
  {
    ulong sea = sec[prh[i].sectionIndex].defaultAddress;
    uint16 *ptr = relptr + prh[i].firstRelocOffset;
    ulong reladdr = sea;
    ulong import  = 0;
    ulong code    = nsec > 0 ? sec[0].defaultAddress : 0;
    ulong data    = nsec > 1 ? sec[1].defaultAddress : 0;
    long repeat   = -1;
    for ( int j=0; j < prh[i].relocCount; ) {
      uint16 insn = mfshort(ptr[j++]);
      uint16 cnt = insn & 0x1FF;
      switch ( insn >> 9 ) {
        default:  // kPEFRelocBySectDWithSkip= 0x00,/* binary: 00xxxxx */
          if ( (insn & 0xC000) == 0 ) {
            int skipCount = (insn >> 6) & 0xFF;
            int relocCount = insn & 0x3F;
            reladdr += skipCount * 4;
            while ( relocCount > 0 ) {
              relocCount--;
              fixup(reladdr, data, 0);
              reladdr += 4;
            }
            break;
          }
          loader_failure("Bad relocation info");

        case kPEFRelocBySectC:  //     = 0x20,  /* binary: 0100000 */
          cnt++;
          while ( cnt > 0 ) {
            cnt--;
            fixup(reladdr, code, 0);
            reladdr += 4;
          }
          break;
        case kPEFRelocBySectD:
          cnt++;
          while ( cnt > 0 ) {
            cnt--;
            fixup(reladdr, data, 0);
            reladdr += 4;
          }
          break;
        case kPEFRelocTVector12:
          cnt++;
          while ( cnt > 0 ) {
            cnt--;
            fixup(reladdr, code, 0);
            reladdr += 4;
            fixup(reladdr, data, 0);
            reladdr += 4;
            reladdr += 4;
          }
          break;
        case kPEFRelocTVector8:
          cnt++;
          while ( cnt > 0 ) {
            cnt--;
            fixup(reladdr, code, 0);
            reladdr += 4;
            fixup(reladdr, data, 0);
            reladdr += 4;
          }
          break;
        case kPEFRelocVTable8:
          cnt++;
          while ( cnt > 0 ) {
            cnt--;
            fixup(reladdr, data, 0);
            reladdr += 4;
            reladdr += 4;
          }
          break;
        case kPEFRelocImportRun:
          cnt++;
          while ( cnt > 0 ) {
            cnt--;
            fixup(reladdr, impsym[import], 1);
            import++;
            reladdr += 4;
          }
          break;

        case kPEFRelocSmByImport:
          fixup(reladdr, impsym[cnt], 1);
          reladdr += 4;
          import = cnt + 1;
          break;
        case kPEFRelocSmSetSectC:
          code = sec[cnt].defaultAddress;
          break;
        case kPEFRelocSmSetSectD:
          data = sec[cnt].defaultAddress;
          break;
        case kPEFRelocSmBySection:
          fixup(reladdr, sec[cnt].defaultAddress, 0);
          reladdr += 4;
          break;

        case kPEFRelocIncrPosition: /* binary: 1000xxx */
        case kPEFRelocIncrPosition+1:
        case kPEFRelocIncrPosition+2:
        case kPEFRelocIncrPosition+3:
        case kPEFRelocIncrPosition+4:
        case kPEFRelocIncrPosition+5:
        case kPEFRelocIncrPosition+6:
        case kPEFRelocIncrPosition+7:
          reladdr += (insn & 0x0FFF)+1;
          break;

        case kPEFRelocSmRepeat:   /* binary: 1001xxx */
        case kPEFRelocSmRepeat+1:
        case kPEFRelocSmRepeat+2:
        case kPEFRelocSmRepeat+3:
        case kPEFRelocSmRepeat+4:
        case kPEFRelocSmRepeat+5:
        case kPEFRelocSmRepeat+6:
        case kPEFRelocSmRepeat+7:
          if ( repeat == -1 ) repeat = (insn & 0xFF)+1;
          repeat--;
          if ( repeat != -1 ) j -= ((insn>>8) & 15)+1 + 1;
          break;

        case kPEFRelocSetPosition:  /* binary: 101000x */
        case kPEFRelocSetPosition+1:
          {
            ushort next = mfshort(ptr[j++]);
            ulong offset = next | (ulong(insn & 0x3FF) << 16);
            reladdr = sea + offset;
          }
          break;

        case kPEFRelocLgByImport: /* binary: 101001x */
        case kPEFRelocLgByImport+1:
          {
            ushort next = mfshort(ptr[j++]);
            ulong index = next | (ulong(insn & 0x3FF) << 16);
            fixup(reladdr, impsym[index], 1);
            reladdr += 4;
            import = index + 1;
          }
          break;

        case kPEFRelocLgRepeat:   /* binary: 101100x */
        case kPEFRelocLgRepeat+1:
          {
            ushort next = mfshort(ptr[j++]);
            if ( repeat == -1 ) repeat = next | (ulong(insn & 0x3F) << 16);
            repeat--;
            if ( repeat != -1 ) j -= ((insn >> 6) & 15) + 1 + 2;
          }
          break;

        case kPEFRelocLgSetOrBySection: /* binary: 101101x */
        case kPEFRelocLgSetOrBySection+1:
          {
            ushort next = mfshort(ptr[j++]);
            ulong index = next | (ulong(insn & 0x3F) << 16);
            int subcode = (insn >> 6) & 15;
            switch ( subcode ) {
              case 0:
                fixup(reladdr, sec[index].defaultAddress, 0);
          reladdr += 4;
          break;
              case 1:
                code = sec[index].defaultAddress;
          break;
              case 2:
                data = sec[index].defaultAddress;
          break;
            }
          }
          break;
      }
    }
  }

EXPORTS:
  for ( i=0; i < pl.exportedSymbolCount; i++ ) {
    uchar sclass = uchar(pe[i].classAndName >> 24);
    char name[MAXSTR];
    ulong ea;
    switch ( pe[i].sectionIndex )
    {
      case -3:
        ea = impsym[pe[i].symbolValue];
        break;
      case -2:  // absolute symbol
        ask_for_feedback("Absolute symbols are not implemented");
        continue;
      default:
        ea = sec[pe[i].sectionIndex].defaultAddress + pe[i].symbolValue;
        break;
    }
    process_symbol_class(ea, sclass & 0xF,
                     get_expsym_name(stable, keytable, pe, i, name, sizeof(name)));
  }
  msg("done.\n");

  if ( pl.mainSection != -1 )
  {
    ulong ea = sec[pl.mainSection].defaultAddress + pl.mainOffset;
    process_vector(ea,"start");
    inf.start_cs = 0;
    inf.startIP  = get_long(ea);
  }
  if ( pl.initSection != -1 )
  {
    ulong ea = sec[pl.initSection].defaultAddress + pl.initOffset;
    process_vector(ea,"INIT_VECTOR");
  }
  if ( pl.termSection != -1 )
  {
    ulong ea = sec[pl.termSection].defaultAddress + pl.termOffset;
    process_vector(ea,"TERM_VECTOR");
  }

  if ( toc_ea != BADADDR )
    set_name(toc_ea, "TOC");
}

//--------------------------------------------------------------------------
static void bad_packed_data(void)
{
  loader_failure("Illegal compressed data");
}

//--------------------------------------------------------------------------
static ulong read_number(const char *&packed)
{
  ulong arg = 0;
  for ( int i=0; ; i++ ) {
    uchar b = *packed++;
    arg <<= 7;
    arg |= (b & 0x7F);
    if ( (b & 0x80) == 0 ) break;
    if ( i > 4 ) bad_packed_data();
  }
  return arg;
}

//--------------------------------------------------------------------------
static void unpack_section(const char *packed,
                                ulong psize,
                                ulong start,
                                ulong usize)
{
  char *unpacked = qnewarray(char, usize);
  if ( unpacked == NULL ) nomem("section usize %lu\n", usize);
  ulong saved_usize = usize;
  char *ptr = unpacked;
  const char *end = packed + psize;
  while ( packed < end ) {
    uchar code = *packed++;
    ulong arg = code & 0x1F;
    if ( arg == 0 ) arg = read_number(packed);
    switch ( code >> 5 ) {
      case 0:           // Zero
//        msg("%02X (Zero) Arg=%lX usize=%lX\n",code, arg, usize);
        if ( arg > usize ) bad_packed_data();
        memset(ptr,0,arg);
        ptr   += arg;
        usize -= arg;
        break;

      case 1:           // blockCopy
//        msg("%02X (Copy) Arg=%lX usize=%lX\n",code, arg, usize);
        if ( arg > usize ) bad_packed_data();
        memcpy(ptr,packed,arg);
        packed += arg;
        ptr    += arg;
        usize  -= arg;
        break;

      case 2:           // repeatedBlock
        {
          ulong repeat = read_number(packed) + 1;
//          msg("%02X (repeatBlock) Arg=%lX Repeat=%lX usize=%lX\n",code, arg, repeat, usize);
          while ( repeat > 0 ) {
            repeat--;
            if ( arg > usize ) bad_packed_data();
            memcpy(ptr,packed,arg);
            ptr   += arg;
            usize -= arg;
          }
          packed += arg;
        }
        break;

      case 3:           // interleaveRepeatBlockWithBlockCopy
        {
          ulong commonSize  = arg;
          ulong customSize  = read_number(packed);
          ulong repeatCount = read_number(packed);
//          msg("%02X (interBlock) Common=%lX Custom=%lX Repeat=%lX usize=%lX\n", code, commonSize, customSize, repeatCount, usize);
          const char *common = packed;
          packed += commonSize;
          while ( repeatCount > 0 ) {
            repeatCount--;
            if ( commonSize > usize ) bad_packed_data();
            memcpy(ptr,common,commonSize);
            ptr   += commonSize;
            usize -= commonSize;
            if ( customSize > usize ) bad_packed_data();
            memcpy(ptr,packed,customSize);
            ptr   += customSize;
            packed+= customSize;
            usize -= customSize;
          }
          if ( commonSize > usize ) bad_packed_data();
          memcpy(ptr,common,commonSize);
          ptr   += commonSize;
          usize -= commonSize;
        }
        break;

      case 4:           // interleaveRepeatBlockWithZero
        {
          ulong commonSize  = arg;
          ulong customSize  = read_number(packed);
          ulong repeatCount = read_number(packed);
//          msg("%02X (interZero) Common=%lX Custom=%lX Repeat=%lX usize=%lX\n", code, commonSize, customSize, repeatCount, usize);
          while ( repeatCount > 0 ) {
            repeatCount--;
            if ( commonSize > usize ) bad_packed_data();
            memset(ptr,0,commonSize);
            ptr   += commonSize;
            usize -= commonSize;
            if ( customSize > usize ) bad_packed_data();
            memcpy(ptr,packed,customSize);
            ptr   += customSize;
            packed+= customSize;
            usize -= customSize;
          }
          if ( commonSize > usize ) bad_packed_data();
          memset(ptr,0,commonSize);
          ptr   += commonSize;
          usize -= commonSize;
        }
        break;

      default:
//        msg("%02X - BAD OPCODE!\n",code);
        bad_packed_data();

    }
  }
  if ( usize != 0 ) memset(ptr,0,usize);
  mem2base(unpacked, start, start+saved_usize, FILEREG_NOTPATCHABLE);
  qfree(unpacked);
}

//--------------------------------------------------------------------------
static void load_section(int i,
                              linput_t *li,
                              pef_section_t &ps,
                              const char *sname,
                              const char *classname,
                              int is_packed) {
  ulong size  = ps.totalSize;
  ulong base  = ps.defaultAddress ? ps.defaultAddress : toEA(inf.baseaddr,0);
  ulong start = freechunk(base, size, 1-(1 << ps.alignment));
  ulong end   = start + size;
  if ( is_packed ) {
    char *packed = qnewarray(char, ps.packedSize);
    if ( packed == NULL ) nomem("section psize %lu\n", ps.packedSize);
    qlseek(li, ps.containerOffset);
    lread(li, packed, ps.packedSize);
    unpack_section(packed, ps.packedSize, start, ps.unpackedSize);
    qfree(packed);
  } else {
    file2base(li, ps.containerOffset,
                        start, start+ps.unpackedSize, FILEREG_PATCHABLE);
  }
  set_selector(i+1, 0);
  if(!add_segm(i+1, start, end, sname, classname)) loader_failure();
  ps.defaultAddress = start;
}

//--------------------------------------------------------------------------
//
//      load file into the database.
//
void load_file(linput_t *li, ushort /*neflag*/, const char * /*fileformatname*/) {
  int i;
  pef_t pef;
  toc_ea = BADADDR;
  toc.create("$ toc");
  qlseek(li,0);
  lread(li, &pef, sizeof(pef_t));
  swap_pef(pef);
  if ( strncmp(pef.architecture,PEF_ARCH_PPC,4) == 0 && ph.id != PLFM_PPC ) set_processor_type("ppc",   SETPROC_ALL|SETPROC_FATAL);
  if ( strncmp(pef.architecture,PEF_ARCH_68K,4) == 0 && ph.id != PLFM_68K ) set_processor_type("68000", SETPROC_ALL|SETPROC_FATAL);
  pef_section_t *sec = qnewarray(pef_section_t, pef.sectionCount);
  if ( sec == NULL ) nomem("sections table\n");
  lread(li, sec, sizeof(pef_section_t)*pef.sectionCount);
  long snames_table = sizeof(pef_t) + sizeof(pef_section_t)*pef.sectionCount;
  pef_section_t *loader = NULL;
  for ( i=0; i < pef.sectionCount; i++ ) {
    swap_pef_section(sec[i]);
    if ( sec[i].sectionKind == PEF_SEC_LOADER ) loader = &sec[i];
  }
  for ( i=0; i < pef.sectionCount; i++ )
  {
    char buf[MAXSTR];
    char *secname = get_string(li, snames_table, sec[i].nameOffset, buf, sizeof(buf));
    switch ( sec[i].sectionKind ) {
      case PEF_SEC_PDATA :      //   Pattern initialized data segment
        load_section(i, li, sec[i], secname, CLASS_DATA, 1);
        break;
      case PEF_SEC_CODE  :      //   Code segment
      case PEF_SEC_EDATA :      //   Executable data segment
        load_section(i, li, sec[i], secname, CLASS_CODE, 0);
        break;
      case PEF_SEC_DATA  :      //   Unpacked data segment
        load_section(i, li, sec[i], secname,
            sec[i].unpackedSize != 0 ? CLASS_DATA : CLASS_BSS, 0);
        break;
      case PEF_SEC_CONST :      //   Read only data
        load_section(i, li, sec[i], secname, CLASS_CONST, 0);
        break;
      case PEF_SEC_LOADER:      //   Loader section
      case PEF_SEC_DEBUG :      //   Reserved for future use
      case PEF_SEC_EXCEPT:      //   Reserved for future use
      case PEF_SEC_TRACEB:      //   Reserved for future use
        continue;
      default:
        ask_for_feedback("Unknown section type");
        continue;
    }
    if ( i == 0 ) create_filename_cmt();
    add_long_cmt(sec[i].defaultAddress, 1, "Segment share type: %s\n",
                 get_sec_share_name(sec[i].shareKind, buf, sizeof(buf)));
  }
  if ( loader != NULL )
  {
    char *ptr = qnewarray(char, loader->packedSize);
    if ( ptr == NULL ) nomem("loader size %lu\n", loader->packedSize);
    qlseek(li, loader->containerOffset);
    lread(li, ptr, loader->packedSize);
    process_loader(ptr, sec, pef.sectionCount);
    qfree(ptr);
  }
  qfree(sec);
}


//--------------------------------------------------------------------------
//
//      check input file format. if recognized, then return 1
//      and fill 'fileformatname'.
//      otherwise return 0
//
int accept_file(linput_t *li, char fileformatname[MAX_FILE_FORMAT_NAME], int n)
{
  if ( n == 0 && is_pef_file(li) )
  {
    qstrncpy(fileformatname,"PEF (Mac OS or Be OS executable)",MAX_FILE_FORMAT_NAME);
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------
bool idaapi init_loader_options(linput_t *li)
{
  pef_t pef;
  qlseek(li,0);
  lread(li,&pef,sizeof(pef_t));
  swap_pef(pef);
  if (strncmp(pef.architecture,PEF_ARCH_PPC,4) == 0 && ph.id != PLFM_PPC)
    set_processor_type("ppc",   SETPROC_ALL|SETPROC_FATAL);
  if (strncmp(pef.architecture,PEF_ARCH_68K,4) == 0 && ph.id != PLFM_68K)
    set_processor_type("68000", SETPROC_ALL|SETPROC_FATAL);
  return true;
}

//----------------------------------------------------------------------
//
//      LOADER DESCRIPTION BLOCK
//
//----------------------------------------------------------------------
loader_t LDSC =
{
  IDP_INTERFACE_VERSION,
  0,                            // loader flags
//
//      check input file format. if recognized, then return 1
//      and fill 'fileformatname'.
//      otherwise return 0
//
  accept_file,
//
//      load file into the database.
//
  load_file,
//
//	create output file from the database.
//	this function may be absent.
//
  NULL,
//      take care of a moved segment (fix up relocations, for example)
  NULL,
//      initialize user configurable options based on the input file.
  init_loader_options,
};
