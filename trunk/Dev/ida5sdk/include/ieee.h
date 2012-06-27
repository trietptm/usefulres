/*
 *      Interactive disassembler (IDA)
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *                        E-mail: ig@datarescue.com
 *      Floating Point Number Libary.
 *      Copyright (c) 1995-2006 by Iouri Kharon.
 *                        E-mail: yjh@styx.cabel.net
 *
 *      ALL RIGHTS RESERVED.
 *
 */

#ifndef _IEEE_H_
#define _IEEE_H_
#pragma pack(push, 1)

//-------------------------------------------------------------------
/* Number of 16 bit words in external x type format */
#define IEEE_NE 6

/* Number of 16 bit words in internal format */
#define IEEE_NI (IEEE_NE+3)

//==========================================================================
/* Array offset to exponent */
#define IEEE_E 1

/* Array offset to high guard word */
#define IEEE_M 2

/* The exponent of 1.0 */
#define IEEE_EXONE (0x3fff)

//===================================================================
typedef unsigned short eNE[IEEE_NE];
typedef unsigned short eNI[IEEE_NI];
//-------------------------------------------------------------------
idaman eNE ida_export_data ieee_ezero;
idaman eNE ida_export_data ieee_eone;
//-------------------------------------------------------------------
// procedure for idp module
void ecleaz(eNI x);
#define ecleaz(x) memset(x, 0, sizeof(eNI))  // clear eNI var

idaman void ida_export emovo(eNI a, eNE b);    // move eNI => eNE
idaman void ida_export emovi(eNE a, eNI b);    // move eNE => eNI
idaman int  ida_export eshift(eNI x, int sc);  // shift NI format up (+) or down
//
//  call for normalize to processor defined base.
//  arg: lost = 0, subflg = 0, exp = new exp,
// rndbase = # bits in significand [correct: 24, 53, 56, 64 (default)]
//
idaman int  ida_export emdnorm(eNI s, int lost, int subflg, long exp, int rndbase);
//
//  eNI format: 0 - sign (0/1)
//              1 - exponent (based of IEEE_EXONE)
//              2 - high word of mantisa (always zero after normalize.)
//          if exp = 0, value = 0
//-------------------------------------------------------------------
// all functions return 0 if complete normaly
//
// error codes for the realcvt() conversion function (load/store):
//     -1 - not supported format for current .idp
//     -2 - number too big (small) for store (mem NOT modified)
//     -3 - illegal real data for load (IEEE data not filled)
//
// error codes for other functions:
//    1 - overfloat / underfloat
//    2 - illegal data (asctoreal)
//    3 - divide by 0 (ediv)
//    4 - too big for long (eetol)
//
//-------------------------------------------------------------------
//
// load/store call format (for ph.realcvt)
//
// int realcvt(void *m, eNE e, unsigned short swt);
//  m -> pointer to data
//  e - internal IEEE format data
//  swt - operation:
//      000 - load trunc. float (DEC ^F)    2 bytes (m->e)
//      001 - load float                    4 bytes (m->e)
//      003 - load double                   8 bytes (m->e)
//      004 - load long double             10 bytes (m->e)
//      005 - load long double             12 bytes (m->e)
//      010 - store trunc. float (DEC ^F)   2 bytes (e->m)
//      011 - store float                   4 bytes (e->m)
//      013 - store double                  8 bytes (e->m)
//      014 - store long double            10 bytes (e->m)
//      015 - store long double            12 bytes (e->m)
//
// IDP module function prototypes -- should be implemented in idp
int idaapi realcvt(void *m, eNE e, unsigned short swt);
int l_realcvt(void *m, eNE e, unsigned short swt); // little endian
int b_realcvt(void *m, eNE e, unsigned short swt); // big endian

// Standard IEEE 754 floating point conversion function to use as ph.realcvt()
idaman int ida_export ieee_realcvt(void *m, eNE e, unsigned short swt);

//------------------------------------------------------------------
// IEEE to ascii string
// ndigits - number of digits after '.'
idaman void ida_export realtoasc(const eNE x, char *buf, size_t bufsize, short ndigs);
// ascii string to IEEE
idaman int ida_export asctoreal(const char **sss, eNE y);

// conversions to/from integers
idaman void ida_export eltoe(sval_t l, eNE e);                   // long to IEEE
idaman int ida_export eetol(eNE a, sval_t *l, bool roundflg);    // IEEE to long (+-0.5 if flg)
// exponent
idaman int ida_export eldexp(eNE a, long pwr2, eNE b);           // b = a*(2**pwr2)
// arifmetic operations
idaman int ida_export eadd(eNE a, eNE b, eNE c, int subflg);     // if(!subflg) c = a + b
                                                                 // else        c = a - b
idaman int ida_export emul(eNE a, eNE b, eNE c);                 // c = a * b
idaman int ida_export ediv(eNE a, eNE b, eNE c);                 // c = a / b
// predefined function
void eclear(eNE a);                            // x = 0
#define eclear(a) memset(a, 0, sizeof(eNE))
void emov(eNE a, eNE b);                       // b = a
#define emov(a, b) memcpy(b, a, sizeof(eNE))
void eabs(eNE x);                              // x = |x|
#define eabs(x) (x[IEEE_NE-1] &= 0x7fff)
#ifdef __cplusplus
inline void eneg(eNE x)                        // x = -x
{
  if(x[IEEE_NE-1]) x[IEEE_NE-1] += 0x8000;
}
#endif
//
// note: non standard answer is returned
int esign(eNE x);                              // x < 0 ?
#define esign(x) (x[IEEE_NE-1] & 0x8000)
// comparison
idaman int ida_export ecmp(eNE a, eNE b);      // 0  if a = b
                                               // 1  if a > b
                                               // -1 if a < b

#pragma pack(pop)
#endif
