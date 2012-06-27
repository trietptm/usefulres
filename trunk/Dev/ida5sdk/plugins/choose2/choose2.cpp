/*
 *  This is a sample plugin module
 *
 *  It demonstrates the use of the choose2() function
 *
 */

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <bytes.hpp>
#include <kernwin.hpp>

//--------------------------------------------------------------------------
//
//      Initialize.
//
int init(void)
{
  return PLUGIN_OK;
}

//--------------------------------------------------------------------------
//      Terminate.
//      Usually this callback is empty.
void term(void)
{
//  warning("term choose2");
}

//--------------------------------------------------------------------------
// column widths
static const int widths[] = { 8, 32 };

// column headers
static const char *header[] =
{
  "Address",
  "Instruction",
};

//-------------------------------------------------------------------------
// function that returns number of lines in the list
static ulong idaapi sizer(void *obj)
{
  netnode *node = (netnode *)obj;
  return node->altval(-1);       // we have saved the number in altval(-1)
}

//-------------------------------------------------------------------------
// function that generates the list line
static void idaapi desc(void *obj,ulong n,char * const *arrptr)
{
  if ( n == 0 ) // generate the column headers
  {
    for ( int i=0; i < qnumber(header); i++ )
      qstrncpy(arrptr[i], header[i], MAXSTR);
    return;
  }
  netnode *node = (netnode *)obj;
  ea_t ea = node->altval(n-1);
  generate_disasm_line(ea, arrptr[1], MAXSTR, 0);
  tag_remove(arrptr[1], arrptr[1], MAXSTR);  // remove the color coding
  qsnprintf(arrptr[0], MAXSTR, "%08a", ea);
}

//-------------------------------------------------------------------------
// function that is called when the user hits Enter
static void idaapi enter_cb(void *obj,ulong n)
{
  netnode *node = (netnode *)obj;
  jumpto(node->altval(n-1));
}

//-------------------------------------------------------------------------
// function that is called when the window is closed
static void idaapi destroy_cb(void *obj)
{
//  warning("destroy_cb");
  netnode *node = (netnode *)obj;
  node->kill();
  delete node;
}

//--------------------------------------------------------------------------
//
//      The plugin method
//
//      This is the main function of plugin.
//
void run(int /*arg*/)
{
  char title[MAXSTR];
  // Let's display the functions called from the current one
  // or from the selected area

  // First we determine the working area

  func_item_iterator_t fii;
  bool ok;
  ea_t ea1, ea2;
  if ( callui(ui_readsel, &ea1, &ea2).cnd )    // the selection is present?
  {
    callui(ui_unmarksel);                      // unmark selection
    qsnprintf(title, sizeof(title), "Functions called from %08a..%08a", ea1, ea2);
    ok = fii.set_range(ea1, ea2);
  }
  else                                         // nothing is selected
  {
    func_t *pfn = get_func(get_screen_ea());   // try the current function
    if ( pfn == NULL )
    {
      warning("Please position the cursor on a function or select an area");
      return;
    }
    ok = fii.set(pfn);
    static const char str[] = "Functions called from ";
    char *ptr = qstpncpy(title, str, sizeof(title));
    get_func_name(ea1, ptr, sizeof(title)-(ptr-title));
  }

  // We are going to remember the call instruction addresses
  // in a netnode
  // altval(i) will contain the address of the call instruction

  netnode *node = new netnode;
  node->create();
  int counter = 0;
  while ( ok )
  {
    ea_t ea = fii.current();
    if ( is_call_insn(ea) )       // a call instruction is found
      node->altset(counter++, ea);//get_first_fcref_from(ea));
    ok = fii.next_code();
  }

  // altval(-1) will contain the number of pairs
  node->altset(-1, counter);

  // now open the window
  choose2(false,                // non-modal window
          -1, -1, -1, -1,       // position is determined by Windows
          node,                 // pass the created netnode to the window
          qnumber(header),      // number of columns
          widths,               // widths of columns
          sizer,                // function that returns number of lines
          desc,                 // function that generates a line
          title,                // window title
          -1,                   // use the default icon for the window
          0,                    // position the cursor on the first line
          NULL,                 // "kill" callback
          NULL,                 // "new" callback
          NULL,                 // "update" callback
          NULL,                 // "edit" callback
          enter_cb,             // function to call when the user pressed Enter
          destroy_cb,           // function to call when the window is closed
          NULL,                 // use default popup menu items
          NULL);                // use the same icon for all lines
}

//--------------------------------------------------------------------------
char comment[] = "This is a sample plugin. It displays the chooser window";

char help[] =
        "A sample plugin module\n"
        "\n"
        "This module shows you how to use choose2() function.\n";


//--------------------------------------------------------------------------
// This is the preferred name of the plugin module in the menu system
// The preferred name may be overriden in plugins.cfg file

char wanted_name[] = "Called functions";


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
  0,                    // plugin flags
  init,                 // initialize

  term,                 // terminate. this pointer may be NULL.

  run,                  // invoke plugin

  comment,              // long comment about the plugin
                        // it could appear in the status line
                        // or as a hint

  help,                 // multiline help about the plugin

  wanted_name,          // the preferred short name of the plugin
  wanted_hotkey         // the preferred hotkey to run the plugin
};
