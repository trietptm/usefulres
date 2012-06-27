#ifndef __NLM_H__
#define __NLM_H__
#pragma pack(push, 1)   // IDA uses 1 byte alignments!

struct nlmexe_t
{
#define NLM_MAGIC_SIZE 0x18
  char    magic[NLM_MAGIC_SIZE];
#define NLM_MAGIC "NetWare Loadable Module\x1A"
  ulong   version;      // file Version
#define NLM_COMPRESSED  0x00000080L     // compressed NLM file
  char    fnamelen;     // modulename length
  char    fname[12+1];
  ulong   codeoff;      // offset to code segment
  ulong   codelen;      // length of code segment
  ulong   dataoff;      // offset to data segment
  ulong   datalen;      // length of data segment
  ulong   bssSize;      // Unitialized data size
  ulong   custoff;      // help off
  ulong   custlen;      // help length
  ulong   autoliboff;   // autoload library offset
  ulong   autolibnum;   // number of autoload libraries
  ulong   fixupoff;     // offset to fixups
  ulong   fixupnum;     // number of fixups
  ulong   impoff;       // offset to imported names
  ulong   impnum;       // number of imported names
  ulong   expoff;       // offset to exported names
  ulong   expnum;       // number of exported names
  ulong   puboff;       // offset to public names
  ulong   pubnum;       // number of public names
  ulong   startIP;      // entry point?
  ulong   endIP;        // terminate NLM
  ulong   auxIP;        // additional entry point
  ulong   modType;      // Module type
  ulong   flags;        // Module flags
};

#define NLM_MODNAMOFF 0x82  //sizeof

#ifdef __BORLANDC__
#if NLM_MODNAMOFF != sizeof(nlmexe_t)
#error
#endif
#endif

#pragma pack(pop)
#endif
