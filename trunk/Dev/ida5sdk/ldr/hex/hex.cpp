#define BUFFSIZE  ((255+6)*2+76)    // buffer to read the string
#define MAX_BYTES  24               // Max number of bytes per line for write
/*
 *  This Loader Module is written by Ilfak Guilfanov and
 *                        rewriten by Yury Haron
 *
 */

/*
   Interesting documentation:

   http://www.intel.com/design/zapcode/Intel_HEX_32_Format.doc
   http://www.keil.com/support/docs/1584.htm

*/

#include "../idaldr.h"

//--------------------------------------------------------------------------
int accept_file(linput_t *li, char fileformatname[MAX_FILE_FORMAT_NAME], int n)
{
  char str[80];
  register char *p;

  if(n) return(0);

  qlgets(p = str, sizeof(str), li);

  while(*p == ' ') p++;
  int  type = 0;
  if(isxdigit((uchar)*(p+1)) && isxdigit((uchar)*(p+2))) switch(*p) {
    case ':':
      p = "Intel Hex Object Format";
      type = f_HEX;
      break;

    case ';':
      p = "MOS Technology Hex Object Format";
      type = f_MEX;
      break;

    case 'S':
      p = "Intel S-record Format";
      type = f_SREC;
    default:
      break;

  }
  if(type) qstrncpy(fileformatname, p, MAX_FILE_FORMAT_NAME);
  return(type);
}

//--------------------------------------------------------------------------
static struct local_data {
  union  {
    char   *ptr;  //load
    int    sz;    //write
  };
  union  {
    ulong  ln;    //load
    int    size;  //write
  };
  ushort sum;     //load/write
  uchar  len;     //load
}lc;
//--------------------------------------------------------------------------
static void errfmt(void)
{
  loader_failure("Bad hex input file format, line %lu", lc.ln);
}

//--------------------------------------------------------------------------
// reads the specified number of bytes from the input line
// if size==0, then initializes itself for a new line
static ulong hexdata(int size)
{
  char n[10], *endp;
  register char *p = n;
  register int  i = size;
  if(!i) i = 2;
  else {
    if(lc.len < i) errfmt();
    lc.len -= i;
    i <<= 1;
  }
  while(i--) *p++ = *lc.ptr++;
  *p = '\0';
  register ulong data = strtoul(n, &endp, 16);
  if(endp != p) errfmt();
  switch(size) {
    case 0:
      lc.len = (uchar)data;
      lc.sum = lc.len;
      break;

    case 4:
      lc.sum += (uchar)(data >> 24);
    case 3:
      lc.sum += (uchar)(data >> 16);
    case 2:
      lc.sum += (uchar)(data >> 8);
    default:  //1
      lc.sum += (uchar)data;
      break;
  }
  return data;
}

//--------------------------------------------------------------------------
void load_file(linput_t *li, ushort neflag, const char * /*fileformatname*/)
{
  char line[BUFFSIZE], bigaddr = 0;
  ea_t addr, startEA = toEA(inf.baseaddr, 0), endEA = 0, seg_start = 0;
  char rstart = (inf.filetype == f_SREC) ? 'S' :
                                     ((inf.filetype == f_HEX) ? ':' : ';');
  register char *p;

  memset(&lc, 0, sizeof(local_data));
  inf.startIP = BADADDR;          // f_SREC without start record

  bool iscode = (neflag & NEF_CODE) != 0;
  int nb = iscode ? ph.cnbits : ph.dnbits;      // number of bits in a byte
  int bs = (nb + 7) / 8;                        // number of bytes
  ushort sel = setup_selector(startEA >> 4);
  bool segment_created = false;

  if ( ph.id == PLFM_PIC )
  {
    static const char form[] =
//    "PIC HEX file addressing mode\n"
//    "\n"
    "There are two flavors of HEX files for PIC: with word addressing\n"
    "and with byte addressing. It is not possible to recognize the\n"
    "flavor automatically. Please specify what addressing mode should\n"
    "be used to load the input file. If you don't know, try both and\n"
    "choose the one which produces more meaningful result\n";
    int code = askbuttons_c("~B~yte addressing",
                            "~W~ord addressing",
                            "~C~ancel", 1, form);
    switch ( code )
    {
      case 1:
        break;
      case 0:
        bs = 1;
        break;
      default:
        loader_failure(NULL);
    }
  }

  ea_t subs_addr = 0;
  for(lc.ln = 1; qlgets(p = line, BUFFSIZE, li); lc.ln++)
  {
    while(*p == ' ') ++p;
    if ( *p == '\n' || *p == '\r' ) continue;
    if(*p++ != rstart) errfmt();

    int sz = 2;
    int mode = (inf.filetype == f_SREC) ? (uchar)*p++ : 0x100;
    lc.ptr = p;
    hexdata(0);
    if ( mode == 0x100 )
    {
      if ( !lc.len ) break;
      lc.len += 2;
      if ( inf.filetype == f_HEX )
        ++lc.len;
    }
    else
    {
      switch ( mode )
      {
        default:
            errfmt();

        case '0':
        case '5':
          continue;

        case '3':
        case '7':
          ++sz;
        case '2':
        case '8':
          ++sz;
        case '1':
        case '9':
          if(mode > '3') mode = 0;
          --lc.len;
          break;
      }
    }
    addr = hexdata(sz) / bs;
    if ( !mode )
    {
      inf.startIP = addr;
      continue;
    }

    if ( inf.filetype == f_HEX )
    {
      int type = hexdata(1);      // record type
      switch ( type )
      {
        case 0xFF:                // mitsubishi hex format
        case 4:                   // Extended linear address record
          subs_addr = hexdata(2) << 16;
          break;
        case 2:                   // Extended segment address record
          subs_addr = hexdata(2) << 4;
          break;
      }
      if ( type != 0 )
      {
        if ( type == 1 )
          break;                  // end of file record
        continue;                 // not a data record
      }
    }
    addr += subs_addr;
    if ( lc.len )
    {
      ea_t top = addr + lc.len / bs;
      p = line;
      while(lc.len) *p++ = (uchar)hexdata(1);
      if(top >= 0x10000l) bigaddr = 1;
      addr += startEA;
      showAddr(addr);
      if((top += startEA) > endEA || !segment_created)
      {
        endEA = top;
        if(neflag & NEF_SEGS)
        {
          if ( !segment_created )
          {
            if(!add_segm(sel, addr, endEA, NULL, CLASS_CODE)) loader_failure(NULL);
            segment_created = true;
            seg_start = addr;
          }
          else
            set_segm_end(getnseg(0)->startEA, endEA, true);
        }
      }
      if ( seg_start > addr )
      {
        set_segm_start(getnseg(0)->startEA, addr, true);
        seg_start = addr;
      }
      mem2base(line, addr, top, -1);
    }
    {
      ushort chi;       // checksum
      ++lc.len;
      switch(inf.filetype) {
        case f_SREC:
          chi = (uchar)(~lc.sum);
          chi ^= (uchar)hexdata(1);
          break;
        case f_HEX:
          hexdata(1);
          chi = (uchar)lc.sum;
          break;
        default:  //MEX
          ++lc.len;
          chi = lc.sum;
          chi -= hexdata(2);
          break;
      }
      if(chi) {
        static char first = 0;
        if(!first) {
          ++first;
          warning("Bad hex input file checksum, line %lu. Ignore?", lc.ln);
        }
      }
    }
  }

//  if(inf.filetype != f_SREC) inf.startIP = 0;

  if(neflag & NEF_SEGS) {
    if(bigaddr) {
      set_segm_addressing(getnseg(0), 1);
      if(ph.id == PLFM_386) inf.lflags |= LFLG_PC_FLAT;
//      inf.s_prefflag &= ~PREF_SEGADR;
//      inf.nametype = NM_EA4;
    }
    set_default_dataseg(sel);
    inf.start_cs  = sel;
  } else {
    FlagsEnable(startEA, endEA);
//    if(inf.startIP != BADADDR) inf.startIP += startEA;
//?
//    inf.ominEA = inf.minEA = startEA;
//    inf.omaxEA = inf.maxEA = endEA;
  }
  inf.af &= ~AF_FINAL;                    // behave as a binary file

  create_filename_cmt();
}

//--------------------------------------------------------------------------
static int set_s_type(ea_t addr)
{
  int  off = 0;
  lc.sz    = 4;
  lc.size += 3;
  if(addr >= 0x10000l) {
    ++off;
    lc.sz += 2;
    lc.sum += (uchar)(addr >> 16);
    ++lc.size;
    if(addr >= 0x01000000l) {
      ++off;
      lc.sz += 2;
      lc.sum += (uchar)(addr >> 24);
      ++lc.size;
    }
  }
  return(off);
}

//--------------------------------------------------------------------------
int write_file(FILE *fp, const char * /*fileformatname*/)
{
  static char fmt0[] = "%02X%0*lX%s%0?X\r\n",
              fmt1[] = "?00?00001FF\r\n",
              fone[] = "%02X";

  char   ident;
  ea_t   ea1, addr, strt, base = toEA(inf.baseaddr, 0);

  if(inf.minEA < base) base = BADADDR;

  if(fp == NULL) {
    if(inf.filetype == f_SREC) return(1);
    if((ea1 = inf.maxEA - inf.minEA) <= 0x10000ul) return(1);
    strt = 0;
    for(addr = inf.minEA; addr < inf.maxEA; ) {
      register segment_t *ps = getseg(addr);
      if(!ps || ps->type != SEG_IMEM) {
        if(isLoaded(addr)) break;
        if(base != BADADDR) {
          if(--ea1 <= 0x10000ul) return(1);
        } else ++strt;
        ++addr;
        continue;
      }
      if(strt) {
        if((ea1 -= strt) != 0x10000ul) return(1);
        strt = 0;
      }
      if((ea1 -= (ps->endEA - addr)) < 0x10000ul) return(1);
      ++ea1;
      addr = ps->endEA;
    }
    if(base == BADADDR) {
      register segment_t *ps = getseg(addr);
      if((ea1 -= ((ps == NULL) ? addr : ps->startEA)) <= 0x10000ul) return(1);
    }
    if(addr == inf.maxEA) return(0);
    for(base = inf.maxEA-1; base > addr; ) {
      register segment_t *ps = getseg(base);
      if(!ps || ps->type != SEG_IMEM) {
        if(isLoaded(base)) break;
        if(--ea1 <= 0x10000ul) return(1);
        --base;
        continue;
      }
      if((ea1 -= (base - ps->startEA)) < 0x10000ul) return(1);
      ++ea1;
      base = ps->startEA;
    }
    return(0);
  }

  fmt0[13] = '2';
  switch(inf.filetype) {
    case f_SREC:
      ident = 'S';
      break;
    case f_HEX:
      ident = ':';
      fmt1[3] = '0';
      break;
    default:
      ident = ';';
      fmt0[13] = '4';
      fmt1[3] = '\0';
      break;
  }
  fmt1[0] = ident;
  lc.sz = 4;

  strt = inf.startIP;
  for(ea1 = inf.minEA; ea1 < inf.maxEA; ) {
    char   str[(2 * MAX_BYTES) + 3], *p;
    char *const end = str + sizeof(str);
    if(!isLoaded(ea1) || segtype(ea1) == SEG_IMEM) {
      ++ea1;
      continue;
    }
    if(base == BADADDR) {
      register segment_t *ps = getseg(ea1);
      base = (ps == NULL) ? ea1 : ps->startEA;
      if(strt != BADADDR) strt += (inf.minEA - base);
    }
    addr  = ea1 - base;
    lc.sum = (uchar)addr + (uchar)(addr >> 8);
    p = str;
    if(inf.filetype == f_HEX)
    {
      *p++ = '0';
      *p++ = '0';
    }
    lc.size = 0;
    do
    {
      uchar b = get_byte(ea1++);
      p += qsnprintf(p, end-p, fone, (unsigned)b);
      lc.sum += b;
    } while(++lc.size < MAX_BYTES && ea1 < inf.maxEA &&
            isLoaded(ea1) && segtype(ea1) != SEG_IMEM);
    qfputc(ident, fp);
    if(inf.filetype == f_SREC) {
      char type = '1' + set_s_type(addr);
      qfputc(type, fp);
      ++lc.sum;  //correct to NOT
    } // else addr = (ushort)addr; // force check
    lc.sum += lc.size;
    if(inf.filetype != f_MEX) lc.sum = (uchar)(0-lc.sum);
    qfprintf(fp, fmt0, lc.size, lc.sz, addr, str, lc.sum);
  }
  if(inf.filetype != f_SREC) qfprintf(fp, fmt1);
  else if(strt != BADADDR) {
    qfputc(ident, fp);
    addr = strt;
    lc.sum  = 0;
    lc.size = 0;
    char type = '9' - set_s_type(addr);
    qfputc(type, fp);
    lc.sum = (~(lc.size + lc.sum)) & 0xFF;
    qfprintf(fp, fmt0, lc.size, lc.sz, addr, &fone[sizeof(fone)-1], lc.sum);
  }
  return(1);
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
//      create output file from the database.
//      this function may be absent.
//
  write_file,
};
