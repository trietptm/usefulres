/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-97 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 *      ARM Object File Loader
 *      ----------------------
 *      This module allows IDA to load ARM object files into
 *      its database and to disassemble them correctly.
 *
 *      NOTE 1: for the moment only B/BL instruction relocations are
 *      supported. I can not find other relocation examples so
 *      they are not implemented yet.
 *
 *      NOTE 2: Thumb modules are not supported.
 *
 *      This module automatically detects the byte sex and sets inf.mf
 *      variable accrodingly.
 *
 *
 */

#include "../idaldr.h"
#include "aof.h"

//--------------------------------------------------------------------------
static void nomem(void)
{
  nomem("AOF loader");
}

//--------------------------------------------------------------------------
static void *read_chunk(linput_t *li,chunk_entry_t *ce,int idx)
{
  void *chunk = qnewarray(char,size_t(ce[idx].size));
  if ( chunk == NULL ) nomem();
  qlseek(li,ce[idx].file_offset);
  lread(li,chunk,size_t(ce[idx].size));
  return chunk;
}

//--------------------------------------------------------------------------
//
//      check input file format. if recognized, then return 1
//      and fill 'fileformatname'.
//      otherwise return 0
//
int accept_file(linput_t *li, char fileformatname[MAX_FILE_FORMAT_NAME], int n)
{
  if ( n != 0 ) return 0;

  chunk_header_t hd;
  if ( qlread(li, &hd, sizeof(hd)) != sizeof(hd) ) return 0;
  if ( hd.ChunkFileId != AOF_MAGIC && hd.ChunkFileId != AOF_MAGIC_B ) return 0;

  qstrncpy(fileformatname, "ARM Object File", MAX_FILE_FORMAT_NAME);
  return 1;
}

//--------------------------------------------------------------------------
static void create32(sel_t sel, ea_t startEA, ea_t endEA,
                                   const char *name, const char *classname)
{
  set_selector(sel, 0);
  if(!add_segm(sel, startEA, endEA, name, classname)) loader_failure();
//  set_segm_addressing(getseg(startEA), 1);
}

//--------------------------------------------------------------------------
static ea_t get_area_base(int idx)
{
  segment_t *s = get_segm_by_sel(idx+1);
  if ( s == NULL ) return BADADDR;
  return s->startEA;
}

//--------------------------------------------------------------------------
static ea_t find_area(area_header_t *ah,
                            int maxarea,
                            const char *strings,
                            const char *areaname)
{
  for ( int i=0; i < maxarea; i++,ah++ )
    if ( strcmp(strings+size_t(ah->name),areaname) == 0 )
      return get_area_base(i);
  return BADADDR;
}

//--------------------------------------------------------------------------
static ea_t create_spec_seg(int *nsegs,int nelem,
                                  const char *name,uchar segtype)
{
  ea_t ea = BADADDR;
  if ( nelem != 0 )
  {
    nelem *= 4;
    ea = freechunk(inf.maxEA,nelem,0xFFF);
    (*nsegs)++;
    create32(*nsegs,ea,ea+nelem,name,CLASS_DATA);
    segment_t *s = getseg(ea);
    s->type = segtype;
    s->update();
    set_arm_segm_flags(s->startEA, 2 << SEGFL_SHIFT); // alignment
  }
  return ea;
}

//--------------------------------------------------------------------------
static void process_name(ea_t ea,const char *name,ulong flags,int iscode)
{
  if ( flags & SF_PUB )
  {
    add_entry(ea,ea,name,iscode);
    make_name_public(ea);
  } else {
    do_name_anyway(ea,name);
  }
  if ( flags & SF_WEAK  ) make_name_weak(ea);
  if ( flags & SF_ICASE ) add_long_cmt(ea,1,"Case-insensitive label");
  if ( flags & SF_STRNG ) add_long_cmt(ea,1,"Strong name");
}

//--------------------------------------------------------------------------
static void reloc_insn(ea_t ea,ulong rvalue,ulong type)
{
  ulong code = get_long(ea);
  switch ( (code >> 24) & 0xF )
  {
    case 0x0A:  // B
    case 0x0B:  // BL
      {
        long off = code & 0x00FFFFFFL;
        if ( off & 0x00800000L ) off |= ~0x00FFFFFFL; // extend sign
        off <<= 2;
        off += rvalue;
        off >>= 2;
        off &= 0xFFFFFFL;
        code &= 0xFF000000L;
        code |= off;
        put_long(ea,code);
      }
      break;
    default:
      warning("This relocation type is not implemented yet\n"
              "\3%a: reloc insn rvalue=%lx, rt=%lx", ea, rvalue,
              type & RF_II);
      break;
  }
}

//--------------------------------------------------------------------------
inline void swap_chunk_entry(chunk_entry_t *ce)
{
  ce->file_offset = swap32(ce->file_offset);
  ce->size        = swap32(ce->size);
}

//--------------------------------------------------------------------------
static void swap_aof_header(aof_header_t *ahd)
{
  ahd->obj_file_type = swap32(ahd->obj_file_type);
  ahd->version       = swap32(ahd->version);
  ahd->num_areas     = swap32(ahd->num_areas);
  ahd->num_syms      = swap32(ahd->num_syms);
  ahd->entry_area    = swap32(ahd->entry_area);
  ahd->entry_offset  = swap32(ahd->entry_offset);
}

//--------------------------------------------------------------------------
static void swap_area_header(area_header_t *ah)
{
  ah->name           = swap32(ah->name);
  ah->flags          = swap32(ah->flags);
  ah->size           = swap32(ah->size);
  ah->num_relocs     = swap32(ah->num_relocs);
  ah->baseaddr       = swap32(ah->baseaddr);
}

//--------------------------------------------------------------------------
static void swap_sym(sym_t *s)
{
  s->name            = swap32(s->name);
  s->flags           = swap32(s->flags);
  s->value           = swap32(s->value);
  s->area            = swap32(s->area);
}

//--------------------------------------------------------------------------
//
//      load file into the database.
//
void load_file(linput_t *li, ushort /*neflag*/, const char * /*fileformatname*/)
{
  int i;
  chunk_header_t hd;
  if ( ph.id != PLFM_ARM )
    set_processor_type("arm", SETPROC_ALL|SETPROC_FATAL);
  lread(li, &hd, sizeof(hd));
  if ( hd.ChunkFileId == AOF_MAGIC_B )             // BIG ENDIAN
  {
    inf.mf = 1;
    hd.max_chunks = swap32(hd.max_chunks);
    hd.num_chunks = swap32(hd.num_chunks);
  }

  chunk_entry_t *ce = qnewarray(chunk_entry_t,size_t(hd.max_chunks));
  if ( ce == NULL )
    nomem();
  lread(li, ce, sizeof(chunk_entry_t)*size_t(hd.max_chunks));
  if ( inf.mf )
    for ( i=0; i < hd.max_chunks; i++ )
      swap_chunk_entry(ce+i);

  int head = -1; // AOF Header
  int area = -1; // Areas
  int idfn = -1; // Identification
  int symt = -1; // Symbol Table
  int strt = -1; // String Table

  for ( i=0; i < hd.max_chunks; i++ )
  {
    if ( ce[i].file_offset == 0 )
      continue;
    if ( strncmp(ce[i].chunkId,OBJ_HEAD,sizeof(ce[i].chunkId)) == 0 ) head = i;
    if ( strncmp(ce[i].chunkId,OBJ_AREA,sizeof(ce[i].chunkId)) == 0 ) area = i;
    if ( strncmp(ce[i].chunkId,OBJ_IDFN,sizeof(ce[i].chunkId)) == 0 ) idfn = i;
    if ( strncmp(ce[i].chunkId,OBJ_SYMT,sizeof(ce[i].chunkId)) == 0 ) symt = i;
    if ( strncmp(ce[i].chunkId,OBJ_STRT,sizeof(ce[i].chunkId)) == 0 ) strt = i;
  }
  if ( head == -1 || area == -1 )
  {
    qfree(ce);
    loader_failure("Header or Area chunk is missing");
  }

  char *strings = (char *)read_chunk(li,ce,strt);
  aof_header_t *ahd = (aof_header_t *)read_chunk(li,ce,head);
  if ( inf.mf )
    swap_aof_header(ahd);

//
//      Areas
//

  area_header_t *ah = (area_header_t *)(ahd + 1);
  if ( inf.mf )
    for ( i=0; i < ahd->num_areas; i++ )
      swap_area_header(ah+i);
  ulong offset = ce[area].file_offset;
  inf.specsegs = 1;
  ea_t ea = toEA(inf.baseaddr,0);
  for ( i=0; i < ahd->num_areas; i++,ah++ )
  {
    if ( ah->flags & AREA_DEBUG )
    {
      offset += ah->size;
      offset += ah->num_relocs * sizeof(reloc_t);
      continue;
    }
    if ( ah->flags & AREA_ABS )
    {
      ea = ah->baseaddr;
      if ( freechunk(ea,ah->size,1) != ea )
        error("Can not allocate area at %a",ea);
    }
    else
    {
      ea = freechunk(ea,ah->size,0xFFF);
    }
    if ( (ah->flags & AREA_BSS) == 0 )
    {
      file2base(li, offset, ea, ea+ah->size, FILEREG_PATCHABLE);
      offset += ah->size;
    }
    const char *name = strings + size_t(ah->name);
    const char *classname;
    if ( ah->flags & AREA_CODE ) classname = CLASS_CODE;
    else if ( ah->flags & (AREA_BSS|AREA_COMREF) ) classname = CLASS_BSS;
    else classname = CLASS_DATA;
    create32(i+1, ea, ea+ah->size, name, classname);

    segment_t *s = getseg(ea);
    ushort sflags = (ah->flags & 0x1F) << SEGFL_SHIFT;       // alignment
    if ( ah->flags & AREA_BASED  )               sflags |= (SEGFL_BASED|ah->get_based_reg());
    if ( ah->flags & AREA_PIC    )               sflags |= SEGFL_PIC;
    if ( ah->flags & AREA_REENTR )               sflags |= SEGFL_REENTR;
    if ( ah->flags & AREA_HALFW  )               sflags |= SEGFL_HALFW;
    if ( ah->flags & AREA_INTER  )               sflags |= SEGFL_INTER;
    if ( ah->flags & AREA_COMMON )               sflags |= SEGFL_COMDEF;
    if ( ah->flags & (AREA_COMMON|AREA_COMREF) ) s->comb = scCommon;
    if ( ah->flags & AREA_RDONLY )               s->perm = SEGPERM_READ;
    if ( ah->flags & AREA_ABS    )               s->align = saAbs;
    s->update();
    set_arm_segm_flags(s->startEA, sflags);

    if ( i == 0 )
    {
      create_filename_cmt();
      char *id = (char *)read_chunk(li,ce,idfn);
      add_pgm_cmt("Translator  : %s",id);
      qfree(id);
    }

    if ( ah->flags & AREA_CODE )
    {
      if ( (ah->flags & AREA_32BIT)  == 0 ) add_pgm_cmt("The 26-bit area");
      if ( (ah->flags & AREA_EXTFP)  != 0 ) add_pgm_cmt("Extended FP instructions are used");
      if ( (ah->flags & AREA_NOCHK)  != 0 ) add_pgm_cmt("No Software Stack Check");
      if ( (ah->flags & AREA_THUMB)  != 0 ) add_pgm_cmt("Thumb code area");
    }
    else
    {
      if ( (ah->flags & AREA_SHARED)  != 0 ) add_pgm_cmt("Shared Library Stub Data");
    }
    ea += ah->size;
    offset += ah->num_relocs * sizeof(reloc_t);
  }
  int nsegs = i;

//
//      Symbol Table
//

  ah = (area_header_t *)(ahd + 1);
  ulong *delta = qnewarray(ulong, size_t(ahd->num_syms));
  if ( delta == NULL ) nomem();
  memset(delta, 0, sizeof(ulong)*size_t(ahd->num_syms));
  sym_t *syms = (sym_t *)read_chunk(li, ce, symt);
  if ( inf.mf ) for ( i=0; i < ahd->num_syms; i++ ) swap_sym(syms+i);
  int n_undef = 0;
  int n_abs   = 0;
  int n_comm  = 0;

  for ( i=0; i < ahd->num_syms; i++ )
  {
    sym_t *s = syms + i;
    if ( s->flags & SF_DEF )
    {
      if ( s->flags & SF_ABS )
      {
        n_abs++;
      }
      else
      {
        const char *areaname = strings + size_t(s->area);
        ea_t areabase = find_area(ah,size_t(ahd->num_areas),strings,areaname);
        delta[i] = areabase;
        ea_t ea = areabase + s->value;
        const char *name = strings + size_t(s->name);
        if ( s->value == 0 && strcmp(areaname,name) == 0 ) continue; // HACK!
        process_name(ea,name,s->flags,segtype(areabase) == SEG_CODE);
      }
    }
    else
    {
      if ( (s->flags & SF_PUB) && (s->flags & SF_COMM) )   // ref to common
        n_comm++;
      else
        n_undef++;
    }
  }

  ea_t abs_ea   = create_spec_seg(&nsegs, n_abs,   NAME_ABS,   SEG_ABSSYM);
  ea_t undef_ea = create_spec_seg(&nsegs, n_undef, NAME_UNDEF, SEG_XTRN);
  ea_t comm_ea  = create_spec_seg(&nsegs, n_comm,  NAME_COMMON,  SEG_COMM);

  if ( n_abs+n_undef+n_comm != 0 )
  {
    for ( i=0; i < ahd->num_syms; i++ )
    {
      sym_t *s = syms + i;
      if ( s->flags & SF_DEF )
      {
        if ( s->flags & SF_ABS )
        {
          put_long(abs_ea, s->value);
          process_name(abs_ea,strings+size_t(s->name),s->flags,0);
          doDwrd(abs_ea,4);
          delta[i] = s->value;
          s->value = abs_ea - delta[i];
          abs_ea += 4;
        }
      }
      else
      {
        if ( (s->flags & SF_PUB) && (s->flags & SF_COMM) )   // ref to common
        {
          put_long(comm_ea, s->value);
          process_name(comm_ea,strings+size_t(s->name),s->flags,0);
          delta[i] = comm_ea;
          comm_ea += 4;
        }
        else
        {
          put_long(undef_ea, 0xE1A0F00E);       // RET
          process_name(undef_ea,strings+size_t(s->name),s->flags,0);
          delta[i] = undef_ea;
          undef_ea += 4;
        }
        s->value = 0;
      }
    }
  }

//
//      Relocations
//

  offset = ce[area].file_offset;
  for ( i=0; i < ahd->num_areas; i++,ah++ )
  {
    if ( ah->flags & AREA_DEBUG )
    {
      offset += ah->size;
      offset += ah->num_relocs * sizeof(reloc_t);
      continue;
    }
    if ( (ah->flags & AREA_BSS) == 0 ) offset += ah->size;
    qlseek(li, offset);
    ea_t base = get_area_base(i);
    for ( int j=0; j < ah->num_relocs; j++ )
    {
      reloc_t r;
      lread(li, &r, sizeof(reloc_t));
      if ( inf.mf ) {
        r.type   = swap32(r.type);
        r.offset = swap32(r.offset);
      }
      size_t sid = r.sid();
      ulong rvalue;
      ulong target;
      fixup_data_t fd;
      if ( r.type & RF_A )
      {
        if ( sid >= ahd->num_syms )
          loader_failure("Bad relocation record at file offset %lx",qltell(li)-sizeof(reloc_t));
        rvalue = delta[sid];
        target = syms[sid].value + rvalue;
        fd.type = FIXUP_EXTDEF;
      }
      else
      {
        rvalue = get_area_base(sid);
        target = rvalue;
        if ( rvalue == BADADDR )
          loader_failure("Bad reference to area %ld at file offset %lx",sid,qltell(li)-sizeof(reloc_t));
        fd.type = 0;
      }
      segment_t *s = getseg(target);
      if ( s == NULL )
        loader_failure("Can't find area for relocation target %lx at %lx",target,qltell(li)-sizeof(reloc_t));
      fd.sel = s->sel;
      fd.off = target - get_segm_base(s);
      if ( (r.type & RF_R) != 0 )
      {
        fd.type |= FIXUP_SELFREL;
        if ( (r.type & RF_A) != 0 )
        {
                // R=1 B=0 or R=1 B=1
                // This specifies PC-relative relocation: to the subject field
                // is added the difference between the relocation value and
                // the base of the area containing the subject field.
                // In pseudo C:
                //   subject_field = subject_field +
                //                    (relocation_value -
                //                  base_of_area_containing(subject_field))
          rvalue -= base;
        }
        else
        {
                //
                // As a special case, if A is 0, and the relocation value is
                // specified as the base of the area containing the subject
                // field, it is not added and:
                //   subject_field = subject_field -
                //                  base_of_area_containing(subject_field)
                // This caters for relocatable PC-relative branches to fixed
                // target addresses. If R is 1, B is usually 0. A B value of 1
                // is used to denote that the inter-link-unit value of a
                // branch destination is to be used, rather than the more
                // usual intra-link-unit value.
          rvalue = -base;
        }
      }
      else
      {
        if ( (r.type & RF_B) != 0 )
        {       // R=0 B=1
                // This specifies based area relocation. The relocation value
                // must be an address within a based data area. The subject
                // field is incremented by the difference between this value
                // and the base address of the consolidated based area group
                // (the linker consolidates all areas based on the same base
                // register into a single, contiguous region of the output
                // image). In pseudo C:
                //   subject_field = subject_field +
                //                      (relocation_value -
                //           base_of_area_group_containing(relocation_value))
                // For example, when generating re-entrant code, the C
                // compiler places address constants in an adcon area based
                // on register sb, and loads them using sb relative LDRs.
                // At link time, separate adcon areas will be merged and sb
                // will no longer point where presumed at compile time. B
                // type relocation of the LDR instructions corrects for this.
           rvalue -= get_area_base(sid);
        }
        else
        {       // R=0 B=0
                // This specifies plain additive relocation: the relocation
                // value is added to the subject field. In pseudo C:
                //      subject_field = subject_field + relocation_value
           /* nothing to do */;
        }
      }
      ea_t ea = base + r.offset;
      switch ( r.type & RF_FT )
      {
        case RF_FT_BYTE: // 00 the field to be relocated is a byte
          fd.type |= FIXUP_BYTE;
          fd.displacement = get_byte(ea);
          add_byte(ea,rvalue);
          break;
        case RF_FT_HALF: // 01 the field to be relocated is a halfword (two bytes)
          fd.type |= FIXUP_OFF16;
          fd.displacement = get_word(ea);
          add_word(ea,rvalue);
          break;
        case RF_FT_WORD: // 10 the field to be relocated is a word (four bytes)
          fd.type |= FIXUP_OFF32;
          fd.displacement = get_long(ea);
          add_long(ea,rvalue);
          break;
        case RF_FT_INSN: // 11 the field to be relocated is an instruction or instruction sequence
          reloc_insn(ea,rvalue,r.type);
          break;
      }
      set_fixup(ea,&fd);
    }
    offset += ah->num_relocs * sizeof(reloc_t);
  }

  if ( ahd->entry_area != 0 )
  {
    inf.start_cs = ahd->entry_area;
    inf.startIP  = ahd->entry_offset;
    inf.beginEA  = ahd->entry_offset;
  }

  qfree(syms);
  qfree(delta);
  qfree(ahd);
  qfree(strings);
  qfree(ce);
  inf.baseaddr = 0;
}

//----------------------------------------------------------------------
bool idaapi init_loader_options(linput_t*)
{
  set_processor_type("arm", SETPROC_ALL|SETPROC_FATAL);
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
//      create output file from the database.
//      this function may be absent.
//
  NULL,
//      take care of a moved segment (fix up relocations, for example)
  NULL,
//      initialize user configurable options based on the input file.
  init_loader_options,
};
