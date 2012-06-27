
// IDA Pro plugin to load function name information from PDB files

// This file was modify from the old PDB plugin.

// I rewrite the sybmol loading code.
// It use native dbghelp.dll now and it should support 64 bit if
// IDA does. Test with windows XP SP1 PDB files.

// Make sure you have the lastest dbghelp.dll in your search
// path. I put the dbghelp.dll in the plugin directory.

// You can define the symbol search path like windbg does.
//                                  - Christopher Li

// Revision history:
//      - Removed static linking to DBGHELP.DLL
//      - Added support for different versions of DBGHELP.DLL
//                                                      Ilfak Guilfanov


/*//////////////////////////////////////////////////////////////////////
                           Necessary Includes
//////////////////////////////////////////////////////////////////////*/

#include <windows.h>
#include <dbghelp.h>
#ifdef __BORLANDC__
#if __BORLANDC__ < 0x560
#include "bc5_add.h"
#endif
#endif

#include <ida.hpp>
#include <idp.hpp>
#include <err.h>
#include <name.hpp>
#include <loader.hpp>
#include <diskio.hpp>
#include <demangle.hpp>

static ulong counter;
static char download_path[QMAXPATH];

#ifdef _DEBUG
#define debug_print(x) msg x
#else
#define debug_print(x)
#endif


//----------------------------------------------------------------------
// Support of old debug interface
//----------------------------------------------------------------------
// Because some systems could not to have IMAGEHLP.DLL, we are not going
// to link its functions statically.
// Instead, we use LoadLibrary and the following pointers:


typedef
BOOL IMAGEAPI t_SymLoadModule(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD           BaseOfDll,
    IN  DWORD           SizeOfDll
    );

typedef
BOOL IMAGEAPI SymEnumerateSymbols64_t(
    IN HANDLE                       hProcess,
    IN DWORD64                      BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK64  EnumSymbolsCallback64,
    IN PVOID                        UserContext
    );

typedef
BOOL IMAGEAPI t_SymEnumerateSymbols(
    IN HANDLE                       hProcess,
    IN DWORD                        BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK    EnumSymbolsCallback,
    IN PVOID                        UserContext
    );

typedef
BOOL IMAGEAPI t_SymGetModuleInfo(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE    ModuleInfo
    );


typedef
BOOL IMAGEAPI t_SymInitialize(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     fInvadeProcess
    );


typedef
DWORD IMAGEAPI t_SymSetOptions(
    IN DWORD   SymOptions
    );

typedef
BOOL
IMAGEAPI
t_SymUnloadModule(
    IN  HANDLE          hProcess,
    IN  DWORD64         BaseOfDll
    );

typedef
BOOL
IMAGEAPI
t_SymCleanup(
    IN HANDLE hProcess
    );

static HINSTANCE imagehlp;
static t_SymLoadModule       *p_SymLoadModule       = NULL;
static t_SymEnumerateSymbols *p_SymEnumerateSymbols = NULL;
static t_SymGetModuleInfo    *p_SymGetModuleInfo    = NULL;
static t_SymInitialize       *p_SymInitialize       = NULL;
static t_SymSetOptions       *p_SymSetOptions       = NULL;
static t_SymUnloadModule     *p_SymUnloadModule     = NULL;
static t_SymCleanup          *p_SymCleanup          = NULL;
static bool use_old = false; // use old method

// Old method: Dynamically load and link to IMAGEHLP
bool old_setup_pointers(void)
{
  // Since there could be no IMAGEHLP.DLL on the system, we link to
  // the functions at run-time. Usually this is not necessary.
  imagehlp = LoadLibrary("IMAGEHLP.DLL");
  if ( imagehlp == NULL )
  {
    deb(IDA_DEBUG_PLUGIN, "PDB plugin: failed to load IMAGEHLP.DLL");
    return false;         // There is no imagehlp.dll
                          // in this system...
  }
  p_SymLoadModule       = (t_SymLoadModule      *)(GetProcAddress(imagehlp, "SymLoadModule"));
  p_SymEnumerateSymbols = (t_SymEnumerateSymbols*)(GetProcAddress(imagehlp, "SymEnumerateSymbols"));
  p_SymGetModuleInfo    = (t_SymGetModuleInfo   *)(GetProcAddress(imagehlp, "SymGetModuleInfo"));
  p_SymInitialize       = (t_SymInitialize      *)(GetProcAddress(imagehlp, "SymInitialize"));
  p_SymSetOptions       = (t_SymSetOptions      *)(GetProcAddress(imagehlp, "SymSetOptions"));
  p_SymUnloadModule     = (t_SymUnloadModule    *)(GetProcAddress(imagehlp, "SymUnloadModule"));
  p_SymCleanup          = (t_SymCleanup         *)(GetProcAddress(imagehlp, "SymCleanup"));
  if ( p_SymLoadModule       == NULL
    || p_SymEnumerateSymbols == NULL
    || p_SymGetModuleInfo    == NULL
    || p_SymInitialize       == NULL
    || p_SymSetOptions       == NULL
    || p_SymUnloadModule     == NULL
    || p_SymCleanup          == NULL )
  {
    deb(IDA_DEBUG_PLUGIN, "PDB plugin: Essential IMAGEHLP.DLL functions are missing\n");
    FreeLibrary(imagehlp);
    imagehlp = NULL;
    return false;
  }
  use_old = true;
  return true;
}

//----------------------------------------------------------------------
// Support of new debug interface
//----------------------------------------------------------------------
typedef DWORD IMAGEAPI SymSetOptions_t(IN DWORD SymOptions);
typedef BOOL IMAGEAPI SymInitialize_t(IN HANDLE hProcess, IN LPCSTR UserSearchPath, IN BOOL fInvadeProcess);
typedef DWORD64 IMAGEAPI SymLoadModule64_t(IN HANDLE hProcess, IN HANDLE hFile, IN PSTR ImageName, IN PSTR ModuleName, IN DWORD64 BaseOfDll, IN DWORD SizeOfDll);
typedef BOOL IMAGEAPI SymEnumSymbols_t(IN HANDLE hProcess, IN ULONG64 BaseOfDll, IN PCSTR Mask, IN PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback, IN PVOID UserContext);
typedef BOOL IMAGEAPI SymUnloadModule64_t(IN HANDLE hProcess, IN DWORD64 BaseOfDll);
typedef BOOL IMAGEAPI SymCleanup_t(IN HANDLE hProcess);

static HINSTANCE dbghelp = NULL;
static SymSetOptions_t     *pSymSetOptions     = NULL;
static SymInitialize_t     *pSymInitialize     = NULL;
static SymLoadModule64_t   *pSymLoadModule64   = NULL;
static SymEnumSymbols_t    *pSymEnumSymbols    = NULL;
static SymUnloadModule64_t *pSymUnloadModule64 = NULL;
static SymCleanup_t        *pSymCleanup        = NULL;
static SymEnumerateSymbols64_t *pSymEnumerateSymbols64 = NULL;

//----------------------------------------------------------------------
// Dynamically load and link to DBGHELP or IMAGEHLP libraries
// Return: success
static bool setup_pointers(void)
{
  char dll[QMAXPATH];
  if ( getsysfile(dll, sizeof(dll), "dbghelp.dll", NULL) == NULL )
    return false;

  dbghelp = LoadLibrary(dll);
  if ( dbghelp == NULL )
  {
    deb(IDA_DEBUG_PLUGIN, "PDB plugin: failed to load DBGHELP.DLL");
  }
  else
  {
    pSymSetOptions     = (SymSetOptions_t    *)GetProcAddress(dbghelp, "SymSetOptions");
    pSymInitialize     = (SymInitialize_t    *)GetProcAddress(dbghelp, "SymInitialize");
    pSymLoadModule64   = (SymLoadModule64_t  *)GetProcAddress(dbghelp, "SymLoadModule64");
    pSymEnumSymbols    = (SymEnumSymbols_t   *)GetProcAddress(dbghelp, "SymEnumSymbols");
    pSymUnloadModule64 = (SymUnloadModule64_t*)GetProcAddress(dbghelp, "SymUnloadModule64");
    pSymCleanup        = (SymCleanup_t       *)GetProcAddress(dbghelp, "SymCleanup");
    pSymEnumerateSymbols64 = (SymEnumerateSymbols64_t*)(GetProcAddress(dbghelp, "SymEnumerateSymbols64"));

    if ( pSymSetOptions     != NULL
      && pSymInitialize     != NULL
      && pSymLoadModule64   != NULL
      && pSymUnloadModule64 != NULL
      && pSymCleanup        != NULL
      && (   pSymEnumSymbols != NULL  // required XP or higher
          || pSymEnumerateSymbols64 != NULL)) return true;
  }
  deb(IDA_DEBUG_PLUGIN, "PDB plugin: Essential DBGHELP.DLL functions are missing\n");
  FreeLibrary(dbghelp);
  dbghelp = NULL;
  return old_setup_pointers();
}

//----------------------------------------------------------------------
static bool apply_name(ea_t ea, const char *name)
{
//  warning("%a: %s\n", ea, name);

  char buf[MAXSTR];
  // check for meaningless 'string' names
  if ( demangle(buf, sizeof(buf), name, long(inf.short_demnames)) > 0 )
  {
    if ( strcmp(buf, "`string'") == 0 )
    {
      size_t s1 = get_max_ascii_length(ea, ASCSTR_C);
      size_t s2 = get_max_ascii_length(ea, ASCSTR_UNICODE);
      make_ascii_string(ea, 0, s1 >= s2 ? ASCSTR_C : ASCSTR_UNICODE);
      return true;
    }
  }

  counter += set_name(ea, name, SN_NOWARN);

  int stype = segtype(ea);
  if ( stype == SEG_NORM || stype == SEG_CODE ) // only for code or normal segments
  {
    if ( get_mangled_name_type(name) != MANGLED_DATA )
      add_func(ea, BADADDR);   // don't create funcs in data segments
                               // add_func will fail if there are no instructions
  }
  showAddr(ea); // so the user doesn't get bored

  return true;
}

//----------------------------------------------------------------------
// New method: symbol enumeration callback
static BOOL CALLBACK EnumerateSymbolsProc(
    PSYMBOL_INFO pSymInfo,
    ULONG /*SymbolSize*/,
    PVOID delta)
{

// For some reason the 'Flags' is always zero :(
//  if ( pSymInfo->Flags )
//    warning("%s %x %x %d\n", pSymInfo->Name, pSymInfo->Address, pSymInfo->Flags, SymbolSize);

  // Tell IDA kernel: rename the function
  ea_t ea = (ea_t)(pSymInfo->Address + *(adiff_t*)delta);
  char *name = pSymInfo->Name;


  if (pSymInfo->Flags || pSymInfo->TypeIndex) {
          debug_print(("Flags %x, name %s, addr %x, typeindex %x\n",
                  pSymInfo->Flags, pSymInfo->Name,
                  pSymInfo->Address, pSymInfo->TypeIndex));
  }

  return apply_name(ea, name);
}

//----------------------------------------------------------------------
// New (universal) method: symbol enumeration callback
static BOOL CALLBACK EnumSymbolsProc64( PSTR    szName,
                                        DWORD64 ulAddr,
                                        ULONG   /*ulSize*/,
                                        PVOID   ud  )
{
  adiff_t delta = *(adiff_t *)ud;
  ea_t ea = ulAddr + delta;
  return apply_name(ea, szName);
}

//----------------------------------------------------------------------
// Display a system error message
static void error_msg(char *name)
{
  msg("%s: %s\n", name, winerr(GetLastError()));
}

//----------------------------------------------------------------------
// Old method: symbol enumeration callback
static BOOL CALLBACK OldEnumSymbolsProc( PSTR  szName,
                                    ULONG ulAddr,
                                    ULONG /*ulSize*/,
                                    PVOID ud  )
{
  adiff_t delta = *(adiff_t *)ud;
  ea_t ea = ulAddr + delta;
  return apply_name(ea, szName);
}

//----------------------------------------------------------------------
// Old method: use imagehlp.dll function to extract information
static void run_old_plugin(char *input_file, ea_t loaded_base)
{
  // Inform the user that we are using the old method
  info("AUTOHIDE REGISTRY\n"
       "The PDB plugin will use the old debug interface since your computer\n"
       "does not have new debug support libraries.\n"
       "Not all information will be extracted from the PDB file.\n"
       "\n"
       "If you want to use the new debug interface, then please download\n"
       "and install the debugging tools from\n"
       "http://www.microsoft.com/whdc/devtools/debugging/default.mspx");

  void *fake_proc = (void *) 0xBEEFFEED;

  p_SymSetOptions(SYMOPT_LOAD_LINES);
  if (!p_SymInitialize(fake_proc, NULL, FALSE))
  {
    error_msg("SymInitialize");
    return;
  }

  DWORD symbase = p_SymLoadModule(fake_proc, 0, input_file, NULL, loaded_base, 0);
  if ( symbase == 0 )
  {
    error_msg("SymLoadModule");
  }
  else
  {
    adiff_t delta = loaded_base - symbase;
    counter = 0;

    if (!p_SymEnumerateSymbols(fake_proc, loaded_base,
                PSYM_ENUMSYMBOLS_CALLBACK(OldEnumSymbolsProc), &delta) )
    {
      error_msg("SymEnumSymbols");
    }
    else
    {
      msg("PDB: total %d symbol%s loaded\n", counter, counter>1 ? "s" : "");
    }
    if (!p_SymUnloadModule(fake_proc, symbase))
      error_msg("SymUnloadModule");
  }
  if (!p_SymCleanup(fake_proc))
    error_msg("SymCleanup");
}

//--------------------------------------------------------------------------
// callback for parsing config file
static const char * idaapi parse_options(const char *keyword, int value_type,
                                         const void *value)
{
  if ( strcmp(keyword, "PDBSYM_DOWNLOAD_PATH") != 0 )
    return IDPOPT_BADKEY;

  if ( value_type != IDPOPT_STR )
    return IDPOPT_BADTYPE;

  qstrncpy(download_path, (const char *)value, sizeof(download_path));

  // empty string used for ida program directory
  if ( download_path[0] != '\0' && !qisdir(download_path) )
    return IDPOPT_BADVALUE;

  return IDPOPT_OK;
}

//----------------------------------------------------------------------
// Main function: do the real job here
// called_from_loader==1: ida decided to call the plugin itself
void idaapi plugin_main(int called_from_loader)
{
  adiff_t delta;
  char input_path[QMAXPATH], *input = input_path;
  void *fake_proc = (void *) 0xBEEFFEED;
  DWORD64 symbase;

  if ( !setup_pointers() ) return; // since the module has been unloaded, reinitialize

  netnode penode("$ PE header");
  ea_t loaded_base = penode.altval(-2);

  // Get the input file name and try to guess the PDB file locaton
  // If failed, ask the user
  get_input_file_path(input_path, sizeof(input_path));
  if ( !qfileexist(input) )
  {
    input = askfile_c(false, input, "Please specify the input file");
    if ( input == NULL )
      return;
  }

  if ( use_old )
  {
    run_old_plugin(input, loaded_base);
    return;
  }

  // make symbol search path: use symbol server, store in TEMP directory.
  // default place for download is TempDir\ida
  if ( !GetTempPath(sizeof(download_path), download_path) )
    download_path[0] = '\0';
  else
    qstrncat(download_path, "ida", sizeof(download_path));
  read_user_config_file("pdb", parse_options);
  static const char spath_prefix[] = "srv*";
  static const char spath_suffix[] = "*http://msdl.microsoft.com/download/symbols";
  char spath[sizeof(spath_prefix)+sizeof(download_path)+sizeof(spath_suffix)];
  char *ptr = spath, *end = ptr + sizeof(spath);
  APPEND(ptr, end, spath_prefix);
  APPEND(ptr, end, download_path);
  APPEND(ptr, end, spath_suffix);
  if ( !pSymInitialize(fake_proc, spath, FALSE) )
  {
    error_msg("SymInitialize");
    return;
  }

  pSymSetOptions(SYMOPT_LOAD_LINES);

  static const char question[] = "AUTOHIDE REGISTRY\nHIDECANCEL\n"
    "IDA Pro has determined that the input file was linked with debug information.\n"
    "Do you want to look for the corresponding PDB file at the local symbol store\n"
    "and the Microsoft Symbol Server?\n";
  if ( called_from_loader && askyn_c(1, question) <= 0 )
    goto cleanup;

  msg("(Retrieving) and loading the PDB file...\n");
  symbase = pSymLoadModule64(fake_proc, 0, input, NULL, loaded_base, 0);
  if ( symbase == 0 )
    goto cleanup;

  counter = 0;
  delta = loaded_base - symbase;
  bool ok;
  if ( pSymEnumerateSymbols64 != NULL )
    ok = pSymEnumerateSymbols64(fake_proc, symbase, EnumSymbolsProc64, &delta);
  else
    ok = pSymEnumSymbols(fake_proc, (DWORD) symbase, "", EnumerateSymbolsProc, &delta);
  if ( !ok )
  {
    error_msg("EnumSymbols");
    goto unload;
  }

  msg("PDB: total %d symbol%s loaded\n", counter, counter>1 ? "s" : "");

unload:
  if (!pSymUnloadModule64(fake_proc, symbase))
    error_msg("SymUnloadModule64:");

cleanup:
  if (!pSymCleanup(fake_proc))
    error_msg("SymCleanup");
}



/*//////////////////////////////////////////////////////////////////////
                      IDA PRO INTERFACE START HERE
//////////////////////////////////////////////////////////////////////*/
//--------------------------------------------------------------------------
//      terminate
//      usually this callback is empty
//
//      IDA will call this function when the user asks to exit.
//      This function won't be called in the case of emergency exits.

void idaapi term(void)
{
  if ( dbghelp != NULL )
  {
    FreeLibrary(dbghelp);
    dbghelp = NULL;
  }
  if ( imagehlp != NULL )
  {
    FreeLibrary(imagehlp);
    imagehlp = NULL;
  }
}
//--------------------------------------------------------------------------
//
//      initialize plugin
//
//      IDA will call this function only once.
//      If this function returns PLGUIN_SKIP, IDA will never load it again.
//      If this function returns PLUGIN_OK, IDA will unload the plugin but
//      remember that the plugin agreed to work with the database.
//      The plugin will be loaded again if the user invokes it by
//      pressing the hotkey or selecting it from the menu.
//      After the second load the plugin will stay on memory.
//
//      In this example we check the input file format and make the decision.
//      You may or may not check any other conditions to decide what you do:
//      whether you agree to work with the database or not.
//

int idaapi init(void)
{
  if ( inf.filetype != f_PE ) return PLUGIN_SKIP; // only for PE files
  if ( !setup_pointers() ) return PLUGIN_SKIP;
  term(); // unload libraries
  return PLUGIN_OK;
}

//--------------------------------------------------------------------------
char comment[] = "Load debug information from a PDB file";

char help[] =
"PDB file loader\n"
"\n"
"This module allows you to load debug information about function names\n"
"from a PDB file.\n"
"\n"
"The PDB file should be in the same directory as the input file\n";


//--------------------------------------------------------------------------
// This is the preferred name of the plugin module in the menu system
// The preferred name may be overriden in plugins.cfg file

char wanted_name[] = "Load PDB file (dbghelp 4.1+)";


// This is the preferred hotkey for the plugin module
// The preferred hotkey may be overriden in plugins.cfg file
// Note: IDA won't tell you if the hotkey is not correct
//       It will just disable the hotkey.

char wanted_hotkey[] = ""; // Ctrl-F12 is used to draw function call graph now
                           // No hotkey for PDB files, but since it is not
                           // used very often, it is tolerable

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_UNL | PLUGIN_MOD | PLUGIN_HIDE, // plugin flags:
  init,                 // initialize

  term,                 // terminate. this pointer may be NULL.

  plugin_main,          // invoke plugin

  comment,              // long comment about the plugin
                        // it could appear in the status line
                        // or as a hint

  help,                 // multiline help about the plugin

  wanted_name,          // the preferred short name of the plugin
  wanted_hotkey         // the preferred hotkey to run the plugin
};


