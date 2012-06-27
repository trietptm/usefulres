#ifndef __W32RUN_H__
#define __W32RUN_H__

struct w32_hdr {
  ulong   ident;
  ulong   beg_fileoff;
  ulong   read_size;
  ulong   reltbl_offset;
  ulong   mem_size;
  ulong   start_offset;
};

#define W32_ID ('F'<<8)+'C'

#define W32_DOS_LOAD_BASE 0x10000ul    //normal dos loading

#endif
