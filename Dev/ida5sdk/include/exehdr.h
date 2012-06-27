/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com, ig@estar.msk.su
 *
 *              EXE-file header layout
 *
 */

#ifndef __EXEHDR_H
#define __EXEHDR_H
#pragma pack(push, 1)

struct exehdr {
    unsigned short exe_ident;
#define EXE_ID  0x5A4D          // 'MZ'
#define EXE_ID2 0x4D5A          // 'ZM' (DOS works with this also)
    unsigned short PartPag;
    unsigned short PageCnt;
    unsigned short ReloCnt;
    unsigned short HdrSize;
    unsigned short MinMem ;
    unsigned short MaxMem ;
    unsigned short ReloSS ;
    unsigned short ExeSP  ;
    unsigned short ChkSum ;
    unsigned short ExeIP  ;
    unsigned short ReloCS ;
    unsigned short TablOff;
    unsigned short Overlay;
        long CalcEXE_Length(void) {
          long len = ( (PageCnt*512L) - (HdrSize*16) );
          if ( PartPag != 0 ) len -= (512 - PartPag);
          return len;
        }
        void CalcEXE_Pages(long len) {
          PartPag = (unsigned short)(len % 512);
          PageCnt = (unsigned short)(len / 512);
          if ( PartPag != 0 ) PageCnt++;
        }
};

#define PSPsize 0x100

#pragma pack(pop)
#endif
