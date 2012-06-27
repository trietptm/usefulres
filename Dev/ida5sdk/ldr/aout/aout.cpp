/*
 *  This Loader Module is written by Yury Haron
 *
 */

/*
  L O A D E R  for a.out (Linux)
*/

#include "../idaldr.h"
#include "aout.h"
#include "common.cpp"

//--------------------------------------------------------------------------
//
//      check input file format. if recognized, then return 1
//      and fill 'fileformatname'.
//      otherwise return 0
//
int accept_file(linput_t *li, char fileformatname[MAX_FILE_FORMAT_NAME], int n)
{
  if(n) return(0);

  int i = get_aout_file_format_index(li);
  if ( i == 0 ) return 0;

  static char *ff[] = {
                "demand-paged executable with NULL-ptr check", //q
                "object or impure executable",                 //o
                "demand-paged executable",                     //z
                "core",                                        //c
                "pure executable",                             //n
                "OpenBSD demand-paged executable",             //zo
               };
  qsnprintf(fileformatname, MAX_FILE_FORMAT_NAME, "a.out (%s)", ff[i-1]);
  return(f_AOUT);
}

//--------------------------------------------------------------------------
static void create32(ushort sel, ea_t startEA, ea_t endEA, char *name, char *classname)
{
  set_selector(sel, 0);
  if(!add_segm(sel, startEA, endEA, name, classname)) loader_failure();
  if(ph.id == PLFM_386) set_segm_addressing(getseg(startEA), 1);
}

//--------------------------------------------------------------------------
void ana_hdr(linput_t *li, exec &ex)
{
  lread(li, &ex, sizeof(ex));

  if(N_BADMAG(ex))
  {
    ex.a_info = swap32(ex.a_info);
    msg("Possible netBSD (SCO) format?\n");
  }
  if(N_MACHTYPE(ex)) switch(N_MACHTYPE(ex)) {
      case M_386:
      case M_386_NETBSD:
        if(ph.id != PLFM_386) set_processor_type("80386r", SETPROC_ALL|SETPROC_FATAL);
        break;

      case M_ARM:
      case M_ARM6_NETBSD:
        if(ph.id != PLFM_ARM) set_processor_type("arm", SETPROC_ALL|SETPROC_FATAL);
        break;

      default:
        loader_failure("Unsupported or unknown machine type");
  } else if(ph.id != PLFM_386) warning("Missing machine type. Continue?");
}

//--------------------------------------------------------------------------
//
//      load file into the database.
//
void load_file(linput_t *li, ushort /*neflag*/, const char * /*fileformatname*/)
{
  exec ex;
  ana_hdr(li, ex);

  int txtoff = N_TXTOFF(ex);
  int txtadr = (ph.id == PLFM_ARM) ? (N_TXTADDR_ARM(ex)) : (N_TXTADDR(ex));
  switch(N_MAGIC(ex)) {
//    case NMAGIC:
//    case CMAGIC:
    default:
        loader_failure("This image type is not supported yet");
      break;

    case ZMAGIC:
      if ( qlsize(li) < ex.a_text + ex.a_data + N_SYMSIZE(ex) + txtoff )
      {
        txtoff = 0;
        txtadr = 0x1000;
      }
      else
        if ( txtoff < 512)
          loader_failure("Size of demand page < size of block");
    case QMAGIC:
      if(ex.a_text & 0xFFF || ex.a_data & 0xFFF)
                                  loader_failure("Executable is not page aligned");
      break;

    case OMAGIC:
      txtoff = sizeof(ex);
      break;
  }
  if(N_TRSIZE(ex) || N_DRSIZE(ex))
    loader_failure("Relocation in image file is not supported yet");

//  if(ex.a_text + ex.a_data == 0) loader_failure("Empty file");

  inf.baseaddr = 0;

  ulong base, top;
  top = base = txtadr;
  if(ex.a_text || ex.a_data) {
    top += ex.a_text;
//    msg("txtoff=%d, base=%d top=%d end=%d\n", txtoff, base, top, top+ex.a_data);
    file2base(li, txtoff, base, top + ex.a_data, FILEREG_PATCHABLE);
    if(ex.a_text) {
      create32(1, base, top, NAME_CODE, CLASS_CODE);
      inf.start_cs = 1;
      inf.startIP  = ex.a_entry;
    }
    if(ex.a_data) {
      base = top;
      create32(2, base, top += ex.a_data, NAME_DATA, CLASS_DATA);
      set_default_dataseg(2);
    }
  }
  if(ex.a_bss) create32(3, top, top + ex.a_bss, NAME_BSS, CLASS_BSS);

// We come in here for the regular a.out style of shared libraries */
//      ((ex.a_entry & 0xfff) && N_MAGIC(ex) == ZMAGIC) ||
//    return -ENOEXEC;
//  }
// For  QMAGIC, the starting address is 0x20 into the page.  We mask
//   this off to get the starting address for the page */
//  start_addr =  ex.a_entry & 0xfffff000;
//////

  create_filename_cmt();
  add_pgm_cmt("Flag value: %lXh", N_FLAGS(ex));
}

//----------------------------------------------------------------------
bool idaapi init_loader_options(linput_t *li)
{
  exec ex;
  ana_hdr(li, ex);
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
