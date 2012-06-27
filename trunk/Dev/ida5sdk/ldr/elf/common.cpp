/*
 *      Interactive disassembler (IDA) 
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *                        E-mail: ig@datarescue.com
 *      ELF-bynary loader.
 *      Copyright (c) 1995-2006 by Iouri Kharon.
 *                        E-mail: yjh@styx.cabel.net
 *
 *      ALL RIGHTS RESERVED.
 *
 */

bool unpatched;
bool elf64;
ushort e_type;

static bool mf;
static linput_t *li;
static char name[MAXSTR];
static ulong dynstr_off, dynstr_size;
static ulong dynsym_off, dynsym_size;
static ulong dynrel_off, dynrel_size;
static ulong dynrela_off, dynrela_size;
static uval_t e_shoff;
static ushort e_shnum, e_shstrndx;

#ifndef _LOADER_HPP
#define loader_failure() qexit(1)
#endif

#ifndef NO_ERRSTRUCT
//--------------------------------------------------------------------------
static void ask_for_exit(const char *str)
{
  if ( askyn_c(1, "HIDECANCEL\n%s. Continue?", str) <= 0 )
    loader_failure();
}

//--------------------------------------------------------------------------
static void _errstruct(int line)
{
  static bool asked = false;
  if ( !asked )
  {
    if ( askyn_c(1, "HIDECANCEL\n"
                    "Bad file structure or read error (line %d). Continue?", line)
                <= 0 ) loader_failure();
    asked = true;
  }
}

#define errstruct() _errstruct(__LINE__)
#endif

//--------------------------------------------------------------------------
static void errnomem(void) { nomem("ELF"); }

//--------------------------------------------------------------------------
// FILE-MEMORY mapping
struct mapping_t
{
  ulong offset;
  ulong size;
  uint64 ea;
};

static mapping_t *map = NULL;
static int nmap = 0;

static void add_mapping(ulong offset, ulong size, uint64 ea)
{
  map = (mapping_t *)qrealloc(map, (nmap+1)*sizeof(mapping_t));
  if ( map == NULL ) errnomem();
  map[nmap].offset = offset;
  map[nmap].size   = size;
  map[nmap].ea     = ea;
  nmap++;
}

static void clear_mappings(void)
{
  qfree(map);
  map = NULL;
  nmap = 0;
}

static ulong map_ea(uint64 ea)
{
  for ( int i=0; i < nmap; i++ )
  {
    if ( map[i].ea <= ea && map[i].ea+map[i].size > ea )
      return low(ea - map[i].ea) + map[i].offset;
  }
  char buf[64];
  warning("Couldn't map address %s", print(ea, buf, sizeof(buf), 16));
  return low(ea);
}

//--------------------------------------------------------------------------
//      Functions common for EFD & DEBUGGER
//--------------------------------------------------------------------------

static Elf32_Shdr   *shdr32;
static Elf64_Shdr   *shdr64;
static ulong        symcnt;

static ushort got_sec, plt_sec, str_sec, sym_sec, dst_sec, dsm_sec, int_sec,
              gpt_sec;

static ulong dyn_offset, dyn_size;
static int dyn_link;

static sym_rel *stb, *dstb;

static ulong  borext_offset, borext_size; // for Kylix (currently - efd only)

//--------------------------------------------------------------------------
bool is_elf_file(linput_t *li)
{
  Elf32_Ehdr h;
  qlseek(li, 0);
  if ( qlread(li, &h, sizeof(h)) != sizeof(h)
    || h.e_ident[EI_MAG0] != ELFMAG0
    || h.e_ident[EI_MAG1] != ELFMAG1
    || h.e_ident[EI_MAG2] != ELFMAG2
    || h.e_ident[EI_MAG3] != ELFMAG3 ) return false;
  return true;
}

//--------------------------------------------------------------------------
static void load_sht32(void)
{
  register int i;
  register Elf32_Shdr *sh;

  qlseek(li, ulong(e_shoff));
  for(i = 0, sh = shdr32; i < e_shnum; i++, sh++) {
    if(lread4bytes(li,          &sh->sh_name,      mf) ||
       lread4bytes(li,          &sh->sh_type,      mf) ||
       lread4bytes(li,          &sh->sh_flags,     mf) ||
       lread4bytes(li, (ulong *)&sh->sh_addr,      mf) ||
       lread4bytes(li,          &sh->sh_offset,    mf) ||
       lread4bytes(li,          &sh->sh_size,      mf) ||
       lread4bytes(li,          &sh->sh_link,      mf) ||
       lread4bytes(li,          &sh->sh_info,      mf) ||
       lread4bytes(li,          &sh->sh_addralign, mf) ||
       lread4bytes(li,          &sh->sh_entsize,   mf))  errstruct();
  }
}

//--------------------------------------------------------------------------
static void load_sht64(void)
{
  register int i;
  register Elf64_Shdr *sh;

  qlseek(li, ulong(e_shoff));
  for(i = 0, sh = shdr64; i < e_shnum; i++, sh++) {
    if(lread4bytes(li, &sh->sh_name,      mf) ||
       lread4bytes(li, &sh->sh_type,      mf) ||
       lread8bytes(li, &sh->sh_flags,     mf) ||
       lread8bytes(li, &sh->sh_addr,      mf) ||
       lread8bytes(li, &sh->sh_offset,    mf) ||
       lread8bytes(li, &sh->sh_size,      mf) ||
       lread4bytes(li, &sh->sh_link,      mf) ||
       lread4bytes(li, &sh->sh_info,      mf) ||
       lread8bytes(li, &sh->sh_addralign, mf) ||
       lread8bytes(li, &sh->sh_entsize,   mf))  errstruct();
  }
}

//--------------------------------------------------------------------------
static void read_dyninfo(ulong offset, ulong size)
{
  qlseek(li, offset);
  const int entsize = elf64 ? sizeof(Elf64_Dyn) : sizeof(Elf32_Dyn);
  for(int i = 1; i < size; i += entsize )
  {
    Elf64_Dyn dyn;
    if ( elf64 )
    {
      if(lread8bytes(li, &dyn.d_tag, mf) ||
         lread8bytes(li, &dyn.d_un, mf))  errstruct();
    }
    else
    {
      ulong tag, val;
      if(lread4bytes(li, &tag, mf) ||
         lread4bytes(li, &val, mf))  errstruct();
      dyn.d_tag = tag;
      dyn.d_un = val;
    }
    switch ( WC4(dyn.d_tag) )
    {
      case DT_STRTAB:
        dynstr_off = map_ea(dyn.d_un);
        break;
      case DT_SYMTAB:
        dynsym_off = map_ea(dyn.d_un);
        break;
      case DT_REL:
        dynrel_off = map_ea(dyn.d_un);
        break;
      case DT_RELA:
        dynrela_off = map_ea(dyn.d_un);
        break;
      case DT_STRSZ:
        dynstr_size = ulong(dyn.d_un);
        break;
      case DT_RELSZ:
        dynrel_size = ulong(dyn.d_un);
        break;
      case DT_RELASZ:
        dynrela_size = ulong(dyn.d_un);
        break;
    }
  }
  ulong off = dynstr_off;
  if ( dynrel_off  ) off = qmin(dynrel_off, off);
  if ( dynrela_off ) off = qmin(dynrela_off, off);
  dynsym_size = off - dynsym_off;
}

//--------------------------------------------------------------------------
static bool load_name2(ushort index, ulong offset)
{
  uint64 off, size;
  if ( index == ushort(-1) )
  {
    off = dynstr_off;
    size = dynstr_size;
  }
  else
  {
    off  = elf64 ? shdr64[index].sh_offset : shdr32[index].sh_offset;
    size = elf64 ? shdr64[index].sh_size   : shdr32[index].sh_size;
  }
  if ( offset >= size )
  {
    qsnprintf(name, sizeof(name), "bad offset %08lx", low(offset+off));
    return false;
  }
  ulong pos = qltell(li);
  offset = low(offset + off);
  qlseek(li, offset);

  register char *p;
  register int  i, j;
  bool ok = true;
  for(i = 0, p = name; i < sizeof(name)-1; i++, p++)
    if((j = qlgetc(li)) == EOF)
    {
      qstrncpy(p, "{truncated name}", sizeof(name)-(p-name));
      ok = false;
      break;
    }
    else if((*p = (char)j) == '\0') break;
  if(i == sizeof(name)-1)
  {
    qstrncpy(p-5, "...", 5);
    ok = false;
  }
  qlseek(li, pos);
  return ok;
}

//--------------------------------------------------------------------------
static const char *get_pht_type(int type)
{
  switch ( type )
  {
    case PT_NULL:     return "NULL";
    case PT_LOAD:     return "LOAD";
    case PT_DYNAMIC:  return "DYNAMIC";
    case PT_INTERP:   return "INTERP";
    case PT_NOTE:     return "NOTE";
    case PT_SHLIB:    return "SHLIB";
    case PT_PHDR:     return "PHDR";
    case PT_TLS:      return "TLS";

    case PT_GNU_EH_FRAME: return "EH_FRAME";
    case PT_GNU_STACK:    return "STACK";
    case PT_GNU_RELRO:    return "RO-AFTER";

    default:
#ifndef EFD_COMPILE
      return  "'?'";
#else
      {
        static char buf[10];
        qsnprintf(buf, sizeof(buf), "%08lX", type);
        return buf;
      }
#endif
  }
}

//--------------------------------------------------------------------------
template<class Elf32_Shdr>
static void get_section_name(Elf32_Shdr *sh)
{
    if(e_shstrndx) load_name2(e_shstrndx, sh->sh_name);
    else if(sh->sh_size != 0) name[0] = '\0'; //nonamed sections ?
}

//--------------------------------------------------------------------------
template<class Elf32_Shdr>
static void analyze_sht(Elf32_Shdr *sh)
{
  int i;

  for(i = 1, sh++; i < e_shnum; i++, sh++)
  {
    if(sh->sh_size==0 && (e_type != ET_REL ||  //skip zero segment's
       (sh->sh_type != SHT_PROGBITS && sh->sh_type != SHT_NOBITS))) continue;

    if ( sh->sh_size == 0 ) continue;
    get_section_name(sh);
    switch(sh->sh_type) {
      case SHT_STRTAB:
        if(!strcmp(name, ".strtab"))
          str_sec = i;
        else if(!strcmp(name, ".dynstr"))
          dst_sec = i;
        break;

      case SHT_DYNAMIC:
      case SHT_DYNSYM:
      case SHT_SYMTAB:
          switch(sh->sh_type)
          {
            case SHT_SYMTAB:
              sym_sec = i;
              symcnt += low(sh->sh_size);
              break;
            case SHT_DYNSYM:
              dsm_sec = i;
              symcnt += low(sh->sh_size);
              break;
            case SHT_DYNAMIC:
              dyn_offset = sh->sh_offset;
              dyn_size   = sh->sh_size;
              dyn_link   = sh->sh_link;
              break;
          }
          break;

      case SHT_PROGBITS:
        if(!strcmp(name, ".interp"))
        {
          int_sec = i;
          break;
        }
        if(!strcmp(name, ".got"))
        {
          got_sec = i;
          break;
        }
        if(!strcmp(name, ".got.plt"))
        {
          gpt_sec = i;
          break;
        }
        // no break
      case SHT_NOBITS:
        if(!strcmp(name, ".plt")) plt_sec = i;
        break;

      // For Kylix (currently - efd only)
      case SHT_LOPROC:
        if(!strcmp(name, "borland.coment")) {
          borext_offset = sh->sh_offset;
          borext_size   = sh->sh_size;
        }
        break;
    }
  }
  if(!gpt_sec) gpt_sec = got_sec; // unification for ABI 2
  else if(!got_sec) gpt_sec = 0;  // unsupported format
}

