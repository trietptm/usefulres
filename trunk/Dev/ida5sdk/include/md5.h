#ifndef MD5_H
#define MD5_H
#pragma pack(push, 1)

#ifdef __alpha
typedef unsigned int uint32;
#else
typedef unsigned long uint32;
#endif

struct MD5Context {
  uint32 buf[4];
  uint32 bits[2];
  unsigned char in[64];
};

idaman void ida_export MD5Init(struct MD5Context *context);
idaman void ida_export MD5Update(struct MD5Context *context,
                                 unsigned char const *buf,
                                 size_t len);
idaman void ida_export MD5Final(unsigned char digest[16],
                                struct MD5Context *context);
idaman void ida_export MD5Transform(uint32 buf[4], uint32 const in[16]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#pragma pack(pop)
#endif /* !MD5_H */
