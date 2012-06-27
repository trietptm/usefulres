/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-97 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 */

#ifndef __FPRO_H
#define __FPRO_H
#pragma pack(push, 1)

//
//      This file contains q.. counterparts of FILE* functions from Clib.
//      The only difference is that they set 'qerrno' variable too.
//      You should not use C standard I/O functions in your modules.
//      The reason: Borland keep FILE * information local to a DLL
//      so if you open a file in the plugin and pass the handle to the
//      kernel, the kernel will not be able to use it.
//


#include <stdio.h>

// If you really need to use the standard functions, define USE_STANDARD_FILE_FUNCTIONS
// In this case do not mix them with q... functions
#ifndef USE_STANDARD_FILE_FUNCTIONS
#undef stdin
#undef stdout
#undef stderr
#define stdin      dont_use_stdin
#define stdout     dont_use_stdout
#define stderr     dont_use_stderr
#define fopen      dont_use_fopen
#define fread      dont_use_fread
#define fwrite     dont_use_fwrite
#define ftell      dont_use_ftell
#define fseek      dont_use_fseek
#define fclose     dont_use_fclose
#define fflush     dont_use_fflush
#define fputc      dont_use_fputc
#define fgetc      dont_use_fgetc
#define fgets      dont_use_fgets
#define fputs      dont_use_fputs
#define vfprintf   dont_use_vfprintf
#define vfscanf    dont_use_vfscanf
#define fprintf    dont_use_fprintf
#define fprintf    dont_use_fprintf
#define fscanf     dont_use_fscanf
#endif

idaman FILE *ida_export qfopen(const char *file, const char *mode);
idaman int   ida_export qfread(FILE *fp, void *buf, size_t n);
idaman int   ida_export qfwrite(FILE *fp, const void *buf, size_t n);
idaman long  ida_export qftell(FILE *fp);
idaman int   ida_export qfseek(FILE *fp, long offset, int whence);                  /* 0-Ok */
idaman int   ida_export qfclose(FILE *fp);
idaman int   ida_export qflush(FILE *fp);         // flush stream and call dup/close(). 0 - ok
idaman FILE *ida_export qtmpfile(void);

// returns temporary file name
// (abs path, includes directory, uses TEMP/TMP vars)
idaman char *ida_export qtmpnam(char *buf, size_t bufsize);

idaman int ida_export qfputc(int chr, FILE *fp);
idaman int ida_export qfgetc(FILE *fp);
idaman char *ida_export qfgets(char *s, size_t len, FILE *fp);
idaman int ida_export qfputs(const char *s, FILE *fp);
idaman int ida_export qvfprintf(FILE *fp, const char *fmt, va_list va);
idaman int ida_export qvprintf(const char *fmt, va_list va);
idaman int ida_export qvfscanf(FILE *fp, const char *fmt, va_list va);
idaman char *ida_export qgets(char *line, size_t linesize);
int idaapi qgetchar(void);
#ifdef __cplusplus
inline int qfprintf(FILE *fp, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int code = qvfprintf(fp, fmt, va);
  va_end(va);
  return code;
}

inline int qprintf(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int code = qvprintf(fmt, va);
  va_end(va);
  return code;
}

// output to stderr, available only to command line utilities
int idaapi qveprintf(const char *fmt, va_list va);
inline int qeprintf(const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int code = qveprintf(fmt, va);
  va_end(va);
  return code;
}

inline int qfscanf(FILE *fp, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  int code = qvfscanf(fp, fmt, va);
  va_end(va);
  return code;
}
#endif

#if !defined(feof) || !defined(ferror)
// If feof() and ferror() are not macros, we can not use them
// Fortunately, for borland and vc they are macros, so there is no problem
// GCC defines them as functions: I have no idea whether they will work or not
// Anyway we remove the error directive from this file
// so the plugins can be compiled with gcc
//#error  feof or ferror are not macros!
#endif

/*==================================================*/
/* Add-ins for 2..32 byte read/writes.
        fp   - pointer to file
        res  - value read from file
        size - size of value in bytes (1..32)
        mostfirst - is MSB first? (0/1)
   All these functions return 0 - Ok */

idaman int ida_export freadbytes(FILE *fp,void *res,int size,int mostfirst);
idaman int ida_export fwritebytes(FILE *fp,const void *l,int size,int mostfirst);

#ifdef __cplusplus
#define DEF_FREADBYTES(read, write, type, size)                         \
        inline int read(FILE *fp, type *res, bool mostfirst)            \
                { return freadbytes(fp, res, size, mostfirst); }        \
        inline int write(FILE *fp, const type *res, bool mostfirst)     \
                { return fwritebytes(fp, res, size, mostfirst); }
DEF_FREADBYTES(fread2bytes, fwrite2bytes, short, 2)
DEF_FREADBYTES(fread2bytes, fwrite2bytes, ushort, 2)
DEF_FREADBYTES(fread4bytes, fwrite4bytes, long, 4)
DEF_FREADBYTES(fread4bytes, fwrite4bytes, ulong, 4)
DEF_FREADBYTES(fread8bytes, fwrite8bytes, longlong, 8)
DEF_FREADBYTES(fread8bytes, fwrite8bytes, ulonglong, 8)
#else
#define fread2bytes(fp,v,mf)  freadbytes(fp,v,2,mf)
#define fwrite2bytes(fp,v,mf) fwritebytes(fp,v,2,mf)
#define fread4bytes(fp,v,mf)  freadbytes(fp,v,4,mf)
#define fwrite4bytes(fp,v,mf) fwritebytes(fp,v,4,mf)
#define fread8bytes(fp,v,mf)  freadbytes(fp,v,8,mf)
#define fwrite8bytes(fp,v,mf) fwritebytes(fp,v,8,mf)
#endif

#pragma pack(pop)
#endif
