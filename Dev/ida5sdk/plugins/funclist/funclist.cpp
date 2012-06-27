/*
 *  This is a sample plugin module
 *
 *      It demonstrates how to get the the entry point prototypes
 *
 */

#include <ida.hpp>
#include <idp.hpp>
#include <auto.hpp>
#include <entry.hpp>
#include <bytes.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <typeinf.hpp>
#include <demangle.hpp>

#ifndef __WATCOMC__
#include <vector>
using std::vector;
#endif

struct item_t
{
  int ord;
  ea_t ea;
  string decl;
  ulong argsize;
};

typedef vector<item_t> entrylist_t;

//--------------------------------------------------------------------------
int init(void)
{
  if ( get_entry_qty() == 0 )
    return PLUGIN_SKIP;
  return PLUGIN_OK;
}

//--------------------------------------------------------------------------
// column widths
static const int widths[] = { 4, 8, 6, 70 };

// column headers
static const char *header[] =
{
  "Ordinal",
  "Address",
  "ArgSize",
  "Declaration",
};

//-------------------------------------------------------------------------
// function that returns number of lines in the list
static ulong idaapi sizer(void *obj)
{
  entrylist_t &li = *(entrylist_t *)obj;
  return li.size();
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
  n--;
  entrylist_t &li = *(entrylist_t *)obj;

  qsnprintf(arrptr[0], MAXSTR, "%d", li[n].ord);
  qsnprintf(arrptr[1], MAXSTR, "%08a", li[n].ea);
  if ( li[n].argsize != 0 )
    qsnprintf(arrptr[2], MAXSTR, "%04a", li[n].argsize);
  qsnprintf(arrptr[3], MAXSTR, "%s", li[n].decl.c_str());
}

//-------------------------------------------------------------------------
// function that is called when the user hits Enter
static void idaapi enter_cb(void *obj,ulong n)
{
  entrylist_t &li = *(entrylist_t *)obj;
  jumpto(li[n-1].ea);
}

//-------------------------------------------------------------------------
// function that is called when the window is closed
static void idaapi destroy_cb(void *obj)
{
  entrylist_t *li = (entrylist_t *)obj;
  delete li;
}

//--------------------------------------------------------------------------
void run(int /*arg*/)
{
  if ( !autoIsOk()
    && askyn_c(-1, "HIDECANCEL\n"
                   "The autoanalysis has not finished yet.\n"
                   "The result might be incomplete. Do you want to continue?") < 0 )
    return;

  // gather information about the entry points
  entrylist_t *li = new entrylist_t;
  int n = get_entry_qty();
  for ( int i=0; i < n; i++ )
  {
    int ord = get_entry_ordinal(i);
    ea_t ea = get_entry(ord);
    if ( ord == ea ) continue;
    type_t type[MAXSTR];
    p_list fnames[MAXSTR];
    char decl[MAXSTR];
    char true_name[MAXSTR];
    ulong argsize = 0;
    get_entry_name(ord, true_name, sizeof(true_name));
    if ( get_ti(ea, type, sizeof(type), fnames, sizeof(fnames))
      && print_type_to_one_line(
                decl, sizeof(decl),
                idati,
                type,
                true_name,
                NULL,
                fnames) == T_NORMAL )
    {
//    found type info -- calc the args size
      ulong arglocs[MAX_FUNC_ARGS];
      type_t *types[MAX_FUNC_ARGS];
      char *names[MAX_FUNC_ARGS];
      int a = build_funcarg_arrays(type, fnames, arglocs, types, names, MAX_FUNC_ARGS, false);
      if ( a != 0 )
      {
        for ( int k=0; k < a; k++ )
        {
          const type_t *ptr = types[k];
          int s1 = get_type_size(idati, ptr);
          s1 = qmax(s1, inf.cc.size_i);
          argsize += s1;
        }
        free_funcarg_arrays(types, names, a);
      }
    }
    else if ( get_long_name(BADADDR, ea, decl, sizeof(decl)) != NULL
           && get_true_name(BADADDR, ea, true_name, sizeof(true_name)) != NULL
           && strcmp(decl, true_name) != 0 )
    {
//      found mangled name
    }
    else
    {
//      found nothing, just show the name
      qstrncpy(decl, get_name(BADADDR, ea, true_name, sizeof(true_name)), sizeof(decl));
    }
    if ( argsize == 0 )
    {
      func_t *pfn = get_func(ea);
      if ( pfn != NULL )
        argsize = pfn->argsize;
    }
    item_t x;
    x.ord = ord;
    x.ea = ea;
    x.decl = decl;
    x.argsize = argsize;
    li->push_back(x);
  }

  // now open the window
  choose2(false,                // non-modal window
          -1, -1, -1, -1,       // position is determined by Windows
          li,                   // pass the created array
          qnumber(header),      // number of columns
          widths,               // widths of columns
          sizer,                // function that returns number of lines
          desc,                 // function that generates a line
          "Exported functions", // window title
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
plugin_t PLUGIN =
{
  IDP_INTERFACE_VERSION,
  0,                    // plugin flags
  init,                 // initialize
  NULL,
  run,                  // invoke plugin
  "Generate list of exported function prototypes",
  "Generate list of exported function prototypes",

  "List of exported functions",
  "Ctrl-F11",
};
