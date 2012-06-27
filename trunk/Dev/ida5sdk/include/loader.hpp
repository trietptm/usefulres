/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-97 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 */

#ifndef _LOADER_HPP
#define _LOADER_HPP
#pragma pack(push, 1)           // IDA uses 1 byte alignments!

//
//      This file contains:
//        - definitions of IDP, LDR, PLUGIN module interfaces.
//        - functions to load files into the database
//        - functions to generate output files
//        - high level functions to work with the database (open, save, close)
//
//      The LDR interface consists of one structure loader_t
//      The IDP interface consists of one structure processor_t (see idp.hpp)
//      The PLUGIN interface consists of one structure plugin_t
//
//      Modules can't use standard FILE* functions.
//      They must use functions from <fpro.h>
//
//      Modules can't use standard memory allocation functions.
//      They must use functions from <pro.h>
//
//      The exported entry #1 in the module should point to the
//      the appropriate structure. (loader_t for LDR module, for example)
//

//----------------------------------------------------------------------
//              DEFINITION OF LDR MODULES
//----------------------------------------------------------------------

class linput_t;         // loader input source. see diskio.hpp for the functions

// Loader description block - must be exported from the loader module

#define MAX_FILE_FORMAT_NAME    64

struct loader_t
{
  ulong version;        // api version, should be IDP_INTERFACE_VERSION
  ulong flags;          // loader flags
#define LDRF_RELOAD  0x0001     // loader recognizes NEF_RELOAD flag

//
//      check input file format. if recognized, then return 1
//      and fill 'fileformatname'.
//      otherwise return 0
//      This function will be called many times till it returns !=0.
//      'n' parameter will be incremented after each call.
//      Initially, n==0 for each loader.
//      This function may return a unique file format number instead of 1.
//      To get this unique number, please contact the author.
//
//      If the return value is ORed with ACCEPT_FIRST, then this format
//      should be placed first in the "load file" dialog box
//
#define ACCEPT_FIRST    0x8000

  int (idaapi* accept_file)(linput_t *li,
                            char fileformatname[MAX_FILE_FORMAT_NAME],
                            int n);
//
//      load file into the database.
//              fp       - pointer to file positioned at the start of the file
//              fileformatname - name of type of the file (it was returned
//                               by the accept_file)
//              neflags  - user-supplied flags. They determine how to
//                         load the file.
//      in the case of failure loader_failure() should be called
//
  void (idaapi* load_file)(linput_t *li,
                           ushort neflags,
                           const char *fileformatname);
#define NEF_SEGS        0x0001            // Create segments
#define NEF_RSCS        0x0002            // Load resources
#define NEF_NAME        0x0004            // Rename entries
#define NEF_MAN         0x0008            // Manual load
#define NEF_FILL        0x0010            // Fill segment gaps
#define NEF_IMPS        0x0020            // Create imports section
#define NEF_TIGHT       0x0040            // Don't align segments (OMF)
#define NEF_FIRST       0x0080            // This is the first file loaded
                                          // into the database.
#define NEF_CODE        0x0100            // for load_binary_file:
                                          //   load as a code segment
#define NEF_RELOAD      0x0200            // reload the file at the same place:
                                          //   don't create segments
                                          //   don't create fixup info
                                          //   don't import segments
                                          //   etc
                                          // load only the bytes into the base.
                                          // a loader should have LDRF_RELOAD
                                          // bit set
#define NEF_FLAT        0x0400            // Autocreate FLAT group (PE)
#define NEF_MINI        0x0800            // Create mini database (do not copy
#define NEF_LOPT        0x1000            // Display additional loader options dialog
//
//      create output file from the database.
//      this function may be absent.
//      if fp == NULL, then this function returns:
//                      0 - can't create file of this type
//                      1 - ok, can create file of this type
//      if fp != NULL, then this function should create the output file
//
  int (idaapi* save_file)(FILE *fp, const char *fileformatname);

//      take care of a moved segment (fix up relocations, for example)
//      this function may be absent.
//        from           - previous linear address of the segment
//        to             - current linear address of the segment
//        size           - size of the moved segment
//        fileformatname - the file format
//      a special calling method move_segm(BADADDR, delta, 0, formatname)
//      means that the whole program has been moved in the memory (rebased) by delta bytes
//      returns: 1-ok, 0-failure

  int (idaapi* move_segm)(ea_t from,
                          ea_t to,
                          asize_t size,
                          const char *fileformatname);

//      initialize user configurable options based on the input file.
//      this function may be absent.
//        fp             - pointer to file positioned at the start of the file
//      This function is called as soon as a loader is selected, and allow the loader
//      to populate XML variables, select a default processor, ...
//      returns: true-ok, false-cancel

  bool (idaapi* init_loader_options)(linput_t *li);

};

#ifdef __BORLANDC__
#if sizeof(loader_t) % 4
#error "Size of loader_t is incorrect"
#endif
#endif

extern "C" loader_t LDSC;          // (declaration for loaders)

// Display a message about a loader failure and stop the loading process
// The kernel will destroy the database
// If format == NULL, no message will be displayed
// This function does not return (it longjumps)!
// It may be called only from loader_t.load_file()

idaman void ida_export vloader_failure(const char *format, va_list va);
inline void loader_failure(const char *format=NULL, ...)
{
  va_list va;
  va_start(va, format);
  vloader_failure(format, va);
  va_end(va);
}

//----------------------------------------------------------------------
// LDR module file name extensions:

#ifdef __NT__
#ifdef __EA64__
#ifdef __AMD64__
#define LOADER_EXT "x64"
#else
#define LOADER_EXT "l64"
#endif
#else
#define LOADER_EXT "ldw"
#endif
#endif

#ifdef __LINUX__
#ifdef __EA64__
#define LOADER_EXT "llx64"
#else
#define LOADER_EXT "llx"
#endif
#endif

#ifdef __EA64__
#define LOADER_DLL "*64." LOADER_EXT
#else
#define LOADER_DLL "*." LOADER_EXT
#endif

//----------------------------------------------------------------------
//      Functions for the UI to load files
//----------------------------------------------------------------------

struct load_info_t      // List of loaders
{
  load_info_t *next;
  char dllname[QMAXPATH];
  char ftypename[MAX_FILE_FORMAT_NAME];
  filetype_t ftype;
  int pri;              // 1-place first, 0-normal priority
};

// Build list of potential loaders

idaman load_info_t *ida_export build_loaders_list(linput_t *li);


// Free the list of loaders

idaman void ida_export free_loaders_list(load_info_t *list);


// Get name of loader from its DLL file.
// (for example, for PE files we will get "PE")

idaman char *ida_export get_loader_name_from_dll(char *dllname);


// Get name of loader used to load the input file into the database
// If no external loader was used, returns -1
// Otherwise copies the loader file name without the extension in the buf
// and returns its length
// (for example, for PE files we will get "PE")

idaman ssize_t ida_export get_loader_name(char *buf, size_t bufsize);


// Initialize user configurable options from the given loader
// based on the input file.

idaman bool ida_export init_loader_options(linput_t *li, load_info_t *loader);


// Load a binary file into the database.
// This function usually is called from ui.
//      filename   - the name of input file as is
//                   (if the input file is from library, then
//                    this is the name from the library)
//      li        - loader input source
//      _neflags  - see NEF_.. constants. For the first file
//                  the flag NEF_FIRST must be set.
//      fileoff   - Offset in the input file
//      basepara  - Load address in paragraphs
//      binoff    - Load offset (load_address=(basepara<<4)+binoff)
//      nbytes    - Number of bytes to load from the file
//                  0 - up to the end of the file
//                  If nbytes is bigger than the number of
//                  bytes rest, the kernel will load as much
//                  as possible
// Returns: 1-ok, 0-failed (couldn't open the file)

idaman bool ida_export load_binary_file(
                             const char *filename,
                             linput_t *li,
                             ushort _neflags,
                             long fileoff,
                             ea_t basepara,
                             ea_t binoff,
                             ulong nbytes);


// Load a non-binary file into the database.
// This function usually is called from ui.
//      filename   - the name of input file as is
//                   (if the input file is from library, then
//                    this is the name from the library)
//      li         - loader input source
//      sysdlldir  - a directory with system dlls. Pass "." if unknown.
//      _neflags   - see NEF_.. constants. For the first file
//                   the flag NEF_FIRST must be set.
//      loader     - pointer to load_info_t structure.
//                   If the current IDP module has ph.loader != NULL
//                   then this argument is ignored.
// Returns: 1-ok, 0-failed

idaman bool ida_export load_nonbinary_file(
                                const char *filename,
                                linput_t *li,
                                const char *sysdlldir,
                                ushort _neflags,
                                load_info_t *loader);


//--------------------------------------------------------------------------
// Output file types:

typedef int ofile_type_t;
const ofile_type_t
  OFILE_MAP  = 0,        // MAP file
  OFILE_EXE  = 1,        // Executable file
  OFILE_IDC  = 2,        // IDC file
  OFILE_LST  = 3,        // Disassembly listing
  OFILE_ASM  = 4,        // Assembly
  OFILE_DIF  = 5;        // Difference

// Callback functions to output lines:

typedef int idaapi html_header_cb_t(FILE *fp);
typedef int idaapi html_footer_cb_t(FILE *fp);
typedef int idaapi html_line_cb_t(FILE *fp,
                                  const char *line,
                                  bgcolor_t prefix_color,
                                  bgcolor_t bg_color);
#define gen_outline_t html_line_cb_t

//------------------------------------------------------------------
// Generate an output file
//      otype  - type of output file.
//      fp     - the output file handle
//      ea1    - start address. For some file types this argument is ignored
//      ea2    - end address. For some file types this argument is ignored
//               as usual in ida, the end address of the range is not included
//      flags  - bit combination of GENFLG_...
// returns: number of the generated lines.
//          -1 if an error occured
//          ofile_exe: 0-can't generate exe file, 1-ok

idaman int ida_export gen_file(ofile_type_t otype, FILE *fp, ea_t ea1, ea_t ea2, int flags);

// 'flag' is a combination of the following:
#define GENFLG_MAPSEG  0x0001          // map: generate map of segments
#define GENFLG_MAPNAME 0x0002          // map: include dummy names
#define GENFLG_MAPDMNG 0x0004          // map: demangle names
#define GENFLG_MAPLOC  0x0008          // map: include local names
#define GENFLG_IDCTYPE 0x0008          // idc: gen only information about types
#define GENFLG_ASMTYPE 0x0010          // asm&lst: gen information about types too
#define GENFLG_GENHTML 0x0020          // asm&lst: generate html (ui_genfile_callback will be used)
#define GENFLG_ASMINC  0x0040          // asm&lst: gen information only about types


//----------------------------------------------------------------------
//      Helper functions for the loaders & ui
//----------------------------------------------------------------------

// Load portion of file into the database
// This function will include (ea1..ea2) into the addressing space of the
// program (make it enabled)
//      li         - pointer ot input source
//      pos        - position in the file
//      (ea1..ea2) - range of destination linear addresses
//      patchable  - should the kernel remember correspondance of
//                   file offsets to linear addresses.
// returns: 1-ok,0-read error, a warning is displayed

idaman int ida_export file2base(linput_t *li,
                                long pos,
                                ea_t ea1,
                                ea_t ea2,
                                int patchable);

#define FILEREG_PATCHABLE       1       // means that the input file may be
                                        // patched (i.e. no compression,
                                        // no iterated data, etc)
#define FILEREG_NOTPATCHABLE    0       // the data is kept in some encoded
                                        // form in the file.


// Load database from the memory.
//      memptr     - pointer to buffer with bytes
//      (ea1..ea2) - range of destination linear addresses
//      fpos       - position in the input file the data is taken from.
//                   if == -1, then no file position correspond to the data.
// This function works for wide byte processors too.
// returns: always 1

idaman int ida_export mem2base(const void *memptr,ea_t ea1,ea_t ea2,long fpos);


// Unload database to a binary file.
//      fp         - pointer to file
//      pos        - position in the file
//      (ea1..ea2) - range of source linear addresses
// This function works for wide byte processors too.
// returns: 1-ok(always), write error leads to immediate exit

idaman int ida_export base2file(FILE *fp,long pos,ea_t ea1,ea_t ea2);


// Load information from DBG file into the database
//      li    - handler to opened input. If NULL, then fname is checked
//      lname - name of loader module without extension and path
//      fname - name of file to load (only if fp == NULL)
//      is_remote - is the file located on a remote computer with
//                  the debugger server?
// Returns: 1-ok, 0-error (message is displayed)

idaman int ida_export load_loader_module(linput_t *li,
                                         const char *lname,
                                         const char *fname,
                                         bool is_remote);


// Add long comment at inf.minEA:
//      Input file:     ....
//      File format:    ....
// This function should be called only from the loader to describe the input file.

idaman void ida_export create_filename_cmt(void);


// Get the input file type
// This function can recognize libraries and zip files.

idaman filetype_t ida_export get_basic_file_type(linput_t *li);


// Get name of the current file type
// The current file type is kept in inf.filetype.
//      buf - buffer for the file type name
//      bufsize - its size
// Returns: size of answer, this function always succeeds

idaman size_t ida_export get_file_type_name(char *buf, size_t bufsize);


//----------------------------------------------------------------------
//      Work with IDS files: read and use information from them
//
// This structure is used in import_module():

struct impinfo_t
{
  const char *dllname;
  void (idaapi*func)(uval_t num, const char *name, uval_t node);
  uval_t node;
};

// Find and import DLL module.
// This function adds information to the database (renames functions, etc)
//      module  - name of DLL
//      windir  - system directory with dlls
//      modnode - node with information about imported entries
//                imports by ordinals:
//                  altval(ord) contains linear address
//                imports by name:
//                  supval(ea) contains the imported name
//                Either altval or supval arrays may be absent.
//                The node should never be deleted.
//      importer- callback function (may be NULL):
//                check dll module
//              call 'func' for all exported entries in the file
//                      fp - pointer to file opened in binary mode.
//                      file position is 0.
//                      ud - pointer to impinfo_t structure
//              this function checks that 'dllname' match the name of module
//              if not, it returns 0
//              otherwise it calls 'func' for each exported entry
//              if dllname==NULL then 'func' will be called with num==0 and name==dllname
//              and returns:
//                      0 - dllname doesn't match, should continue
//                      1 - ok
//      ostype  - type of operating system (subdir name)
//                NULL means the IDS directory itself (not recommended)

idaman void ida_export import_module(const char *module,
                   const char *windir,
                   uval_t modnode,
                   int (idaapi*importer)(linput_t *li,impinfo_t *ii),
                   const char *ostype);


// Load and apply IDS file
//      fname - name of file to apply
// This function loads the specified IDS file and applies it to the database
// If the program imports functions from a module with the same name
// as the name of the ids file being loaded, then only functions from this
// module will be affected. Otherwise (i.e. when the program does not import
// a module with this name) any function in the program may be affected.
// Returns:
//      1 - ok
//      0 - some error (a message is displayed)
//          if the ids file does not exist, no message is displayed

idaman int ida_export load_ids_module(char *fname);


//----------------------------------------------------------------------
//              DEFINITION OF PLUGIN MODULES
//----------------------------------------------------------------------
// A plugin is a module in plugins subdirectory which can be called by
// pressing a hotkey. It usually performs an action asked by the user.

class plugin_t
{
public:
  int version;          // Should be equal to IDP_INTERFACE_VERSION
  int flags;            // Features of the plugin:
#define PLUGIN_MOD  0x0001      // Plugin changes the database.
                                // IDA won't call the plugin if
                                // the processor prohibited any changes
                                // by setting PR_NOCHANGES in processor_t.
#define PLUGIN_DRAW 0x0002      // IDA should redraw everything after calling
                                // the plugin
#define PLUGIN_SEG  0x0004      // Plugin may be applied only if the
                                // current address belongs to a segment
#define PLUGIN_UNL  0x0008      // Unload the plugin immediately after
                                // calling 'run'.
                                // This flag may be set anytime.
                                // The kernel checks it after each call to 'run'
                                // The main purpose of this flag is to ease
                                // the debugging of new plugins.
#define PLUGIN_HIDE 0x0010      // Plugin should not appear in the Edit, Plugins menu
                                // This flag is checked at the start
#define PLUGIN_DBG  0x0020      // A debugger plugin. init() should put
                                // the address of debugger_t to dbg
                                // See idd.hpp for details
#define PLUGIN_PROC 0x0040      // Load plugin when a processor module is loaded and keep it
                                // until the processor module is unloaded
#define PLUGIN_FIX  0x0080      // Load plugin when IDA starts and keep it in the
                                // memory until IDA stops
  int (idaapi* init)(void); // Initialize plugin
#define PLUGIN_SKIP  0  // Plugin doesn't want to be loaded
#define PLUGIN_OK    1  // Plugin agrees to work with the current database
                        // It will be loaded as soon as the user presses the hotkey
#define PLUGIN_KEEP  2  // Plugin agrees to work with the current database
                        // and wants to stay in the memory
  void (idaapi* term)(void);   // Terminate plugin. This function will be called
                        // when the plugin is unloaded. May be NULL.
  void (idaapi* run)(int arg); // Invoke plugin
  char *comment;        // Long comment about the plugin
                        // it could appear in the status line
                        // or as a hint
  char *help;           // Multiline help about the plugin
  char *wanted_name;    // The preferred short name of the plugin
  char *wanted_hotkey;  // The preferred hotkey to run the plugin
};

extern "C" plugin_t PLUGIN; // (declaration for plugins)

//--------------------------------------------------------------------------
// A plugin can hook to the notification point and receive notifications
// of all major events in IDA. The callback function will be called
// for each event. The parameters of the callback:
//      user_data         - data supplied in call to hook_to_notification_point()
//      notification_code - idp_notify or ui_notification_t code, depending on
//                          the hoot type
//      va                - additional parameters supplied with the notification
//                          see the event descriptions for information
// The callback should return:
//      0   - ok, the event should be processed further
//      !=0 - the event is blocked and should be discarded
//            in the case of processor modules, the returned value is used
//            as the return value of notify()

typedef int idaapi hook_cb_t(void *user_data, int notification_code, va_list va);

enum hook_type_t
{
  HT_IDP,         // Hook to the processor module.
                  // The callback will receive all idp_notify events.
                  // See file idp.hpp for the list of events.
  HT_UI,          // Hook to the user interface.
                  // The callback will receive all ui_notification_t events.
                  // See file kernwin.hpp for the list of events.
  HT_DBG,         // Hook to the debugger.
                  // The callback will receive all dbg_notification_t events.
                  // See file dbg.hpp for the list of events.
  HT_GRAPH,       // Hook to the graph events
                  // The callback will receive all graph_notification_t events.
                  // See file graph.hpp for the list of events.
  HT_LAST
};

idaman bool ida_export hook_to_notification_point(hook_type_t hook_type,
                                hook_cb_t *cb,
                                void *user_data);


// The plugin should unhook before being unloaded:
// (preferably in its termination function)
// Returns number of unhooked functions.
// If different callbacks have the same callback function pointer
// and user_data is not NULL, only the callback whose associated
// user defined data matchs will be removed.

idaman int ida_export unhook_from_notification_point(hook_type_t hook_type,
                                    hook_cb_t *cb,
                                    void *user_data = NULL);


// A well behaved processor module should call this function
// in his notify() function. If this function returns 0, then
// the processor module should process the notification itself
// Otherwise the code should be returned to the caller, like this:
//
//      int code = invoke_callbacks(HT_IDP, what, va);
//      if ( code ) return code;
//      ...
//

idaman int ida_export invoke_callbacks(hook_type_t hook_type, int notification_code, va_list va);

// Method to generate graph notifications:
inline int idaapi grcall(int code, ...)
{
  va_list va;
  va_start(va, code);
  int result = invoke_callbacks(HT_GRAPH, code, va);
  va_end(va);
  return result;
}


// Get plugin options from the command line
// If the user has specified the options in the -Oplugin_name:options
// format, them this function will return the 'options' part of it
// The 'plugin' parameter should denote the plugin name
// Returns NULL if there we no options specified

idaman const char *ida_export get_plugin_options(const char *plugin);


//--------------------------------------------------------------------------
// PLUGIN module file name extensions:

#ifdef __NT__
#ifdef __EA64__
#ifdef __AMD64__
#define PLUGIN_EXT  "x64"
#else
#define PLUGIN_EXT  "p64"
#endif
#else
#define PLUGIN_EXT  "plw"
#endif
#endif
#ifdef __LINUX__
#ifdef __EA64__
#define PLUGIN_EXT  "plx64"
#else
#define PLUGIN_EXT  "plx"
#endif
#endif

#define PLUGIN_DLL "*." PLUGIN_EXT

//----------------------------------------------------------------------
// LOW LEVEL DLL LOADING FUNCTIONS
// Only the kernel should use these functions!

#define LNE_MAXSEG      10      // Max number of segments

#if 0
extern char dlldata[4096];      // Reserved place for DLL data
#define DLLDATASTART    0xA0    // Absolute offset of dlldata
extern char ldrdata[64];        // Reserved place for LOADER data
#define LDRDATASTART    (DLLDATASTART+sizeof(dlldata)) // Absolute offset of ldrdata
#endif

struct idadll_t
{
  void *dllinfo[LNE_MAXSEG];
  void *entry;                  // first entry point of DLL
  bool is_loaded(void) const { return dllinfo[0] != NULL; }
};

int load_dll(const char *file, idadll_t *dllmem);
                                // dllmem - allocated segments
                                //          dos: segment 1 (data) isn't allocated
                                // Returns 0 - ok, else:
#define RE_NOFILE       1       /* No such file */
#define RE_NOTIDP       2       /* Not IDP file */
#define RE_NOPAGE       3       /* Can't load: bad segments */
#define RE_NOLINK       4       /* No linkage info */
#define RE_BADRTP       5       /* Bad relocation type */
#define RE_BADORD       6       /* Bad imported ordinal */
#define RE_BADATP       7       /* Bad relocation atype */
#define RE_BADMAP       8       /* DLLDATA offset is invalid */

void                  load_dll_or_die(const char *file, idadll_t *dllmem);
idaman int ida_export load_dll_or_say(const char *file, idadll_t *dllmem);

idaman void ida_export free_dll(idadll_t *dllmem);

// The processor description string should be at the offset 0x80 of the IDP file
// (the string consists of the processor types separated by colons)

#define IDP_DESC_START   0x80
#define IDP_DESC_END    0x200

idaman char *ida_export get_idp_desc(const char *file, char *buf, size_t bufsize); // Get IDP module name

//--------------------------------------------------------------------------
// IDP module file name extensions:

#ifdef __NT__
#ifdef __EA64__
#ifdef __AMD64__
#define IDP_EXT  "x64"
#else
#define IDP_EXT  "w64"
#endif
#else
#define IDP_EXT  "w32"
#endif
#endif
#ifdef __LINUX__
#ifdef __EA64__
#define IDP_EXT  "ilx64"
#else
#define IDP_EXT  "ilx"
#endif
#endif

#ifdef __EA64__
#define IDP_DLL "*64." IDP_EXT
#else
#define IDP_DLL "*." IDP_EXT
#endif


//--------------------------------------------------------------------------
// Plugin information in IDA is stored in the following structure:
struct plugin_info_t
{
  plugin_info_t *next;  // next plugin information
  char *path;           // full path to the plugin
  char *org_name;       // original short name of the plugin
  char *name;           // short name of the plugin
                        // it will appear in the menu
  ushort org_hotkey;    // original hotkey to run the plugin
  ushort hotkey;        // current hotkey to run the plugin
  int arg;              // argument used to call the plugin
  plugin_t *entry;      // pointer to the plugin if it is already loaded
  idadll_t dllmem;
  int flags;            // a copy of plugin_t.flags
};

// Get pointer to the list of plugins (some plugins might be listed several times
// in the list - once for each configured argument)

idaman plugin_info_t *ida_export get_plugins(void);


// Load a user-defined plugin
//   name - short plugin name without path and extension
//          or absolute path to the file name
// Returns: pointer to plugin description block

idaman plugin_t *ida_export load_plugin(const char *name);

// Run a loaded plugin with the specified argument
//      ptr - pointer to plugin description block
//      arg - argument to run with


idaman bool ida_export run_plugin(plugin_t *ptr, int arg);


// Load & run a plugin

inline bool idaapi load_and_run_plugin(const char *name, int arg)
{
  return run_plugin(load_plugin(name), arg);
}


// Run a plugin as configured
//      ptr - pointer to plugin information block

idaman bool ida_export invoke_plugin(plugin_info_t *ptr);


// Information for the user interface about available debuggers
struct dbg_info_t
{
  plugin_info_t *pi;
  struct debugger_t *dbg;
  dbg_info_t(plugin_info_t *_pi, struct debugger_t *_dbg) : pi(_pi), dbg(_dbg) {}
};

idaman size_t ida_export get_debugger_plugins(const dbg_info_t **array);


// Initialize plugins with the specified flag

idaman void ida_export init_plugins(int flag);


// Terminate plugins with the specified flag

idaman void ida_export term_plugins(int flag);


//------------------------------------------------------------------------

//
//      work with file regions (for patching)
//
void init_fileregions(void);                            // called by the kernel
void term_fileregions(void);                            // called by the kernel
inline void save_fileregions(void) {}                   // called by the kernel
void add_fileregion(ea_t ea1,ea_t ea2,long fpos);       // called by the kernel
void move_fileregions(ea_t from, ea_t to, asize_t size);// called by the kernel

// Get offset in the input file which corresponds to the given ea
// If the specified ea can't be mapped into the input file offset,
// return -1.

idaman long ida_export get_fileregion_offset(ea_t ea);


// Get linear address which corresponds to the specified input file offset.
// If can't be found, then return BADADDR

idaman ea_t ida_export get_fileregion_ea(long offset);


//------------------------------------------------------------------------
// Generate an exe file (unload the database in binary form)
//      fp  - the output file handle
//            if fp == NULL then returns:
//                      1 - can generate an executable file
//                      0 - can't generate an executable file
// returns: 1-ok, 0-failed

idaman int ida_export gen_exe_file(FILE *fp);


//------------------------------------------------------------------------
// Reload the input file
// This function reloads the byte values from the input file
// It doesn't modify the segmentation, names, comments, etc.
//      file - name of the input file
//             if file == NULL then returns:
//                      1 - can reload the input file
//                      0 - can't reload the input file
//      is_remote - is the file located on a remote computer with
//                  the debugger server?
// returns: 1-ok, 0-failed

idaman int ida_export reload_file(const char *file, bool is_remote);


//------------------------------------------------------------------------
// Generate an IDC file (unload the database in text form)
//      fp        - the output file handle
//      onlytypes - if true then generate idc about enums, structs and
//                  other type definitions used in the program
// returns: 1-ok, 0-failed

int local_gen_idc_file(FILE *fp, ea_t ea1, ea_t ea2, bool onlytypes);


//------------------------------------------------------------------------
// Output to a file all lines controlled by place 'pl'
// These functions are for the kernel only.

class place_t;
long print_all_places(FILE *fp,place_t *pl, html_line_cb_t *line_cb = NULL);

extern html_line_cb_t save_text_line;
long idaapi print_all_structs(FILE *fp, html_line_cb_t *line_cb = NULL);
long idaapi print_all_enums(FILE *fp, html_line_cb_t *line_cb = NULL);

//--------------------------------------------------------------------------
//
//      KERNEL ONLY functions & data
//

idaman ida_export_data char command_line_file[QMAXPATH];  // full path to the file specified in the command line
idaman ida_export_data char database_idb[QMAXPATH];       // full path of IDB file
extern char database_id0[QMAXPATH];                       // full path of original ID0 file (not exported)
idaman bool ida_export is_database_ext(const char *ext);  // check the file extension

extern ulong ida_database_memory;       // Database buffer size in bytes.

idaman ulong ida_export_data database_flags; // close_database() will:
#define DBFL_KILL       0x01            // delete unpacked database
#define DBFL_COMP       0x02            // compress database
#define DBFL_BAK        0x04            // create backup file
                                        // (only if idb file is created)
#define DBFL_TEMP       0x08            // temporary database

inline bool is_temp_database(void) { return (database_flags & DBFL_TEMP) != 0; }

extern bool pe_create_idata;
extern bool pe_load_resources;
extern bool pe_create_flat_group;

// database check codes
typedef int dbcheck_t;
const dbcheck_t
  DBCHK_NONE = 0,               // database doesn't exist
  DBCHK_OK   = 1,               // database exists
  DBCHK_BAD  = 2,               // database exists but we can't use it
  DBCHK_NEW  = 3;               // database exists but we the user asked to destroy it

dbcheck_t check_database(const char *file);

int open_database(bool isnew, const char *file, ulong input_size, bool is_temp_database);
                                                // returns 'waspacked'
idaman int ida_export flush_buffers(void);             // Flush buffers to disk
idaman void ida_export save_database(const char *outfile, bool delete_unpacked);
                                                // Flush buffers and make
                                                // a copy of database
void close_database(void);                      // see database_flags
bool compress_btree(const char *btree_file);    // (internal) compress .ID0
bool get_input_file_from_archive(char *filename, size_t namesize, bool is_remote, char **temp_file_ptr);
bool loader_move_segm(ea_t from, ea_t to, asize_t size, bool keep);
int generate_ida_copyright(FILE *fp, const char *cmt, html_line_cb_t *line_cb);
bool is_in_loader(void);
void get_ids_filename(char *buf,
                      size_t bufsize,
                      const char *idsname,
                      const char *ostype,
                      bool *use_ordinals,
                      char **autotils,
                      int maxtils);

#pragma pack(pop)
#endif
