/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su, ig@datarescue.com
 *                              FIDO:   2:5020/209
 *
 */

#ifndef _LLONG_HPP
#define _LLONG_HPP
#pragma pack(push, 1)           // IDA uses 1 byte alignments!

//---------------------------------------------------------------------------
#if defined(__BORLANDC__)

#define __HAS_LONGLONG__
//#define __HAS_INT128__
typedef unsigned __int64 ulonglong;
typedef          __int64 longlong;

#elif defined(_MSC_VER)

#define __HAS_LONGLONG__
typedef unsigned __int64 ulonglong;
typedef          __int64 longlong;

#elif defined(__GNUC__)

#define __HAS_LONGLONG__
typedef unsigned long long ulonglong;
typedef          long long longlong;

#endif

//---------------------------------------------------------------------------
#ifdef __HAS_LONGLONG__

idaman char *ida_export print(ulong l,ulong h,char *buf,int bufsize,int radix,int issigned);

#ifdef __cplusplus
inline longlong make_longlong(ulong ll,long hh) { return ll | (longlong(hh) << 32); }
inline ulonglong make_ulonglong(ulong ll,long hh) { return ll | (ulonglong(hh) << 32); }
inline ulong low      (const ulonglong &x) { return ulong(x); }
inline ulong high     (const ulonglong &x) { return ulong(x>>32); }
inline ulong low      (const longlong &x) { return ulong(x); }
inline long  high     (const longlong &x) { return ulong(x>>32); }
// all print() functions: if bufsize == -1, then don't check the buffer size
inline char *print(longlong x,char *buf,int bufsize,int radix)
        { return print(ulong(x),ulong(x>>32),buf,bufsize,radix,true); }
inline char *print(ulonglong x,char *buf,int bufsize,int radix)
        { return print(ulong(x),ulong(x>>32),buf,bufsize,radix,false); }
#else
#define make_longlong(ll,hh)   (ll | (longlong(hh) << 32))
#define make_ulonglong(ll,hh)  (ll | (ulonglong(hh) << 32))
//#define low(x)   ulong(x)
//#define high(x)  ulong(x>>32)
#endif

#else

class longlong;
class ulonglong;

#define DECLARE_LLONG_HELPERS(decl) \
decl void     ida_export llong_div(const longlong &x,const longlong &y,longlong &r,longlong &q);\
decl char *   ida_export print(ulong l,ulong h,char *buf,int bufsize,int radix,int issigned);\
decl longlong ida_export llong_mul(const ulonglong &x,const ulonglong &y);\
decl void     ida_export llong_udiv(const ulonglong &x,const ulonglong &y,longlong &r,longlong &q);\
decl longlong ida_export shift_left(const ulonglong &x, int cnt,int issigned);\
decl longlong ida_export shift_right(const ulonglong &x, int cnt,int issigned);

DECLARE_LLONG_HELPERS(idaman)

class longlong
{
  private:
    ulong l;
    long h;
    DECLARE_LLONG_HELPERS(friend)
    friend class ulonglong;
  public:
    longlong(void) {}
    longlong(ulong x) { l=x; h=0; }
    longlong(long x) { l=x; h=(long(l)<0) ? -1 : 0; }
    longlong(uint x) { l=x; h=0; }
    longlong(int x) { l=x; h=(long(l)<0) ? -1 : 0; }
    friend longlong make_longlong(ulong ll,long hh) { longlong x; x.l=ll; x.h=hh; return x; }
    longlong(ulonglong x);
    friend ulong low (const longlong &x) { return x.l; }
    friend long  high(const longlong &x) { return x.h; }
    friend char *print(longlong x,char *buf,int bufsize,int radix)
        { return ::print(x.l,x.h,buf,bufsize,radix,1); }
    friend longlong operator+(const longlong &x, const longlong &y);
    friend longlong operator-(const longlong &x, const longlong &y);
    friend longlong operator/(const longlong &x, const longlong &y);
    friend longlong operator%(const longlong &x, const longlong &y);
    friend longlong operator*(const longlong &x, const longlong &y);
    friend longlong operator|(const longlong &x, const longlong &y);
    friend longlong operator&(const longlong &x, const longlong &y);
    friend longlong operator^(const longlong &x, const longlong &y);
    friend longlong operator>>(const longlong &x, int cnt);
    friend longlong operator<<(const longlong &x, int cnt);
    longlong &operator+=(const longlong &y);
    longlong &operator-=(const longlong &y);
    longlong &operator/=(const longlong &y);
    longlong &operator%=(const longlong &y);
    longlong &operator*=(const longlong &y);
    longlong &operator|=(const longlong &y);
    longlong &operator&=(const longlong &y);
    longlong &operator^=(const longlong &y);
    longlong &operator>>=(int cnt);
    longlong &operator<<=(int cnt);
    longlong &operator++(void);
    longlong &operator--(void);
    friend longlong operator+(const longlong &x) { return x; }
    friend longlong operator-(const longlong &x);
    friend longlong operator~(const longlong &x) { return make_longlong(~x.l,~x.h); }
    friend int operator==(const longlong &x, const longlong &y) { return x.l == y.l && x.h == y.h; }
    friend int operator!=(const longlong &x, const longlong &y) { return x.l != y.l || x.h != y.h; }
    friend int operator> (const longlong &x, const longlong &y) { return x.h > y.h || (x.h == y.h && x.l >  y.l); }
    friend int operator< (const longlong &x, const longlong &y) { return x.h < y.h || (x.h == y.h && x.l <  y.l); }
    friend int operator>=(const longlong &x, const longlong &y) { return x.h > y.h || (x.h == y.h && x.l >= y.l); }
    friend int operator<=(const longlong &x, const longlong &y) { return x.h < y.h || (x.h == y.h && x.l <= y.l); }
};

//---------------------------------------------------------------------------
class ulonglong
{
  private:
    DECLARE_LLONG_HELPERS(friend)
    ulong l;
    ulong h;
  public:
    ulonglong(void) {}
    ulonglong(ulong x) { l=x; h=0; }
    ulonglong(long x) { l=x; h=(long(l)<0) ? -1 : 0; }
    ulonglong(uint x) { l=x; h=0; }
    ulonglong(int x) { l=x; h=(long(l)<0) ? -1 : 0; }
    friend ulonglong make_ulonglong(ulong ll,long hh) { ulonglong x; x.l=ll; x.h=hh; return x; }
    ulonglong(longlong x) { l=x.l; h=x.h; }
    friend ulong low (const ulonglong &x) { return x.l; }
    friend ulong high(const ulonglong &x) { return x.h; }
    friend char *print(ulonglong x,char *buf,int bufsize,int radix)
        { return ::print(x.l,x.h,buf,bufsize,radix,0); }
    friend ulonglong operator+(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator-(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator/(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator%(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator*(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator|(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator&(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator^(const ulonglong &x, const ulonglong &y);
    friend ulonglong operator>>(const ulonglong &x, int cnt);
    friend ulonglong operator<<(const ulonglong &x, int cnt);
    ulonglong &operator+=(const ulonglong &y);
    ulonglong &operator-=(const ulonglong &y);
    ulonglong &operator/=(const ulonglong &y);
    ulonglong &operator%=(const ulonglong &y);
    ulonglong &operator*=(const ulonglong &y);
    ulonglong &operator|=(const ulonglong &y);
    ulonglong &operator&=(const ulonglong &y);
    ulonglong &operator^=(const ulonglong &y);
    ulonglong &operator>>=(int cnt);
    ulonglong &operator<<=(int cnt);
    ulonglong &operator++(void);
    ulonglong &operator--(void);
    friend ulonglong operator+(const ulonglong &x) { return x; }
    friend ulonglong operator-(const ulonglong &x);
    friend ulonglong operator~(const ulonglong &x) { return make_ulonglong(~x.l,~x.h); }
    friend int operator==(const ulonglong &x, const ulonglong &y) { return x.l == y.l && x.h == y.h; }
    friend int operator!=(const ulonglong &x, const ulonglong &y) { return x.l != y.l || x.h != y.h; }
    friend int operator> (const ulonglong &x, const ulonglong &y) { return x.h > y.h || (x.h == y.h && x.l >  y.l); }
    friend int operator< (const ulonglong &x, const ulonglong &y) { return x.h < y.h || (x.h == y.h && x.l <  y.l); }
    friend int operator>=(const ulonglong &x, const ulonglong &y) { return x.h > y.h || (x.h == y.h && x.l >= y.l); }
    friend int operator<=(const ulonglong &x, const ulonglong &y) { return x.h < y.h || (x.h == y.h && x.l <= y.l); }
};

//---------------------------------------------------------------------------
inline longlong::longlong(ulonglong x) { l=low(x); h=high(x); }

//---------------------------------------------------------------------------
inline longlong operator+(const longlong &x, const longlong &y) {
  long  h = x.h + y.h;
  ulong l = x.l + y.l;
  if ( l < x.l ) h++;
  return make_longlong(l,h);
}

//---------------------------------------------------------------------------
inline longlong operator-(const longlong &x, const longlong &y) {
  long  h = x.h - y.h;
  ulong l = x.l - y.l;
  if ( l > x.l ) h--;
  return make_longlong(l,h);
}

//---------------------------------------------------------------------------
inline longlong operator|(const longlong &x, const longlong &y) {
  return make_longlong(x.l | y.l,
                       x.h | y.h);
}

//---------------------------------------------------------------------------
inline longlong operator&(const longlong &x, const longlong &y) {
  return make_longlong(x.l & y.l,
                       x.h & y.h);
}

//---------------------------------------------------------------------------
inline longlong operator^(const longlong &x, const longlong &y) {
  return make_longlong(x.l ^ y.l,
                       x.h ^ y.h);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator+=(const longlong &y) {
  return (*this = *this + y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator-=(const longlong &y) {
  return (*this = *this - y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator|=(const longlong &y) {
  return (*this = *this | y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator&=(const longlong &y) {
  return (*this = *this & y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator^=(const longlong &y) {
  return (*this = *this ^ y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator/=(const longlong &y) {
  return (*this = *this / y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator%=(const longlong &y) {
  return (*this = *this % y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator*=(const longlong &y) {
  return (*this = *this * y);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator<<=(int cnt) {
  return (*this = *this << cnt);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator>>=(int cnt) {
  return (*this = *this >> cnt);
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator++(void) {
  if ( ++l == 0 ) ++h;
  return *this;
}

//---------------------------------------------------------------------------
inline longlong &longlong::operator--(void) {
  if ( l-- == 0 ) --h;
  return *this;
}

//---------------------------------------------------------------------------
inline longlong operator-(const longlong &x) {
  return ~x + 1;
}

//---------------------------------------------------------------------------
inline longlong operator/(const longlong &x, const longlong &y) {
  longlong remainder,quotient;
  llong_div(x,y,remainder,quotient);
  return quotient;
}

//---------------------------------------------------------------------------
inline longlong operator%(const longlong &x, const longlong &y) {
  longlong remainder,quotient;
  llong_div(x,y,remainder,quotient);
  return remainder;
}

//---------------------------------------------------------------------------
inline longlong operator*(const longlong &x, const longlong &y) {
  return llong_mul(ulonglong(x),ulonglong(y));
}

//---------------------------------------------------------------------------
inline longlong operator>>(const longlong &x, int cnt) {
  return shift_right(ulonglong(x),cnt,1);
}

//---------------------------------------------------------------------------
inline longlong operator<<(const longlong &x, int cnt) {
  return shift_left(ulonglong(x),cnt,1);
}

//---------------------------------------------------------------------------
inline ulonglong operator+(const ulonglong &x, const ulonglong &y) {
  long  h = x.h + y.h;
  ulong l = x.l + y.l;
  if ( l < x.l ) h++;
  return make_ulonglong(l,h);
}

//---------------------------------------------------------------------------
inline ulonglong operator-(const ulonglong &x, const ulonglong &y) {
  long  h = x.h - y.h;
  ulong l = x.l - y.l;
  if ( l > x.l ) h--;
  return make_ulonglong(l,h);
}

//---------------------------------------------------------------------------
inline ulonglong operator|(const ulonglong &x, const ulonglong &y) {
  return make_ulonglong(x.l | y.l,
                        x.h | y.h);
}

//---------------------------------------------------------------------------
inline ulonglong operator&(const ulonglong &x, const ulonglong &y) {
  return make_ulonglong(x.l & y.l,
                        x.h & y.h);
}

//---------------------------------------------------------------------------
inline ulonglong operator^(const ulonglong &x, const ulonglong &y) {
  return make_ulonglong(x.l ^ y.l,
                        x.h ^ y.h);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator+=(const ulonglong &y) {
  return (*this = *this + y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator-=(const ulonglong &y) {
  return (*this = *this - y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator|=(const ulonglong &y) {
  return (*this = *this | y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator&=(const ulonglong &y) {
  return (*this = *this & y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator^=(const ulonglong &y) {
  return (*this = *this ^ y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator/=(const ulonglong &y) {
  return (*this = *this / y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator%=(const ulonglong &y) {
  return (*this = *this % y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator*=(const ulonglong &y) {
  return (*this = *this * y);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator<<=(int cnt) {
  return (*this = *this << cnt);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator>>=(int cnt) {
  return (*this = *this >> cnt);
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator++(void) {
  if ( ++l == 0 ) ++h;
  return *this;
}

//---------------------------------------------------------------------------
inline ulonglong &ulonglong::operator--(void) {
  if ( l-- == 0 ) --h;
  return *this;
}

//---------------------------------------------------------------------------
inline ulonglong operator-(const ulonglong &x) {
  return ~x + 1;
}

//---------------------------------------------------------------------------
inline ulonglong operator/(const ulonglong &x, const ulonglong &y) {
  longlong remainder,quotient;
  llong_udiv(x,y,remainder,quotient);
  return quotient;
}

//---------------------------------------------------------------------------
inline ulonglong operator%(const ulonglong &x, const ulonglong &y) {
  longlong remainder,quotient;
  llong_udiv(x,y,remainder,quotient);
  return remainder;
}

//---------------------------------------------------------------------------
inline ulonglong operator*(const ulonglong &x, const ulonglong &y) {
  return llong_mul(x,y);
}

//---------------------------------------------------------------------------
inline ulonglong operator>>(const ulonglong &x, int cnt) {
  return shift_right(x,cnt,0);
}

//---------------------------------------------------------------------------
inline ulonglong operator<<(const ulonglong &x, int cnt) {
  return shift_left(x,cnt,0);
}

#endif // ifdef __HAS_LONGLONG__

idaman longlong ida_export llong_scan(const char *buf,int radix,const char **end);
idaman ulonglong ida_export swap64(ulonglong);
#ifdef __cplusplus
inline longlong swap64(longlong x) { return longlong(swap64(ulonglong(x))); }
#endif

//---------------------------------------------------------------------------
//      128 BIT NUMBERS
//---------------------------------------------------------------------------
#ifdef __HAS_INT128__

typedef unsigned __int128 uint128;
typedef          __int128 int128;

inline int128 make_int128(ulonglong ll,longlong hh) { return ll | (int128(hh) << 64); }
inline uint128 make_uint128(ulonglong ll,ulonglong hh) { return ll | (uint128(hh) << 64); }
inline ulonglong low      (const uint128 &x) { return ulonglong(x); }
inline ulonglong high     (const uint128 &x) { return ulonglong(x>>64); }
inline ulonglong low      (const int128 &x) { return ulonglong(x); }
inline longlong  high     (const int128 &x) { return ulonglong(x>>64); }

#else
#ifdef __cplusplus
class uint128
{
  ulonglong l;
  ulonglong h;
  friend class int128;
public:
  uint128(void)  {}
  uint128(uint x) { l=x; h=0; }
  uint128(int x)  { l=x; h=(x<0)?-1:0; }
  uint128(ulonglong x) { l=x; h=0; }
  uint128(longlong x)  { l=x; h=(x<0)?-1:0; }
  uint128(ulonglong ll, ulonglong hh) { l=ll; h=hh; }
  friend ulonglong low (const uint128 &x) { return x.l; }
  friend ulonglong high(const uint128 &x) { return x.h; }
  friend uint128 operator+(const uint128 &x, const uint128 &y);
  friend uint128 operator-(const uint128 &x, const uint128 &y);
  friend uint128 operator/(const uint128 &x, const uint128 &y);
  friend uint128 operator%(const uint128 &x, const uint128 &y);
  friend uint128 operator*(const uint128 &x, const uint128 &y);
  friend uint128 operator|(const uint128 &x, const uint128 &y);
  friend uint128 operator&(const uint128 &x, const uint128 &y);
  friend uint128 operator^(const uint128 &x, const uint128 &y);
  friend uint128 operator>>(const uint128 &x, int cnt);
  friend uint128 operator<<(const uint128 &x, int cnt);
  uint128 &operator+=(const uint128 &y);
  uint128 &operator-=(const uint128 &y);
  uint128 &operator/=(const uint128 &y);
  uint128 &operator%=(const uint128 &y);
  uint128 &operator*=(const uint128 &y);
  uint128 &operator|=(const uint128 &y);
  uint128 &operator&=(const uint128 &y);
  uint128 &operator^=(const uint128 &y);
  uint128 &operator>>=(int cnt);
  uint128 &operator<<=(int cnt);
  uint128 &operator++(void);
  uint128 &operator--(void);
  friend uint128 operator+(const uint128 &x) { return x; }
  friend uint128 operator-(const uint128 &x);
  friend uint128 operator~(const uint128 &x) { return uint128(~x.l,~x.h); }
  friend int operator==(const uint128 &x, const uint128 &y) { return x.l == y.l && x.h == y.h; }
  friend int operator!=(const uint128 &x, const uint128 &y) { return x.l != y.l || x.h != y.h; }
  friend int operator> (const uint128 &x, const uint128 &y) { return x.h > y.h || (x.h == y.h && x.l >  y.l); }
  friend int operator< (const uint128 &x, const uint128 &y) { return x.h < y.h || (x.h == y.h && x.l <  y.l); }
  friend int operator>=(const uint128 &x, const uint128 &y) { return x.h > y.h || (x.h == y.h && x.l >= y.l); }
  friend int operator<=(const uint128 &x, const uint128 &y) { return x.h < y.h || (x.h == y.h && x.l <= y.l); }
};

class int128
{
  ulonglong l;
   longlong h;
  friend class uint128;
public:
  int128(void)  {}
  int128(uint x) { l=x; h=0; }
  int128(int x)  { l=x; h=(x<0)?-1:0; }
  int128(ulonglong x) { l=x; h=0; }
  int128(longlong x)  { l=x; h=(x<0)?-1:0; }
  int128(ulonglong ll, ulonglong hh) { l=ll; h=hh; }
  int128(const uint128 &x) { l=x.l; h=x.h; }
  friend ulonglong low (const int128 &x) { return x.l; }
  friend ulonglong high(const int128 &x) { return x.h; }
  friend int128 operator+(const int128 &x, const int128 &y);
  friend int128 operator-(const int128 &x, const int128 &y);
  friend int128 operator/(const int128 &x, const int128 &y);
  friend int128 operator%(const int128 &x, const int128 &y);
  friend int128 operator*(const int128 &x, const int128 &y);
  friend int128 operator|(const int128 &x, const int128 &y);
  friend int128 operator&(const int128 &x, const int128 &y);
  friend int128 operator^(const int128 &x, const int128 &y);
  friend int128 operator>>(const int128 &x, int cnt);
  friend int128 operator<<(const int128 &x, int cnt);
  int128 &operator+=(const int128 &y);
  int128 &operator-=(const int128 &y);
  int128 &operator/=(const int128 &y);
  int128 &operator%=(const int128 &y);
  int128 &operator*=(const int128 &y);
  int128 &operator|=(const int128 &y);
  int128 &operator&=(const int128 &y);
  int128 &operator^=(const int128 &y);
  int128 &operator>>=(int cnt);
  int128 &operator<<=(int cnt);
  int128 &operator++(void);
  int128 &operator--(void);
  friend int128 operator+(const int128 &x) { return x; }
  friend int128 operator-(const int128 &x);
  friend int128 operator~(const int128 &x) { return int128(~x.l,~x.h); }
  friend int operator==(const int128 &x, const int128 &y) { return x.l == y.l && x.h == y.h; }
  friend int operator!=(const int128 &x, const int128 &y) { return x.l != y.l || x.h != y.h; }
  friend int operator> (const int128 &x, const int128 &y) { return x.h > y.h || (x.h == y.h && x.l >  y.l); }
  friend int operator< (const int128 &x, const int128 &y) { return x.h < y.h || (x.h == y.h && x.l <  y.l); }
  friend int operator>=(const int128 &x, const int128 &y) { return x.h > y.h || (x.h == y.h && x.l >= y.l); }
  friend int operator<=(const int128 &x, const int128 &y) { return x.h < y.h || (x.h == y.h && x.l <= y.l); }
};

idaman void ida_export swap128(uint128 *x);

//---------------------------------------------------------------------------
inline uint128 operator+(const uint128 &x, const uint128 &y) {
  ulonglong h = x.h + y.h;
  ulonglong l = x.l + y.l;
  if ( l < x.l )
    h = h + 1;
  return uint128(l,h);
}

//---------------------------------------------------------------------------
inline uint128 operator-(const uint128 &x, const uint128 &y) {
  ulonglong h = x.h - y.h;
  ulonglong l = x.l - y.l;
  if ( l > x.l )
    h = h - 1;
  return uint128(l,h);
}

//---------------------------------------------------------------------------
inline uint128 operator|(const uint128 &x, const uint128 &y) {
  return uint128(x.l | y.l,
                        x.h | y.h);
}

//---------------------------------------------------------------------------
inline uint128 operator&(const uint128 &x, const uint128 &y) {
  return uint128(x.l & y.l,
                        x.h & y.h);
}

//---------------------------------------------------------------------------
inline uint128 operator^(const uint128 &x, const uint128 &y) {
  return uint128(x.l ^ y.l,
                        x.h ^ y.h);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator+=(const uint128 &y) {
  return (*this = *this + y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator-=(const uint128 &y) {
  return (*this = *this - y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator|=(const uint128 &y) {
  return (*this = *this | y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator&=(const uint128 &y) {
  return (*this = *this & y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator^=(const uint128 &y) {
  return (*this = *this ^ y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator/=(const uint128 &y) {
  return (*this = *this / y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator%=(const uint128 &y) {
  return (*this = *this % y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator*=(const uint128 &y) {
  return (*this = *this * y);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator<<=(int cnt) {
  return (*this = *this << cnt);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator>>=(int cnt) {
  return (*this = *this >> cnt);
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator++(void) {
  if ( ++l == 0 ) ++h;
  return *this;
}

//---------------------------------------------------------------------------
inline uint128 &uint128::operator--(void) {
  if ( l == 0 ) --h;
  --l;
  return *this;
}

//---------------------------------------------------------------------------
inline uint128 operator-(const uint128 &x) {
  return ~x + 1;
}

#endif // ifdef __cplusplus
#endif // ifdef __HAS_INT128__

// stupid watcom compiler does not know about 64bit numbers
#ifdef __WATCOMC__
#define WC4(x)  low(x)
#else
#define WC4(x)  (x)
#endif

#pragma pack(pop)
#endif // define _LLONG_HPP
