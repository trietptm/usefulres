/*
  IDA LOADER  for QNX 16/32 bit executables
  (c) Zhengxi Ltd., 1998.

  start: 25.07.98
  end:   26.07.98

changed:
  28.07.98  Yury Haron
  09.08.98  Denis Petrov
  10.08.98  YH - patch to new sdk format

*/


#include "../idaldr.h"
#include "lmf.h"

//--------------------------------------------------------------------------
//
//  check input file format. if recognized, then return 1
//  and fill 'fileformatname'.
//  otherwise return 0
//
int accept_file(linput_t *li, char fileformatname[MAX_FILE_FORMAT_NAME], int n)
{
  ex_header ex;                 // ��������� lmf
  ulong     n_segments; // �᫮ ᥣ���⮢

  if(n) return(0);

  if (qlread(li, &ex, sizeof(ex)) != sizeof(ex)) return 0;
  if (0    != ex.lmf_header.rec_type    ) return 0;
  if (0    != ex.lmf_header.zero1       ) return 0;
//  if (0x38 != ex.lmf_header.data_nbytes ) return 0;
  if (0    != ex.lmf_header.spare       ) return 0;
  if (386  != ex.lmf_definition.cpu
   && 286  != ex.lmf_definition.cpu     ) return 0;

  n_segments = (ex.lmf_header.data_nbytes - sizeof(_lmf_definition))
               / sizeof (ulong);

  if ( (MIN_SEGMENTS > n_segments) || (n_segments > MAX_SEGMENTS) )
     return 0;


  _lmf_data lmf_data;
  lmf_data.segment_index = -1;

  for(ulong at = sizeof(ex.lmf_header)+ex.lmf_header.data_nbytes;
      lmf_data.segment_index != _LMF_EOF_REC;
      at += sizeof(lmf_data) + lmf_data.offset)
  {
    qlseek( li, at, 0 );
    if ( sizeof(_lmf_data) !=
            qlread( li, &lmf_data, sizeof(_lmf_data) ) ) return 0;
    switch(lmf_data.segment_index)
    {
      case _LMF_DEFINITION_REC:
        return 0;
      case _LMF_COMMENT_REC:
        break;
      case _LMF_DATA_REC:
        break;
      case _LMF_FIXUP_SEG_REC:
        break;
      case _LMF_FIXUP_80X87_REC:
        break;
      case _LMF_EOF_REC:
        if ( lmf_data.offset != sizeof(_lmf_eof) ) return 0;
        break;
      case _LMF_RESOURCE_REC:
        break;
      case _LMF_ENDDATA_REC:
        if ( lmf_data.offset != 6 /*sizeof(???)*/ ) return 0;
        break;
      case _LMF_FIXUP_LINEAR_REC:
        break;
      case _LMF_PHRESOURCE:
        return 0;
      default:
        return 0;
    }
  }

  qsnprintf(fileformatname, MAX_FILE_FORMAT_NAME, "QNX %d-executable",
           (_PCF_32BIT & ex.lmf_definition.cflags) ? 32 : 16);
  return(f_LOADER);
}



//--------------------------------------------------------------------------
//
//  load file into the database.
//
//#define _CODE   0
//#define _DATA   1
//#define _BSS    2
//#define _STACK  3
//#define MAXSEG  2

void load_file(linput_t *li, ushort /*neflag*/, const char * /*fileformatname*/)
{
  ex_header ex;         // ��������� lmf
  ulong     n_segments; // �᫮ ᥣ���⮢
  ulong     nseg;       // watcom 10.6 not working properly without this!

  if ( ph.id != PLFM_386 )
    set_processor_type("80386r", SETPROC_ALL|SETPROC_FATAL);

  qlseek(li, 0);
  lread(li, &ex, sizeof(ex));

  struct {
    ulong minea,topea;
  } perseg[MAX_SEGMENTS];

  n_segments = (ex.lmf_header.data_nbytes - sizeof(_lmf_definition))
               / sizeof (ulong);

  for (nseg=0; nseg<n_segments; nseg++)
  {
    perseg[nseg].minea =
      nseg==0
                ? ex.lmf_definition.flat_offset
        : (perseg[nseg-1].topea + 0x0FFF) & ~0x0FFF;
    perseg[nseg].topea = perseg[nseg].minea + ( ex.segsizes[nseg] & 0x0FFFFFFFl );
  }

  ulong ring = (_PCF_PRIVMASK &ex.lmf_definition.cflags)>>2;

//  ulong myselector = 0x04 + ring;

// ᥫ����� � LDT �� ���浪�.
#define LDT_SELECTOR(nseg) (((nseg)<<3)+0x04+ring)
#define ISFLAT (_PCF_FLAT &ex.lmf_definition.cflags)

  inf.baseaddr =  0;
  if (ISFLAT)
  {
    inf.lflags     |= LFLG_PC_FLAT;
//    inf.s_prefflag &= ~PREF_SEGADR;
//    inf.nametype   =  NM_EA4;
  }

  inf.startIP  = ex.lmf_definition.code_offset;
  if (ISFLAT)
    inf.startIP +=  perseg[ex.lmf_definition.code_index].minea;
  inf.start_cs = LDT_SELECTOR(ex.lmf_definition.code_index);

  _lmf_data lmf_data, lmf_data2;
  lmf_data.segment_index = -1;

  for(ulong at = sizeof(ex.lmf_header)+ex.lmf_header.data_nbytes;
      lmf_data.segment_index != _LMF_EOF_REC;
      at += sizeof(lmf_data) + lmf_data.offset)
  {
    qlseek( li, at );
    lread( li, &lmf_data, sizeof(_lmf_data) );
    switch(lmf_data.segment_index)
    {

      case _LMF_DEFINITION_REC:
        break;

      case _LMF_COMMENT_REC:
        break;

      case _LMF_DATA_REC:
        {
         lread( li, &lmf_data2, sizeof(_lmf_data) );
         ulong body_offset = perseg[lmf_data2.segment_index].minea
                             + lmf_data2.offset;
         ulong body_size   = lmf_data.offset-sizeof(_lmf_data);

         file2base(li,
                   at+sizeof(_lmf_data)+sizeof(_lmf_data),
                   body_offset,
                   body_offset + body_size,
                   FILEREG_PATCHABLE);
        }
        break;

      case _LMF_FIXUP_SEG_REC:
        fixup_data_t fd;
        ulong n_fixups;
        _lmf_seg_fixup lmf_seg_fixup;
        n_fixups = lmf_data.offset / sizeof(_lmf_seg_fixup);
        while(n_fixups--)
        {
          lread( li, &lmf_seg_fixup, sizeof(_lmf_seg_fixup) );
          ulong ea=lmf_seg_fixup.data[0].fixup_offset;
          ea += perseg[ lmf_seg_fixup.data[0].fixup_seg_index ].minea; // fix!
          fd.type = FIXUP_SEG16;
          fd.displacement = 0;
          fd.off = 0;
          fd.sel = get_word(ea); //lmf_seg_fixup.data[0].fixup_seg_index;
          set_fixup(ea, &fd);
        }
        break;

      case _LMF_FIXUP_80X87_REC: // �� ������ ������権 80x87
        break;

      case _LMF_EOF_REC:         // no interesting for ida
        break;

      case _LMF_RESOURCE_REC: // don't support now
        break;

      case _LMF_ENDDATA_REC:  // ��� �����-� 6 ����, �.�. ��
        break;

      case _LMF_FIXUP_LINEAR_REC:
        break;

      case _LMF_PHRESOURCE: // don't support now
        break;

    }
  }

  ulong itext=0;
  ulong idata=0;
  for (nseg=0; nseg<n_segments; nseg++)
  {
    ulong selector  = LDT_SELECTOR(nseg);
    char  seg_name[8], *seg_class;

    if ( (ex.segsizes[nseg]>>28) == _LMF_CODE )
    {
      qsnprintf(seg_name, sizeof(seg_name), "cseg_%.02d", ++itext);
      seg_class = CLASS_CODE;
    } else {
      qsnprintf(seg_name, sizeof(seg_name), "dseg_%.02d", ++idata);
      seg_class = CLASS_DATA;
    }

    set_selector( selector, ISFLAT ? 0 : perseg[nseg].minea>>4 );

    if(!add_segm( selector,
                  perseg[nseg].minea,  perseg[nseg].topea,
                  seg_name,  seg_class
                )) loader_failure();
    if (_PCF_32BIT     &ex.lmf_definition.cflags)
      set_segm_addressing( getseg(perseg[nseg].minea), 1 ); // 32bit

  }

  set_default_dataseg( LDT_SELECTOR(ex.lmf_definition.argv_index) );



  create_filename_cmt();
  add_pgm_cmt("Version     : %d.%d",
              ex.lmf_definition.version_no>>8,
              ex.lmf_definition.version_no&255 );

  add_pgm_cmt("Priv level  : %d",
              (_PCF_PRIVMASK &ex.lmf_definition.cflags)>>2);

  char str[MAXSTR], *p = str;
  char *e = str + sizeof(str);


  if (_PCF_LONG_LIVED&ex.lmf_definition.cflags) APPEND(p, e, " LONG_LIVED");
  if (_PCF_32BIT     &ex.lmf_definition.cflags) {
    if (p != str) APPCHAR(p, e, ',');
    APPEND(p, e, " 32BIT");
  }
  if (ISFLAT) {
    if (p != str) APPCHAR(p, e, ',');
    APPEND(p, e, " FLAT");
  }
  if (_PCF_NOSHARE   &ex.lmf_definition.cflags) {
    if (p != str) APPCHAR(p, e, ',');
    APPEND(p, e, " NOSHARE");
  }
  if ( p == str ) APPEND(p, e, " None");
  add_pgm_cmt("Code flags  :%s", str);

  // ig 08.09.00: Automatically load the Watcom signature file
  plan_to_apply_idasgn(ISFLAT ? "wa32qnx" : "wa16qnx");
}

//----------------------------------------------------------------------
bool idaapi init_loader_options(linput_t*)
{
  set_processor_type("80386r", SETPROC_ALL|SETPROC_FATAL);
  return true;
}

//----------------------------------------------------------------------
//
//  LOADER DESCRIPTION BLOCK
//
//----------------------------------------------------------------------
loader_t LDSC =
{
  IDP_INTERFACE_VERSION,
  0,        // loader flags
//
//  check input file format. if recognized, then return 1
//  and fill 'fileformatname'.
//  otherwise return 0
//
  accept_file,
//
//  load file into the database.
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
