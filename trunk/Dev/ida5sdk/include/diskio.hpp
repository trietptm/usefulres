/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@datarescue.com, ig@estar.msk.su
 *
 *
 */

#ifndef _DISKIO_HPP
#define _DISKIO_HPP
#pragma pack(push, 1)

//
//      This file contains file I/O functions for IDA.
//      You should not use standard C file I/O functions in modules.
//      Use functions from this header, pro.h and fpro.h instead.
//
//      Also this file declares a call_system() function.
//

#include <stdio.h>
#include <help.h>
#include <llong.hpp>

//-------------------------------------------------------------------------
//      S E A R C H   F O R   F I L E S
//-------------------------------------------------------------------------


// Get IDA directory (if subdir==NULL)
// or the specified subdirectory

idaman const char *ida_export idadir(const char *subdir);


// Search for IDA system file
// This function searches for a file in
//  1. get_user_idadir() subdirectory
//  2. ida (sub)directory
//  3. current directory
//      filename - name of file to search
// returns: NULL-not found, otherwise - pointer to full file name
// if the subdir is specified, the file is looked in the specified subdirectory
// of the ida directory first


idaman char *ida_export getsysfile(char *buf,
                                   size_t bufsize,
                                   const char *filename,
                                   const char *subdir);

#define CFG_SUBDIR "cfg"
#define IDC_SUBDIR "idc"
#define IDS_SUBDIR "ids"
#define IDP_SUBDIR "procs"
#define LDR_SUBDIR "loaders"
#define SIG_SUBDIR "sig"
#define TIL_SUBDIR "til"


// Get user ida related directory
// Under Linux: $HOME/.ida
// Under Windows: Application Data\Datarescue\IDA Pro
// If the directory did not exist, it will be created

idaman const char *ida_export get_user_idadir(void);


// enumerate files in the specified directory
// while func() returns 0
//      answer  - buffer to contain the file name for which func()!=0
//                (answer may be == NULL)
//      answer_size - size of 'answer'
//      path    - directory to enumerate files in
//      fname   - mask of file names to enumerate
//      func    - callback function called for each file
//                      file - full file name (with path)
//                      ud   - user data
//                if 'func' returns non-zero value, the enumeration
//                is stopped and the return code is
//                is returned to the caller.
//      ud      - user data. this pointer will be passed to
//                the callback function
// returns zero or the code returned by func()

idaman int ida_export enumerate_files(
                             char *answer,
                             size_t answer_size,
                             const char *path,
                             const char *fname,
                             int (idaapi*func)(const char *file,void *ud),
                             void *ud);


// enumerate IDA system files
// while func() returns 0
//      answer  - buffer to contain the file name for which func()!=0
//                (answer may be == NULL)
//      answer_size - size of 'answer'
//      subdir  - IDA subdirectory or NULL
//      fname   - mask of file names to enumerate
//      func    - callback function called for each file
//                      file - full file name (with path)
//                      ud   - user data
//                if 'func' returns non-zero value, the enumeration
//                is stopped and full path of the current file
//                is returned to the caller.
//      ud      - user data. this pointer will be passed to
//                the callback function
// returns zero or the code returned by func()

inline int idaapi enumerate_system_files(
                             char *answer,
                             size_t answer_size,
                             const char *subdir,
                             const char *fname,
                             int (idaapi*func)(const char *file,void *ud),
                             void *ud)
{
  return enumerate_files(answer, answer_size, idadir(subdir), fname, func, ud);
}

//-------------------------------------------------------------------------
//      O P E N / R E A D / W R I T E / C L O S E   F I L E S
//-------------------------------------------------------------------------

//      There are two sets of "open file" functions.
//      The first set tries to open a file and returns: success or failure
//      The second set is "open or die": if the file cannot be opened
//      then the function will display an error message and exit.


// Open a new file for write in text mode, deny write
// If a file exists, it will be removed.
// returns: NULL-failure

idaman FILE *ida_export fopenWT(const char *file);


// Open a new file for write in binary mode, deny read/write
// If a file exists, it will be removed.
// returns: NULL-failure

idaman FILE *ida_export fopenWB(const char *file);


// Open a file for read in text mode, deny write
// returns: NULL-failure

idaman FILE *ida_export fopenRT(const char *file);


// Open a file for read in binary mode, deny write
// returns: NULL-failure

idaman FILE *ida_export fopenRB(const char *file);


// Open a file for read/write in binary mode, deny write
// returns: NULL-failure

idaman FILE *ida_export fopenM(const char *file);


// Open a file for append in text mode, deny none
// returns: NULL-failure

idaman FILE *ida_export fopenA(const char *file);


// Open a file for read in binary mode or die, deny write
// If a file cannot be opened, this function displays a message and exits.

idaman FILE *ida_export openR(const char *file);


// Open a file for read in text mode or die, deny write
// If a file cannot be opened, this function displays a message and exits.

idaman FILE *ida_export openRT(const char *file);


// Open a file for read/write in binary mode or die, deny read/write
// If a file cannot be opened, this function displays a message and exits.

idaman FILE *ida_export openM(const char *file);


// Open a new file for write in binary mode or die, deny read/write
// If a file exists, it will be removed.
// If a file cannot be opened, this function displays a message and exits.

idaman FILE *ida_export ecreate(const char *file);


// Open a new file for write in text mode or die, deny read/write
// If a file exists, it will be removed.
// If a file cannot be opened, this function displays a message and exits.

idaman FILE *ida_export ecreateT(const char *file);


// Close a file or die.
// If a file cannot be closed, this function displays a message and exits.

idaman void ida_export eclose(FILE *fp);


// Read from file or die.
//      fp   - pointer to file
//      buf  - buffer to read in
//      size - number of bytes to read
// If a read error occurs, this function displays a message and exits.

idaman void ida_export eread(FILE *fp, void *buf, ssize_t size);


// Write to file or die.
//      fp   - pointer to file
//      buf  - buffer to write
//      size - number of bytes to write
// If a write error occurs, this function displays a message and exits.

idaman void ida_export ewrite(FILE *fp, const void *buf, ssize_t size);


// Position in the file or die.
//      fp   - pointer to file
//      pos  - absolute position in the file
// If an error occurs, this function displays a message and exits.

idaman void ida_export eseek(FILE *fp,long pos);


//-------------------------------------------------------------------------
//      F I L E   S I Z E   /   D I S K   S P A C E
//-------------------------------------------------------------------------

// Get length of file in bytes
//      fp   - pointer to file

idaman ulong ida_export efilelength(FILE *fp);


// Change size of file or die.
//      fp   - pointer to file
//      size - new size of file
// If the file is expanded, it is expanded with zero bytes.
// If an error occurs, this function displays a message and exits.

idaman void ida_export echsize(FILE *fp,ulong size);


// Get free disk space in bytes
//      path - name of any directory on the disk to get information about

idaman ulonglong ida_export getdspace(const char *path);


//-------------------------------------------------------------------------
//      I / O  P O R T  D E F I N I T I O N S  F I L E
//-------------------------------------------------------------------------
struct ioport_bit_t
{
  char *name;           // name of the bit (attention: may be NULL!)
  char *cmt;            // comment
};

typedef ioport_bit_t ioport_bits_t[16];

struct ioport_t
{
  ea_t address;         // address of the port
  char *name;           // name of the port
  char *cmt;            // comment
  ioport_bits_t *bits;  // bit names
  void *userdata;       // arbitrary data. initialized by NULL.
};

// read i/o port definitions from a config file
//      _numports - place to put the number of ports
//      file      - config file name
//      default_device - contains device name to load. If default_device[0] == 0
//                  then the default device is determined by .default directive
//                  in the config file
//      dsize     - sizeof(default_device)
//      callback  - callback to call when the input line can't be parsed normally
//                    line - input line to parse
//                  returns error message. if NULL, then the line is parsed ok.
// returns: array of ioports
// The format of the input file:
// ; each device definition begins with a line like this:
// ;
// ;       .devicename
// ;
// ;  after it go the port definitions in this format:
// ;
// ;       portname        address
// ;
// ;  the bit definitions (optional) are represented like this:
// ;
// ;       portname.bitname  bitnumber
// ;
// ; lines beginning with a space are ignored.
// ; comment lines should be started with ';' character.
// ;
// ; the default device is specified at the start of the file
// ;
// ;       .default device_name
// ;
// ; all lines non conforming to the format are passed to the callback function
// It is allowed to have a symbol mapped to several addresses
// but all addresses must be unique

idaman ioport_t *ida_export read_ioports(size_t *_numports,
                       const char *file,
                       char *default_device,
                       size_t dsize,
                       const char *(idaapi* callback)(const ioport_t *ports, size_t numports, const char *line));

// Allow the user to choose the ioport device
//      file      - config file name
//      device    - in: contains default device name. If default_device[0] == 0
//                  then the default device is determined by .default directive
//                  in the config file
//                  out: the selected device name
//      device_size - size of the 'device' buffer
//      parse_params - if present (non NULL), then defines a callback which
//                  will be called for all lines not starting with a dot (.)
//                  This callback may parse these lines are prepare simple
//                  processor parameter string. This string will be displayed
//                  along with the device name.
// returns: true  - the user selected a device, its name is in 'device'
//          false - the selection was cancelled. if device=="NONE" upon return,
//                  then no devices were found in the configuration file

idaman bool ida_export choose_ioport_device(const char *file,
                              char *device,
                              size_t device_size,
                              const char *(idaapi* parse_params)(const char *line,
                                                          char *buf,
                                                          size_t bufsize));


// Find ioport in the array of ioports
idaman const ioport_t *ida_export find_ioport(const ioport_t *ports, size_t numports, ea_t address);


// Find ioport bit in the array of ioports
idaman const ioport_bit_t *ida_export find_ioport_bit(const ioport_t *ports, size_t numports, ea_t address, size_t bit);


// Free ioports array. The 'userdata' field is not examined!
idaman void ida_export free_ioports(ioport_t *ports, size_t numports);


//-------------------------------------------------------------------------
//      S Y S T E M  S P E C I F I C  C A L L S
//-------------------------------------------------------------------------

// Execute a operating system command
// This function suspends the interface (Tvision), runs the command
// and redraw the screen.
//      command - command to execute. If NULL, a shell is activated
// Returns: the error code returned by system() call.

idaman int ida_export call_system(const char *command);


// Work with priorities

ulong get_thread_priority(void);        // 0 - something wrong (can't happen
                                        // under OS/2)
idaman int   ida_export set_thread_priority(ushort pclass,long delta);


//-------------------------------------------------------------------------
//       L O A D E R  I N P U T  S O U R C E  F U N C T I O N S
//-------------------------------------------------------------------------

// Starting with v4.8 IDA can load and run remote files.
// In order to do that, we replace the FILE* in the loader modules
// with an abstract input source. The source might be linked to
// a local or remote file

class linput_t;         // loader input source


// The following functions may be used to work with the input source:

// Read the input source
// If failed, inform the user and ask him if he wants to continue
// If he does not, this function will not return (loader_failure will be called)
// This function may be called only from loaders!

idaman void ida_export lread(linput_t *li, void *buf, size_t size);


// Read the input source
// Return number of read bytes or -1

idaman ssize_t ida_export qlread(linput_t *li, void *buf, size_t size);


// Read one line from the input source
// Returns: NULL if failure, othersize 's'

idaman char *ida_export qlgets(char *s, size_t len, linput_t *li);


// Read one character from the input source
// Returns: EOF if failure, otherwise the read character

idaman int ida_export qlgetc(linput_t *li);


// Read several bytes
// Returns 0-ok, -1-failure

idaman int ida_export lreadbytes(linput_t *li, void *buf, size_t size, bool mf);

#define DEF_LREADBYTES(read, type, size)                        \
inline int idaapi read(linput_t *li, type *res, bool mostfirst) \
               { return lreadbytes(li, res, size, mostfirst); }
DEF_LREADBYTES(lread2bytes, short, 2)
DEF_LREADBYTES(lread2bytes, ushort, 2)
DEF_LREADBYTES(lread4bytes, long, 4)
DEF_LREADBYTES(lread4bytes, ulong, 4)
DEF_LREADBYTES(lread8bytes, longlong, 8)
DEF_LREADBYTES(lread8bytes, ulonglong, 8)
#undef DEF_LREADBYTES


// Get the input source size

idaman long ida_export qlsize(linput_t *li);


// Set input source position
// Returns the new position (not 0 as fseek!)

idaman long ida_export qlseek(linput_t *li, long pos, int whence=SEEK_SET);


// Get input source position

inline long idaapi qltell(linput_t *li) { return qlseek(li, 0, SEEK_CUR); }


// Open loader input

idaman linput_t *ida_export open_linput(const char *file, bool remote);


// Close loader input

idaman void ida_export close_linput(linput_t *li);


// Get FILE* from the input source
// If the input source is linked to a remote file, then return NULL
// Otherwise return the undeflying FILE*
// Please do not use this function if possible.

idaman FILE *ida_export qlfile(linput_t *li);


// Convert FILE * to input source
// Used to have a linput_t temporarily - call unmake_linput to free
// the slot after the use

idaman linput_t *ida_export make_linput(FILE *fp);
idaman void ida_export unmake_linput(linput_t *li);


//-------------------------------------------------------------------------
//      K E R N E L   O N L Y   F U N C T I O N S
//-------------------------------------------------------------------------

// The following definitions are for the kernel only:

idaman void ida_export checkdspace(unsigned long space); // if there is less than
                                                // 'space' on the current disk
                                                // warns the user
int     lowdiskgo(unsigned long space);         // if there is less than
                                                // 'space' on the current disk
                                                // asks user to confirm
                                                // returns 1 - continue


extern char **ida_argv;
extern char *exename;


#pragma pack(pop)
#endif // _DISKIO_HPP
