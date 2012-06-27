
// read elf symbols

#include <pro.h>
#include <fpro.h>
#include <kernwin.hpp>
#include <diskio.hpp>
#include "../../ldr/elf/elfbase.h"
#include "../../ldr/elf/elf.h"

static ushort e_machine;

#define NO_ERRSTRUCT
#define errstruct() warning("bad input file structure")
#define nomem       error

#include "../../ldr/elf/common.cpp"

inline ulong low(ulong x) { return x; }

//--------------------------------------------------------------------------
static void (idaapi *add_symbol)(ea_t, const char *name, void *ud);
static void *ud;

//--------------------------------------------------------------------------
static void handle_symbol(int shndx, int info, ulong st_name, uval_t st_value, int namsec)
{
  if ( shndx == SHN_UNDEF
    || shndx == SHN_LOPROC
    || shndx == SHN_HIPROC
    || shndx == SHN_ABS ) return;

  int type = ELF32_ST_TYPE(info);
  if ( type != STT_OBJECT && type != STT_FUNC ) return;

  int bind = ELF32_ST_BIND(info);
  if ( bind != STB_GLOBAL ) return;  // what about local and weak names?

  if ( st_name )
  {
    load_name2(namsec, st_name);
    add_symbol(st_value, name, ud);
  }
}

//--------------------------------------------------------------------------
static bool load_symbols32(ulong pos, ulong size, int namsec)
{
  qlseek(li, pos + sizeof(Elf32_Sym)); // skip _UNDEF
  ulong cnt = size / sizeof(Elf32_Sym);
  for ( int i = 1; i < cnt; i++ )
  {
    Elf32_Sym st;
    if(lread4bytes(li,          &st.st_name,  mf) ||
       lread4bytes(li, (ulong *)&st.st_value, mf) ||
       lread4bytes(li,          &st.st_size,  mf) ||
       qlread(li, &st.st_info,  2) != 2           ||
       lread2bytes(li,          &st.st_shndx, mf))     return false;

    handle_symbol(st.st_shndx, st.st_info, st.st_name, st.st_value, namsec);
  }
  return true;
}

static bool load_symbols32(ushort symsec, int namsec)
{
  Elf32_Shdr *sh = shdr32+symsec;
  return load_symbols32(sh->sh_offset, sh->sh_size, namsec);
}

//--------------------------------------------------------------------------
static bool load_symbols64(ulong pos, ulong size, int namsec)
{
  qlseek(li, pos + sizeof(Elf64_Sym)); // skip _UNDEF
  ulong cnt = size / sizeof(Elf64_Sym);
  for ( int i = 1; i < cnt; i++ )
  {
    Elf64_Sym st;
    if(lread4bytes(li, &st.st_name,  mf) ||
       qlread(li, &st.st_info,  2) != 2  ||
       lread2bytes(li, &st.st_shndx, mf) ||
       lread8bytes(li, &st.st_value, mf) ||
       lread8bytes(li, &st.st_size,  mf) ) return false;

    handle_symbol(st.st_shndx, st.st_info, st.st_name, st.st_value, namsec);
  }
  return true;
}

static bool load_symbols64(ushort symsec,ushort namsec)
{
  Elf64_Shdr *sh = shdr64+symsec;
  return load_symbols64(low(sh->sh_offset), sh->sh_size, namsec);
}

//--------------------------------------------------------------------------
static bool map_pht32(linput_t *li, ulong e_phoff, int e_phnum)
{
  qlseek(li, e_phoff);
  for ( int i=0; i < e_phnum; i++ )
  {
    Elf32_Phdr p;
    if(lread4bytes(li, &p.p_type, mf) ||
       lread4bytes(li, &p.p_offset, mf) ||
       lread4bytes(li, &p.p_vaddr, mf) ||
       lread4bytes(li, &p.p_paddr, mf) ||
       lread4bytes(li, &p.p_filesz, mf) ||
       lread4bytes(li, &p.p_memsz, mf) ||
       lread4bytes(li, &p.p_flags, mf) ||
       lread4bytes(li, &p.p_align, mf))       return false;
    add_mapping(p.p_offset, p.p_filesz, p.p_vaddr);
    if ( p.p_type == PT_DYNAMIC )
    {
      dyn_offset = p.p_offset;
      dyn_size   = p.p_filesz;
      dyn_link   = -1;
    }
  }
  return true;
}

//--------------------------------------------------------------------------
static bool map_pht64(linput_t *li, ulong e_phoff, int e_phnum)
{
  qlseek(li, e_phoff);
  for ( int i=0; i < e_phnum; i++ )
  {
    Elf64_Phdr p;
    if(lread4bytes(li, &p.p_type, mf) ||
       lread4bytes(li, &p.p_flags, mf) ||
       lread8bytes(li, &p.p_offset, mf) ||
       lread8bytes(li, &p.p_vaddr, mf) ||
       lread8bytes(li, &p.p_paddr, mf) ||
       lread8bytes(li, &p.p_filesz, mf) ||
       lread8bytes(li, &p.p_memsz, mf) ||
       lread8bytes(li, &p.p_align, mf))       return false;
    add_mapping(p.p_offset, p.p_filesz, p.p_vaddr);
    if ( p.p_type == PT_DYNAMIC )
    {
      dyn_offset = p.p_offset;
      dyn_size   = p.p_filesz;
      dyn_link   = -1;
    }
  }
  return true;
}


//--------------------------------------------------------------------------
bool load_elf_symbols(linput_t *_li,
                      void idaapi _add_symbol(ea_t, const char *name, void *ud),
                      void *_ud)
{
  shdr32 = NULL;
  shdr64 = NULL;
  sym_sec = 0;
  dsm_sec = 0;
  dyn_size = 0;
  dynsym_size = 0;
  add_symbol = _add_symbol;
  ud = _ud;

  li = _li;
  if ( !is_elf_file(li) )
    return false;

  unsigned char e_ident[EI_NIDENT];
  Elf32_Off     e_phoff;
  Elf32_Half    e_phnum,
                e_ehsize, e_phentsize, e_shentsize;
  Elf32_Word    e_flags;
  uint64        e_entry;

  qlseek(li, 0);
  qlread(li, &e_ident, sizeof(e_ident));
  if ( e_ident[EI_CLASS] != ELFCLASS32 && e_ident[EI_CLASS] != ELFCLASS64 )
  {
//    warning("Unknown elf class %d (should be %d for 32-bit, %d for 64-bit)",
//      e_ident[EI_CLASS], ELFCLASS32, ELFCLASS64);
    return false;
  }
  if ( e_ident[EI_DATA] != ELFDATA2LSB && e_ident[EI_DATA] != ELFDATA2MSB )
  {
//    warning("Unknown elf byte sex %d (should be %d for LSB, %d for MSB)",
//      e_ident[EI_DATA], ELFDATA2LSB, ELFDATA2MSB);
    return false;
  }

  elf64 = e_ident[EI_CLASS] == ELFCLASS64;
  mf    = e_ident[EI_DATA]  == ELFDATA2MSB;

  {
    Elf32_Word e_version;
    if(lread2bytes(li, &e_type, mf) ||
       lread2bytes(li, &e_machine, mf) ||
       lread4bytes(li, &e_version, mf))
         return false;

//    if ( e_type != ET_EXEC && e_type != ET_DYN )
//      return false;

//    if ( e_machine != EM_386 )
//      return false;

  }

  if ( elf64 )
  {
    uint64 e2, e3;
    if(lread8bytes(li, &e_entry, mf) ||
       lread8bytes(li, &e2, mf)        ||
       lread8bytes(li, &e3, mf) ) return false;
    if ( low(e2) != e2 ) return false;
    if ( low(e3) != e3 ) return false;
    e_phoff = low(e2);
    e_shoff = low(e3);
  }
  else
  {
    ulong e3, e_entry32;
    if(lread4bytes(li, &e_entry32, mf) ||
       lread4bytes(li, &e_phoff, mf)   ||
       lread4bytes(li, &e3, mf) ) return false;
    e_shoff = e3;
    e_entry = e_entry32;
  }
  if(lread4bytes(li, &e_flags,     mf) ||
     lread2bytes(li, &e_ehsize,    mf) ||
     lread2bytes(li, &e_phentsize, mf) ||
     lread2bytes(li, &e_phnum,     mf) ||
     lread2bytes(li, &e_shentsize, mf) ||
     lread2bytes(li, &e_shnum,     mf) ||
     lread2bytes(li, &e_shstrndx,  mf) ) return false;

  // Sanitize SHT parameters
  const size_t sht_sizeof = elf64 ? sizeof(Elf64_Shdr) : sizeof(Elf32_Shdr);
  if ( e_shnum && e_shentsize != sht_sizeof )
  {
//    warning("SHT entry size is invalid (should be %d)\n", sht_sizeof);
    e_shentsize = sht_sizeof;
  }
  if ( (e_shnum == 0) != (e_shoff == 0)
    || e_shoff + e_shnum*sht_sizeof > qlsize(li) )
  {
//    warning("SHT size or offset is invalid");
    e_shnum = 0;
  }

  bool ok = false;
  if ( e_phnum )
  {
    ok = elf64 ? map_pht64(li, e_phoff, e_phnum)
               : map_pht32(li, e_phoff, e_phnum);
    if ( !ok ) goto ret;
  }

  if ( e_shnum && e_shentsize )
  {
    if ( elf64 )
    {
      shdr64 = (Elf64_Shdr *)qalloc(e_shnum * sizeof(Elf64_Shdr));
      if ( shdr64 == NULL ) goto ret;
      load_sht64();
      analyze_sht(shdr64);
    }
    else
    {
      shdr32 = (Elf32_Shdr *)qalloc(e_shnum * sizeof(Elf32_Shdr));
      if ( shdr32 == NULL ) goto ret;
      load_sht32();
      analyze_sht(shdr32);
    }
  }
  if ( dyn_size )
    read_dyninfo(dyn_offset, dyn_size);

  if ( sym_sec || dsm_sec )
  {
    // Loading symbols
    if ( sym_sec )
      if ( elf64 )
        ok = load_symbols64(sym_sec, str_sec);
      else
        ok = load_symbols32(sym_sec, str_sec);

    if ( ok && dsm_sec )
      if ( elf64 )
        ok = load_symbols64(dsm_sec, dst_sec);
      else
        ok = load_symbols32(dsm_sec, dst_sec);
  }
  else if ( dynsym_size )
  {
    if ( elf64 )
      ok = load_symbols64(dynsym_off, dynsym_size, -1);
    else
      ok = load_symbols32(dynsym_off, dynsym_size, -1);
  }

ret:
  qfree(shdr64);
  qfree(shdr32);
  clear_mappings();
  return ok;
}
