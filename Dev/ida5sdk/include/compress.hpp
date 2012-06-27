
#ifndef COMPRESS_HPP
#define COMPRESS_HPP
#pragma pack(push, 1)

// compress data
idaman int ida_export zip_deflate(
  void *ud,
  ssize_t (idaapi *file_reader)(void *ud, void *buf, size_t size),
  ssize_t (idaapi *file_writer)(void *ud, const void *buf, size_t size));

// uncompress data
idaman int ida_export zip_inflate(
  void *ud,
  ssize_t (idaapi *file_reader)(void *ud, void *buf, size_t size),
  ssize_t (idaapi *file_writer)(void *ud, const void *buf, size_t size));

// Process zip file and enumerate all files stored in it
/* returns PK-type error code */
idaman int ida_export process_zipfile(const char *zipfile,      // name of zip file
                    int (idaapi *_callback)(                    // callback for each file
                                     void *ud,                  // user data
                                     long offset,               // offset in the zip file
                                     int method,                // compression method
                                     unsigned long csize,       // compressed size
                                     unsigned long ucsize,      // uncompressed size
                                     unsigned long attributes,
                                     const char *filename),
                    void *ud);                                  // user data

// error codes
#define PKZ_OK            0
#define PKZ_ERRNO         1
#define PKZ_STREAM_ERROR  2
#define PKZ_DATA_ERROR    3
#define PKZ_MEM_ERROR     4
#define PKZ_BUF_ERROR     5
#define PKZ_VERSION_ERROR 6
#define PKZ_RERR          777   // read error
#define PKZ_WERR          778   // write error

#define STORED            0    /* compression methods */
#define SHRUNK            1
#define REDUCED1          2
#define REDUCED2          3
#define REDUCED3          4
#define REDUCED4          5
#define IMPLODED          6
#define TOKENIZED         7
#define DEFLATED          8
#define NUM_METHODS       9    /* index of last method + 1 */

extern bool legacy_idb;         // for old idb files

#pragma pack(pop)
#endif
