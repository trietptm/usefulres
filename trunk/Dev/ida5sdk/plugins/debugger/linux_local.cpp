#include <loader.hpp>

//--------------------------------------------------------------------------
void idaapi rebase_if_required_to(ea_t new_base)
{
/*  netnode penode;
  penode.create(PE_NODE);
  ea_t currentbase = new_base;
  ea_t imagebase = penode.altval(-2); // loading address (usually pe.imagebase)
  if ( imagebase != currentbase )
  {
    int code = rebase_program(currentbase - imagebase, MSF_FIXONCE);
    if ( code != MOVE_SEGM_OK )
    {
      msg("Failed to rebase program, error code %d\n", code);
      warning("IDA Pro failed to rebase the program.\n"
              "Most likely it happened because of the debugger\n"
              "segments created to reflect the real memory state.\n\n"
              "Please stop the debugger and rebase the program manually.\n"
              "For that, please select the whole program and\n"
              "use Edit, Segments, Rebase program with delta 0x%08lX",
                                        currentbase - imagebase);
    }
  }*/
}

//--------------------------------------------------------------------------
static bool init_plugin(void)
{
  if ( !netnode::inited() || is_miniidb() )
  {
#ifdef __LINUX__
    // local debugger is available if we are running under Linux
    return true;
#else
    // for other systems only the remote debugger is available
    return debugger.is_remote();
#endif
  }

  if ( inf.filetype != f_ELF ) return false; // only ELF files
  if ( ph.id != PLFM_386 ) return false; // only IBM PC

  is_dll = false; // FIXME!

  return true;
}

//--------------------------------------------------------------------------
char comment[] = "Userland linux debugger plugin.";

char help[] =
        "A sample Userland linux debugger plugin\n"
        "\n"
        "This module shows you how to create debugger plugins.\n";

