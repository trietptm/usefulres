/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 */

#include "../idaldr.h"
#include <expr.hpp>
#include "pilot.hpp"

#include "common.cpp"
//------------------------------------------------------------------------
static void describe_all(DatabaseHdrType &h)
{
  create_filename_cmt();
  add_pgm_cmt("Version     : %04X",h.version);
  add_pgm_cmt("DatabaseType: %4.4s",&h.type);
  add_pgm_cmt("\n appl \"%s\", '%4.4s'",h.name,&h.id);
}

//------------------------------------------------------------------------
//
// Here's the structure of a Metrowerks DATA 0 resource:
//
// +---------------------------------+
// | long:   offset of CODE 1 xrefs  |---+
// +---------------------------------+   |
// | char[]: compressed init data    |   |
// +---------------------------------+   |
// | char[]: compressed DATA 0 xrefs |   |
// +---------------------------------+   |
// | char[]: compressed CODE 1 xrefs |<--+
// +---------------------------------+
//
static ulong unpack_data0000(linput_t *li, long fpos, size_t size, ea_t ea)
{
  uchar *packed = qnewarray(uchar, size);
  uchar *packed_sav = packed;
  if ( packed_sav == NULL ) nomem("data0000 (size=%lu)",size);
  qlseek(li, fpos);
  lread(li, packed, size);
  ulong usize = 0;              // unpacked size
  packed += sizeof(ulong);      // skip data size
//  for ( int i=0; i < 3; i++ ) {       // we decompress only the data (no xrefs)
    long offset = swap32(*(ulong*)packed);
    packed += sizeof(ulong);
    ulong cea = ea;
    ulong a5;
    if ( offset >= 0 )
    {
      cea = ea + offset;
      FlagsEnable(ea,cea);      // make sure there are at least uninited bytes
                                // prior to our chunk
      a5 = ea;
    } else {
      usize = -offset + 1;
      FlagsEnable(ea,ea+usize);
      a5 = ea + usize - 1;
      doByte(a5,1);
      static const char mesg[] =
        "A5 register does not point to the start of the data segment\n"
        "\3The file should not be recompiled using Pila\n";
      info(mesg);
      add_pgm_cmt(mesg);
    }
    set_name(a5,"A5BASE",SN_AUTO);
    set_cmt(a5,"A5 points here",0);

// Pilot understands enhanced RLE

    while ( 1 )
    {
      char buf[256];
      uchar cnt = *packed++;
      if ( cnt == 0 ) break;
      if ( cnt & 0x80 )
      {
        cnt = (cnt & 0x7F) + 1;
        mem2base(packed, cea, cea+cnt, fpos+size_t(packed-packed_sav));
        packed += cnt;
        cea += cnt;
        continue;
      }
      if ( cnt & 0x40 )
      {
        cnt = (cnt & 0x3F) + 1;
        memset(buf, 0, cnt);
      }
      else if ( cnt & 0x20 )
      {
        cnt = (cnt & 0x1F) + 2;
        memset(buf, *packed++, cnt);
      }
      else if ( cnt & 0x10 )
      {
        cnt = (cnt & 0x0F) + 1;
        memset(buf, 0xFF, cnt);
      }
      else if ( cnt == 1 )
      {
        buf[0] = 0x00;
        buf[1] = 0x00;
        buf[2] = 0x00;
        buf[3] = 0x00;
        buf[4] = 0xFF;
        buf[5] = 0xFF;
        buf[6] = *packed++;
        buf[7] = *packed++;
        cnt = 8;
      }
      else if ( cnt == 2 )
      {
        buf[0] = 0x00;
        buf[1] = 0x00;
        buf[2] = 0x00;
        buf[3] = 0x00;
        buf[4] = 0xFF;
        buf[5] = *packed++;
        buf[6] = *packed++;
        buf[7] = *packed++;
        cnt = 8;
      }
      else if ( cnt == 3 )
      {
        buf[0] = 0xA9;
        buf[1] = 0xF0;
        buf[2] = 0x00;
        buf[3] = 0x00;
        buf[4] = *packed++;
        buf[5] = *packed++;
        buf[6] = 0x00;
        buf[7] = *packed++;
        cnt = 8;
      }
      else if ( cnt == 4 )
      {
        buf[0] = 0xA9;
        buf[1] = 0xF0;
        buf[2] = 0x00;
        buf[3] = *packed++;
        buf[4] = *packed++;
        buf[5] = *packed++;
        buf[6] = 0x00;
        buf[7] = *packed++;
        cnt = 8;
      }
      mem2base(buf, cea, cea+cnt, -1);
      cea += cnt;
    }
    cea -= ea;
    if ( usize < cea ) usize = cea;
//  }
  // we don't process relocation information!
  // (it should be processed here)
  qfree(packed_sav);
  return usize;
}

//------------------------------------------------------------------------
void load_file(linput_t *li,ushort /*_neflags*/,const char * /*fileformatname*/)
{
  int i;
  sel_t dgroup = BADSEL;
  set_processor_type("68K", SETPROC_ALL);
  set_target_assembler(2);              // PalmPilot assembler Pila
  DatabaseHdrType h;
  lread(li,&h,sizeof(h));
  swap_prc(h);
  size_t size = sizeof(ResourceMapEntry) * h.numRecords;
  ResourceMapEntry *re = qnewarray(ResourceMapEntry, h.numRecords);
  if ( re == NULL ) nomem("resource map entries (size=%u)\n",size);
  lread(li,re,size);
  for ( i=0; i < h.numRecords; i++ ) swap_resource_map_entry(re[i]);
  // determine the bss size
  size_t bss_size = 0;
  for ( i=0; i < h.numRecords; i++ )
  {
    if ( re[i].fcType == PILOT_RSC_CODE && re[i].id == 0 ) // code0000
    {
      code0000_t code0000;
      qlseek(li, re[i].ulOffset);
      lread(li, &code0000, sizeof(code0000));
      swap_code0000(&code0000);
      bss_size = code0000.bss_size;
      break;
    }
  }
  // load the segment contents
  for ( i=0; i < h.numRecords; i++ )
  {
    ulong size;
    if ( i == (h.numRecords-1) )
      size = qlsize(li);
    else
      size = re[i+1].ulOffset;
    if ( size < re[i].ulOffset )
      loader_failure("Invalid file structure");
    size -= re[i].ulOffset;
    ea_t ea1 = (inf.maxEA + 0xF) & ~0xF;
    ea_t ea2 = ea1 + size;

    char segname[10];
    qsnprintf(segname, sizeof(segname), "%4.4s%04X", &re[i].fcType, re[i].id);
    const char *sclass = "RSC";
    if ( strncmp(segname,"code",4)  == 0 )
    {
      if ( strcmp(segname,"code0000") == 0 ) continue;  // skip code0000 resource
      sclass = CLASS_CODE;
      if ( re[i].id == 1 )
      {
        inf.start_cs = i + 1;
        inf.startIP  = 0;
      }
    }
    if ( strcmp(segname, "data0000") == 0 )
    {
      sclass = CLASS_DATA;
      dgroup = i + 1;
      ulong usize = unpack_data0000(li, re[i].ulOffset, size, ea1+bss_size);
      ea2 = ea1 + bss_size + usize;
    }
    else
    {
      file2base(li, re[i].ulOffset, ea1, ea2, FILEREG_PATCHABLE);
    }
    set_selector(i+1, ea1 >> 4);
    if ( !add_segm(i+1, ea1, ea2, segname, sclass) )
      loader_failure();
  }
  qfree(re);
  if ( dgroup != BADSEL ) set_default_dataseg(dgroup);  // input: selector
  describe_all(h);
  dosysfile(1,"pilot.idc");
}

//--------------------------------------------------------------------------
int accept_file(linput_t *li,char fileformatname[MAX_FILE_FORMAT_NAME],int n)
{
  if ( n != 0 ) return 0;
  if ( is_prc_file(li) )
  {
    qstrncpy(fileformatname,"PalmPilot program file",MAX_FILE_FORMAT_NAME);
    return f_PRC;
  }
  return 0;
}

//--------------------------------------------------------------------------
bool idaapi init_loader_options(linput_t*)
{
  set_processor_type("68K", SETPROC_ALL);
  return true;
}

//--------------------------------------------------------------------------
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
