/*
 *  Change the callee address for constructions like
 *
 *  call esi    ; LocalFree
 *
 */

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <bytes.hpp>
#include <auto.hpp>

//--------------------------------------------------------------------------
int init(void)
{
  if ( ph.id != PLFM_386 ) return PLUGIN_SKIP; // only for 80x86 processors
  return PLUGIN_OK;
}

//--------------------------------------------------------------------------
static char comment[] = "Change the callee address";
static char help[] =
        "This plugin allows the user to change the address of the called function\n"
        "in constructs like\n"
        "\n"
        "       call esi\n"
        "\n"
        "You can enter a function name instead of its address\n";

//--------------------------------------------------------------------------
static const char form[] =
"HELP\n"
"%s\n"
"ENDHELP\n"
"Enter the callee address\n"
"\n"
"  <~C~allee:$:500:40:::>\n"
"\n"
"\n";

void run(int)
{
  netnode n("$ vmm functions");
  ea_t ea = get_screen_ea();    // get current address
  if ( !isCode(get_flags_novalue(ea)) ) return; // not an instruction
  ea_t callee = n.altval(ea)-1;         // get the callee address from the database
  char buf[MAXSTR];
  qsnprintf(buf, sizeof(buf), form, help);
  if ( AskUsingForm_c(buf, &callee) )
  {
    n.altset(ea, callee+1);     // save the new address
    noUsed(ea);                 // reanalyze the current instruction
  }
}

//--------------------------------------------------------------------------
static char wanted_name[] = "Change the callee address";
static char wanted_hotkey[] = "Alt-F11";


//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  0,                    // plugin flags
  init,                 // initialize

  NULL,                 // terminate. this pointer may be NULL.

  run,                  // invoke plugin

  comment,              // long comment about the plugin
                        // it could appear in the status line
                        // or as a hint

  help,                 // multiline help about the plugin

  wanted_name,          // the preferred short name of the plugin
  wanted_hotkey         // the preferred hotkey to run the plugin
};
