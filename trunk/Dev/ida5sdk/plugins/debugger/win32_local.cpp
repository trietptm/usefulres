#ifndef __NT__
#define EXCEPTION_ACCESS_VIOLATION          STATUS_ACCESS_VIOLATION
#define EXCEPTION_DATATYPE_MISALIGNMENT     STATUS_DATATYPE_MISALIGNMENT
#define EXCEPTION_BREAKPOINT                STATUS_BREAKPOINT
#define EXCEPTION_SINGLE_STEP               STATUS_SINGLE_STEP
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED     STATUS_ARRAY_BOUNDS_EXCEEDED
#define EXCEPTION_FLT_DENORMAL_OPERAND      STATUS_FLOAT_DENORMAL_OPERAND
#define EXCEPTION_FLT_DIVIDE_BY_ZERO        STATUS_FLOAT_DIVIDE_BY_ZERO
#define EXCEPTION_FLT_INEXACT_RESULT        STATUS_FLOAT_INEXACT_RESULT
#define EXCEPTION_FLT_INVALID_OPERATION     STATUS_FLOAT_INVALID_OPERATION
#define EXCEPTION_FLT_OVERFLOW              STATUS_FLOAT_OVERFLOW
#define EXCEPTION_FLT_STACK_CHECK           STATUS_FLOAT_STACK_CHECK
#define EXCEPTION_FLT_UNDERFLOW             STATUS_FLOAT_UNDERFLOW
#define EXCEPTION_INT_DIVIDE_BY_ZERO        STATUS_INTEGER_DIVIDE_BY_ZERO
#define EXCEPTION_INT_OVERFLOW              STATUS_INTEGER_OVERFLOW
#define EXCEPTION_PRIV_INSTRUCTION          STATUS_PRIVILEGED_INSTRUCTION
#define EXCEPTION_IN_PAGE_ERROR             STATUS_IN_PAGE_ERROR
#define EXCEPTION_ILLEGAL_INSTRUCTION       STATUS_ILLEGAL_INSTRUCTION
#define EXCEPTION_NONCONTINUABLE_EXCEPTION  STATUS_NONCONTINUABLE_EXCEPTION
#define EXCEPTION_STACK_OVERFLOW            STATUS_STACK_OVERFLOW
#define EXCEPTION_INVALID_DISPOSITION       STATUS_INVALID_DISPOSITION
#define EXCEPTION_GUARD_PAGE                STATUS_GUARD_PAGE_VIOLATION
#define EXCEPTION_INVALID_HANDLE            STATUS_INVALID_HANDLE
#define CONTROL_C_EXIT                      STATUS_CONTROL_C_EXIT
#define DBG_CONTROL_C                    0x40010005L
#define DBG_CONTROL_BREAK                0x40010008L
#define STATUS_GUARD_PAGE_VIOLATION      0x80000001L
#define STATUS_DATATYPE_MISALIGNMENT     0x80000002L
#define STATUS_BREAKPOINT                0x80000003L
#define STATUS_SINGLE_STEP               0x80000004L
#define STATUS_ACCESS_VIOLATION          0xC0000005L
#define STATUS_IN_PAGE_ERROR             0xC0000006L
#define STATUS_INVALID_HANDLE            0xC0000008L
#define STATUS_NO_MEMORY                 0xC0000017L
#define STATUS_ILLEGAL_INSTRUCTION       0xC000001DL
#define STATUS_NONCONTINUABLE_EXCEPTION  0xC0000025L
#define STATUS_INVALID_DISPOSITION       0xC0000026L
#define STATUS_ARRAY_BOUNDS_EXCEEDED     0xC000008CL
#define STATUS_FLOAT_DENORMAL_OPERAND    0xC000008DL
#define STATUS_FLOAT_DIVIDE_BY_ZERO      0xC000008EL
#define STATUS_FLOAT_INEXACT_RESULT      0xC000008FL
#define STATUS_FLOAT_INVALID_OPERATION   0xC0000090L
#define STATUS_FLOAT_OVERFLOW            0xC0000091L
#define STATUS_FLOAT_STACK_CHECK         0xC0000092L
#define STATUS_FLOAT_UNDERFLOW           0xC0000093L
#define STATUS_INTEGER_DIVIDE_BY_ZERO    0xC0000094L
#define STATUS_INTEGER_OVERFLOW          0xC0000095L
#define STATUS_PRIVILEGED_INSTRUCTION    0xC0000096L
#define STATUS_STACK_OVERFLOW            0xC00000FDL
#define STATUS_CONTROL_C_EXIT            0xC000013AL
#define STATUS_FLOAT_MULTIPLE_FAULTS     0xC00002B4L
#define STATUS_FLOAT_MULTIPLE_TRAPS      0xC00002B5L
#define STATUS_REG_NAT_CONSUMPTION       0xC00002C9L
#endif

#include <loader.hpp>
#include "../../ldr/pe/pe.h"

//--------------------------------------------------------------------------
void idaapi rebase_if_required_to(ea_t new_base)
{
  if ( is_miniidb() )
    return;
  netnode penode;
  penode.create(PE_NODE);
  ea_t currentbase = new_base;
  ea_t imagebase = penode.altval(PE_ALT_IMAGEBASE); // loading address (usually pe.imagebase)

  if ( imagebase == 0 )
  {
    warning("AUTOHIDE DATABASE\n"
            "IDA Pro couldn't automatically determine if the program should be\n"
            "rebased in the database because the database format is too old and\n"
            "doesn't contain enough information.\n"
            "Create a new database if you want automated rebasing to work properly.\n"
            "Note you can always manually rebase the program by using the\n"
            "Edit, Segments, Rebase program command.");
  }
  else if ( imagebase != currentbase )
  {
    int code = rebase_program(currentbase - imagebase, MSF_FIXONCE);
    if ( code != MOVE_SEGM_OK )
    {
      msg("Failed to rebase program, error code %d\n", code);
      warning("ICON ERROR\n"
              "AUTOHIDE NONE\n"
              "IDA Pro failed to rebase the program.\n"
              "Most likely it happened because of the debugger\n"
              "segments created to reflect the real memory state.\n\n"
              "Please stop the debugger and rebase the program manually.\n"
              "For that, please select the whole program and\n"
              "use Edit, Segments, Rebase program with delta 0x%08lX",
                                        currentbase - imagebase);
    }
  }
}

//--------------------------------------------------------------------------
// Initialize Win32 debugger plugin
static bool init_plugin(void)
{
  if ( !netnode::inited() || is_miniidb() )
  {
#ifdef __NT__
    // local debugger is available if we are running under Windows
    return true;
#else
    // for other systems only the remote debugger is available
    return debugger.is_remote();
#endif
  }

  if ( inf.filetype != f_PE ) return false; // only PE files
#ifdef USE_ASYNC        // connection to PocketPC device
  if ( ph.id != PLFM_ARM ) return false; // only ARM
#else
  if ( ph.id != PLFM_386 ) return false; // only IBM PC
#endif

  // find out the pe header
  netnode penode;
  penode.create(PE_NODE);
  peheader_t pe;
  if ( penode.valobj(&pe, sizeof(pe)) <= 0 )
    return false;

  is_dll = (pe.flags & PEF_DLL) != 0;

  if ( pe.subsys != PES_UNKNOWN )  // Unknown
  {
#ifdef USE_ASYNC        // connection to PocketPC device
    // debug only wince applications
    if ( pe.subsys != PES_WINCE )  // Windows CE
      return false;
#else
    // debug only gui or console applications
    if ( pe.subsys != PES_WINGUI && pe.subsys != PES_WINCHAR )
      return false;
#endif
  }
  return true;
}

//--------------------------------------------------------------------------
char comment[] = "Userland win32 debugger plugin.";

char help[] =
        "A sample Userland win32 debugger plugin\n"
        "\n"
        "This module shows you how to create debugger plugins.\n";

