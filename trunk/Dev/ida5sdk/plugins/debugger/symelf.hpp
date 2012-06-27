
// read symbols from an elf file

bool load_elf_symbols(linput_t *li,
                      void idaapi _add_symbol(ea_t, const char *name, void *ud),
                      void *_ud);
