/*
 *  This is a sample plugin module
 *
 *  It can be compiled by any of the supported compilers:
 *
 *      - Borland C++, CBuilder, free C++
 *      - Watcom C++ for DOS32
 *      - Watcom C++ for OS/2
 *      - Visual C++
 *
 */

#include <windows.h>

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

//--------------------------------------------------------------------------
int idaapi init(void)
{
  // stay in the memory since most likely we will define our own window proc
  // and add subcontrols to the window
  return PLUGIN_KEEP;
}

//--------------------------------------------------------------------------
void idaapi run(int /*arg*/)
{
  HWND hwnd = NULL;
  TForm *form = create_tform("Sample subwindow", &hwnd);
  if ( hwnd != NULL )
    open_tform(form, FORM_MDI|FORM_TAB|FORM_MENU);
  else
    close_tform(form, 0);
}

//--------------------------------------------------------------------------
char comment[] = "This is a sample plugin.";

char help[] =
        "A sample plugin module\n"
        "\n"
        "This module shows you how to create MDI child window.";


//--------------------------------------------------------------------------
// This is the preferred name of the plugin module in the menu system
// The preferred name may be overriden in plugins.cfg file

char wanted_name[] = "Create IDA subwindow";


// This is the preferred hotkey for the plugin module
// The preferred hotkey may be overriden in plugins.cfg file
// Note: IDA won't tell you if the hotkey is not correct
//       It will just disable the hotkey.

char wanted_hotkey[] = "";


//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  PLUGIN_UNL,           // plugin flags
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
