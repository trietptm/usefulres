/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su, ig@datarescue.com
 *                              FIDO:   2:5020/209
 *
 */

//
//      This file defines the interface between the kernel and the user
//      interface (UI). It contains:
//              - the UI dispatcher notification codes
//              - convention functions for UI services
//              - structures which hold information about the
//                lines (disassembly, structures, enums) generated
//                by the kernel
//              - functions to interact with the user (dialog boxes)
//              - some string and conversion functions.
//

#ifndef __KERNWIN_HPP
#define __KERNWIN_HPP
#pragma pack(push, 1)           // IDA uses 1 byte alignments!

#include <ida.hpp>
#include <fpro.h>
#include <help.h>
#include <llong.hpp>

typedef int atype_t;            // autoanalysis types
typedef int idastate_t;         // ida state indicator (ready, thinking, waiting)
typedef uchar color_t;          // see <lines.hpp>
typedef uval_t bmask_t;         // see <enum.hpp>
typedef tid_t enum_t;           // see <enum.hpp>

// Message box kinds:

enum mbox_kind_t
{
  mbox_internal,                // internal error
  mbox_info,
  mbox_warning,
  mbox_error,
  mbox_nomem,
  mbox_feedback,
  mbox_readerror,
  mbox_writeerror,
  mbox_filestruct,
  mbox_wait,
  mbox_hide,
};


// List chooser types

enum choose_type_t
{
  chtype_generic,               // the generic choose() function (1-column)
  chtype_idasgn,                // signature
  chtype_entry,                 // entry point
  chtype_name,                  // name from list
  chtype_stkvar_xref,           // xref to stack variable
  chtype_xref,                  // xref to address
  chtype_enum,                  // enum
  chtype_enum_by_value,         // enum (restriction by value)
  chtype_func,                  // function
  chtype_segm,                  // segment
  chtype_segreg,                // segment register change point
  chtype_struc,                 // structure
  chtype_strpath,               // structure path
  chtype_generic2,              // the generic choose2() function (n-column)
  chtype_idatil,                // type information libraries
};


enum beep_t             // Beep types
{
  beep_default = 0
};


// Notify UI about various events. The kernel will call this function
// when something interesting for the UI happens.
// The UI should avoid calling the kernel from this callback.

class func_t;
class segment_t;
class segreg_t;
class struc_t;
class member_t;
class TView;
class plugin_t;
class minsn_t;
class value_t;
class linput_t;

union callui_t          // Return codes (size of this type should be 4 bytes at most)
{                       //              (otherwise different compilers return it differently)
  bool cnd;
  char i8;
  int i;
  short i16;
  long i32;
  uchar u8;
  ushort u16;
  ulong u32;
  char *cptr;
  void *vptr;
  func_t *fptr;
  segment_t *segptr;
  segreg_t *sregptr;
  struc_t *strptr;
  plugin_t *pluginptr;
};

#ifdef __BORLANDC__
#if sizeof(callui_t) > 4
#error bad callui_t definition
#endif
#endif

// Events marked as '*' should be used as a parameter to callui()
// See convenience functions below (like get_screen_ea())
// Events marked as 'cb' are designed to be callbacks and should not
// be used in callui(). The user may hook to the notification event to catch them

enum ui_notification_t
{
  ui_null = 0,

  ui_range,             // cb: the disassembly range changed (inf.minEA..inf.maxEA)
                        // UI should redraw the scrollbars
                        // See also: lock_range_refresh
                        // Parameters: none
                        // Returns:    none

  ui_list,              // cb: the list window contents are changed (names, signatures, etc)
                        // UI should redraw them
                        // Parameters: none
                        // Returns:    none

  ui_idcstart,          // cb: Start of IDC engine work
                        // Parameters: none
                        // Returns:    none

  ui_idcstop,           // cb: Stop of IDC engine work
                        // Parameters: none
                        // Returns:    none

  ui_suspend,           // cb: Suspend graphical interface.
                        // Only the text version
                        // interface should response to it
                        // Parameters: none
                        // Returns:    none

  ui_resume,            // cb: Resume the suspended graphical interface.
                        // Only the text version
                        // interface should response to it
                        // Parameters: none
                        // Returns:    none

  ui_jumpto,            // * Jump to the specified addres
                        // Parameters:
                        //      ea_t ea
                        //      int operand_num (-1: don't change x coord)
                        // Returns: bool success

  ui_readsel,           // * Get the selected area bounds
                        // Parameters:
                        //      ea_t *startea
                        //      ea_t *endea
                        // Returns: bool
                        //          0 - no area is selected
                        //          1 - ok, startea and endea are filled

  ui_unmarksel,         // * Unmark selection
                        // Parameters: none
                        // Returns:    none

  ui_screenea,          // * Return the address at the screen cursor
                        // Parameters: ea_t *result
                        // Returns:    none

  ui_saving,            // cb: The kernel is saving the database.
                        // The user interface should save its state.
                        // Parameters: none
                        // Returns:    none

  ui_saved,             // cb: The kernel has saved the database.
                        // This callback just informs the interface.
                        // Parameters: none
                        // Returns:    none

  ui_refreshmarked,     // * Refresh marked disassembly views
                        // Parameters: none
                        // Returns:    none

  ui_refresh,           // * Refresh all disassembly views
                        // Parameters: none
                        // Returns:    none

  ui_choose,            // * Allow the user to choose an object
                        // Parameters:
                        //      choose_type_t type
                        //      ...
                        // other parameters depend on the 'type'
                        // see below for inline functions using this
                        // notification code.
                        // Always use the helper inline functions below.
                        // Returns: depends on the 'type'

  ui_close_chooser,     // * Close a non-modal chooser
                        // Parameters:
                        //      const char *title
                        // Returns: bool success

  ui_banner,            // * Show a banner dialog box
                        // Parameters:
                        //      int wait
                        // Returns: bool 1-ok, 0-esc was pressed

  ui_setidle,           // * Set a function to call at idle times
                        // Parameters:
                        //      int (*func)(void);
                        // Returns: none

  ui_noabort,           // * Disable 'abort' menu item - the database was not
                        // compressed
                        // Parameters: none
                        // Returns:    none

  ui_term,              // cb: IDA is terminated
                        // The database is already closed.
                        // The UI may close its windows in this callback.
                        // Parameters: none
                        // Returns:    none

  ui_mbox,              // * Show a message box
                        // Parameters:
                        //      mbox_kind_t kind
                        //      const char *format
                        //      va_list va
                        // Returns: none

  ui_beep,              // * Beep
                        // Parameters:
                        //      beep_t beep_type
                        // Returns:    none

  ui_msg,               // * Show a message in the message window
                        // Parameters:
                        //      const char *format
                        //      va_list va
                        // Returns: number of bytes output

  ui_askyn,             // * Ask the user and get his yes/no response
                        // Parameters:
                        //      const char *yes_button
                        //      const char *no_button
                        //      const char *cancel_button
                        //      int default_answer
                        //      const char *format
                        //      va_list va
                        // Returns: -1-cancel/0-no/1-yes

  ui_askfile,           // * Ask the user a file name
                        // Parameters:
                        //      int savefile
                        //      const char *default_answer
                        //      const char *format
                        //      va_list va
                        // Returns: file name

  ui_form,              // * Show a dialog form
                        // Parameters:
                        //      const char *format
                        //      va_list va
                        // Returns: bool 0-esc, 1-ok

  ui_close_form,        // * Close the form
                        // This function may be called from pushbutton
                        // callbacks in ui_form
                        //      TView *fields[]
                        //      int is_ok
                        // Returns: none

  ui_clearbreak,        // * clear ctrl-break flag
                        // Parameters: none
                        // Returns: none

  ui_wasbreak,          // * test the ctrl-break flag
                        // Parameters: none
                        // Returns: 1 - Ctrl-Break is detected, a message is displayed
                        //          2 - Ctrl-Break is detected again, a message is not displayed
                        //          0 - Ctrl-Break is not detected

  ui_asktext,           // * Ask text
                        // Parameters:
                        //      size_t size
                        //      char *answer
                        //      const char *default_value
                        //      const char *format
                        //      va_list va
                        // Returns: the entered text

  ui_askstr,            // * Ask a string
                        // Parameters:
                        //      int history_number
                        //      const char *default_value
                        //      const char *format
                        //      va_list va
                        // Returns: the entered string

  ui_askident,          // * Ask an identifier
                        // Parameters:
                        //      const char *default_value
                        //      const char *format
                        //      va_list va
                        // Returns: cptr the entered identifier

  ui_askaddr,           // * Ask an address
                        // Parameters:
                        //      ea_t *answer
                        //      const char *format
                        //      va_list va
                        // Returns: bool success

  ui_askseg,            // * Ask a segment
                        // Parameters:
                        //      sel_t *answer
                        //      const char *format
                        //      va_list va
                        // Returns: bool success

  ui_asklong,           // * Ask a long
                        // Parameters:
                        //      sval_t *answer
                        //      const char *format
                        //      va_list va
                        // Returns: bool success

  ui_showauto,          // * Show the autoanalysis state
                        // Parameters:
                        //      ea_t ea
                        //      int auto_t (see auto.hpp)
                        // Returns: none

  ui_setstate,          // * Show READY, BUSY, THINKING, etc
                        // Parameters:
                        //      int idastate_t (see auto.hpp)
                        // Returns: int: old ida state

  ui_add_idckey,        // * Add hotkey for IDC function
                        // After this function the UI should call the
                        // specified IDC function
                        // when the hotkey is pressed
                        // Parameters:
                        //      const char *hotkey
                        //      const char *idcfuncname
                        // Returns: int code
#define IDCHK_OK        0       // ok
#define IDCHK_ARG       -1      // bad argument(s)
#define IDCHK_KEY       -2      // bad hotkey name
#define IDCHK_MAX       -3      // too many IDC hotkeys

  ui_del_idckey,        // * Delete IDC function hotkey
                        // Parameters:
                        //      hotkey  - hotkey name
                        // Returns: bool success

  ui_get_marker,        // * Get pointer to function
                        // "void mark_idaview_for_refresh(ea_t ea)"
                        // This function will be called by the kernel when the
                        // database is changed
                        // Parameters: none
                        // Returns: vptr: (idaapi*marker)(ea_t ea) or NULL

  ui_analyzer_options,  // * Allow the user to set analyzer options
                        // (show a dialog box)
                        // Parameters: none
                        // Returns: none

  ui_is_msg_inited,     // * Can we use msg() functions?
                        // Parameters: none
                        // Returns: bool cnd

  ui_load_file,         // Display a load file dialog and load file
                        // Parameters:
                        //      const char *filename
                        //              the name of input file as is
                        //              (if the input file is from library,
                        //               then this is the name in the library)
                        //      linput_t *li
                        //              loader input source
                        //      ushort neflags
                        //              combination of NEF_... bits
                        //              (see loader.hpp)
                        // Returns: bool cnd;

  ui_run_dbg,           // * Load a debugger plugin and run the specified program
                        // Parameters:
                        //      const char *dbgopts - value of the -r command line switch
                        //      const char *exename - name of the file to run
                        //      int argc            - number of arguments for the executable
                        //      char **argv         - argument vector
                        // Returns: bool cnd

  ui_get_cursor,        // * Get the cursor position on the screen
                        // Parameters:
                        //             int *x
                        //             int *y
                        // Returns:    bool cnd
                        //               true: x,y pointers are filled
                        //               false: no disassemly window open

  ui_get_curline,       // * Get current line from the disassemble window
                        // Parameters: none
                        // Returns:    cptr current line with the color codes
                        // (use tag_remove function to remove the color codes)

  ui_get_hwnd,          // * Get HWND of the main IDA window
                        // Parameters: none
                        // Returns:    txt version: NULL
                        //             gui version: HWND
                        // HWND is returned in result.vptr

  ui_copywarn,          // * Display copyright warning
                        // Parameters: none
                        // Returns:    bool yes/no

  ui_getvcl,            // * Get VCL variables
                        // Parameters:
                        //              TApplication **app
                        //              TScreen **screen
                        //              TMouse **mouse
                        // Returns: int sizeof(TApplication)+sizeof(TScreen)+sizeof(TMouse)
                        // The text version fills the pointers with NULLs and returns 0

  ui_idp_event,         // cb: A processor module event has been generated (idp.hpp, idp_notify)
                        // Parameteres:
                        //      ph::idp_notify event_code
                        //      va_list va
                        // Returns:
                        //      int code; code==0 - process the event
                        //                otherwise return code as the result
                        // This event should not be used as a parameter to callui()
                        // The kernel uses it to notify the ui about the events

  ui_lock_range_refresh,// * Lock the ui_range refreshes. The ranges will not
                        // be refreshed until the corresponding unlock_range_refresh
                        // is issued.
                        // See also: unlock_range_refresh
                        // Parameters: none
                        // Returns:    none

  ui_unlock_range_refresh,// * Unlock the ui_range refreshes. If the number of locks
                        // is back to zero, then refresh the ranges.
                        // See also: ui_range
                        // Parameters: none
                        // Returns:    none

  ui_setbreak,          // * set ctrl-break flag
                        // Parameters: none
                        // Returns: none

  ui_genfile_callback,  // cb: handle html generation
                        // parameters: html_header_cb_t **,
                        //             html_footer_cb_t **,
                        //             html_line_cb_t **
                        // returns: nothing

  ui_open_url,          // * open url
                        // Parameters: const char *url
                        // Returns: none

  ui_hexdumpea,         // * Return the current address in a hex view
                        // Parameters: ea_t *result
                        //             int hexdump_num
                        // Returns:    none

  ui_set_xml,           // * set/update one or more XML values. The 'name' element
                        // or attribute (use @XXX for an attribute) is created in
                        // all XML elements returned by the evaluation of the
                        // 'path' XPath expression, and receives the given 'value'.
                        // If 'name' is empty, the returned elements or attributes
                        // are directly updated to contain the new 'value'.
                        // Parameters: const char *path
                        //             const char *name
                        //             const char *value
                        // Returns:    bool

  ui_get_xml,           // * return an XML value by evaluating the 'path' XPath
                        // expression.
                        // Parameters: const char *path
                        //             value_t *value
                        // Returns:    bool

  ui_del_xml,           // * delete XML values corresponding to the evaluation of the
                        // 'path' XPath expression.
                        // Parameters: const char *path
                        // Returns:    bool

  ui_push_xml,          // * push an XML element on a stack whose uppermost element will be
                        // used to evaluate future relative XPath expressions.
                        // Parameters: const char *path
                        // Returns:    bool

  ui_pop_xml,           // * pop the uppermost XML element from the stack.
                        // Parameters: none
                        // Returns:    bool

  ui_get_key_code,      // * get keyboard key code by its name
                        // Parameters: const char *keyname
                        // Returns:    short code

  ui_setup_plugins_menu,// * setup plugins submenu
                        // Parameters: none
                        // Returns:    none

  ui_refresh_navband,   // * refresh navigation band if changed
                        // Parameters: bool force
                        // Returns:    none

  ui_new_custom_viewer, // * new ida viewer
                        // Parameters: undocumented

  ui_add_menu_item,     // * add a menu item
                        // Parameters: const char *menupath,
                        //             const char *name,
                        //             const char *hotkey,
                        //             int flags,
#define SETMENU_INS 0x0000      // add menu item before the specified path
#define SETMENU_APP 0x0001      // add menu item after the specified path
                        //             menu_item_callback_t *callback,
                        //             void *ud
                        // Returns:    bool

  ui_del_menu_item,     // * del a menu item
                        // Parameters: const char *menupath
                        // Returns:    bool

  ui_debugger_menu_change, // cb: debugger menu modification detected
                        // Parameters: bool enable
                        // enable=true: debugger menu has been added
                        // enable=false: debugger menu will be removed

  ui_get_curplace,      // * Get current place in the specified viewer
                        // Parameters: undocumented

  ui_create_tform,      // * create a new tform (only gui vcl version)
                        // Parameters: const char *caption
                        //             HWND *handle
                        // If you need the handle of the new window,
                        // you can get it using the 'handle' parameter.
                        // You will need it if you do not use VCL.
                        // If a window with the specified caption exists, return
                        // a pointer to it. 'handle' will be NULL is this case.
                        // Returns: TForm * of a new or existing window
                        // The text version always returns NULL

  ui_open_tform,        // * open tform (only gui vcl version)
                        // Parameters: TForm *form
                        //             int options
#define FORM_MDI      0x01 // start by default as MDI
#define FORM_TAB      0x02 // attached by default to a tab
#define FORM_RESTORE  0x04 // restore state from desktop config
#define FORM_ONTOP    0x08 // form should be ontop
#define FORM_MENU     0x10 // form must be listed in the windows menu
                           // (automatically set for all plugins)
#define FORM_CENTERED 0x20 // form will be centered on the screen
                        // Returns: nothing

  ui_close_tform,       // * close tform (only gui vcl version)
                        // Parameters: TForm *form
                        //             int options
#define FORM_SAVE           0x1 // save state in desktop config
#define FORM_NO_CONTEXT     0x2 // don't change the current context (useful for toolbars)
#define FORM_DONT_SAVE_SIZE 0x4 // don't save size of the window
                        // Returns: nothing

  ui_switchto_tform,    // * activate tform (only gui vcl version)
                        // Parameters: TForm *form
                        //             bool take_focus
                        // Returns: nothing

  ui_find_tform,        // * find tform with the specified caption  (only gui vcl version)
                        // Parameters: const char *caption
                        // Returns: TFrom *
                        // NB: this callback works only with the tabbed forms!

  ui_get_current_tform, // * get current tform (only gui vcl version)
                        // Parameters: none
                        // Returns: TFrom *
                        // NB: this callback works only with the tabbed forms!

  ui_last,              // The last notification code

  // debugger callgates. should not be used directly, see dbg.hpp for details

  ui_dbg_begin = 1000,
  ui_dbg_run_requests = ui_dbg_begin,
  ui_dbg_get_running_request,
  ui_dbg_get_running_notification,
  ui_dbg_clear_requests_queue,
  ui_dbg_get_process_state,
  ui_dbg_start_process,
  ui_dbg_request_start_process,
  ui_dbg_suspend_process,
  ui_dbg_request_suspend_process,
  ui_dbg_continue_process,
  ui_dbg_request_continue_process,
  ui_dbg_exit_process,
  ui_dbg_request_exit_process,
  ui_dbg_get_thread_qty,
  ui_dbg_getn_thread,
  ui_dbg_select_thread,
  ui_dbg_request_select_thread,
  ui_dbg_step_into,
  ui_dbg_request_step_into,
  ui_dbg_step_over,
  ui_dbg_request_step_over,
  ui_dbg_run_to,
  ui_dbg_request_run_to,
  ui_dbg_step_until_ret,
  ui_dbg_request_step_until_ret,
  ui_dbg_get_reg_val,
  ui_dbg_set_reg_val,
  ui_dbg_request_set_reg_val,
  ui_dbg_get_bpt_qty,
  ui_dbg_getn_bpt,
  ui_dbg_get_bpt,
  ui_dbg_add_bpt,
  ui_dbg_request_add_bpt,
  ui_dbg_del_bpt,
  ui_dbg_request_del_bpt,
  ui_dbg_update_bpt,
  ui_dbg_enable_bpt,
  ui_dbg_request_enable_bpt,
  ui_dbg_set_trace_size,
  ui_dbg_clear_trace,
  ui_dbg_request_clear_trace,
  ui_dbg_is_step_trace_enabled,
  ui_dbg_enable_step_trace,
  ui_dbg_request_enable_step_trace,
  ui_dbg_get_step_trace_options,
  ui_dbg_set_step_trace_options,
  ui_dbg_request_set_step_trace_options,
  ui_dbg_is_insn_trace_enabled,
  ui_dbg_enable_insn_trace,
  ui_dbg_request_enable_insn_trace,
  ui_dbg_get_insn_trace_options,
  ui_dbg_set_insn_trace_options,
  ui_dbg_request_set_insn_trace_options,
  ui_dbg_is_func_trace_enabled,
  ui_dbg_enable_func_trace,
  ui_dbg_request_enable_func_trace,
  ui_dbg_get_func_trace_options,
  ui_dbg_set_func_trace_options,
  ui_dbg_request_set_func_trace_options,
  ui_dbg_get_tev_qty,
  ui_dbg_get_tev_info,
  ui_dbg_get_insn_tev_reg_val,
  ui_dbg_get_insn_tev_reg_result,
  ui_dbg_get_call_tev_callee,
  ui_dbg_get_ret_tev_return,
  ui_dbg_get_bpt_tev_ea,
  ui_dbg_is_reg_integer,
  ui_dbg_get_process_qty,
  ui_dbg_get_process_info,
  ui_dbg_attach_process,
  ui_dbg_request_attach_process,
  ui_dbg_detach_process,
  ui_dbg_request_detach_process,
  ui_dbg_get_first_module,
  ui_dbg_get_next_module,
  ui_dbg_bring_to_front,

  ui_dbg_end,

};


//--------------------------------------------------------------------------
//
//      The user interface must call init_kernel before calling
//      any kernel functions. It should pass a function named
//      CALLUI to init_kernel. The CALLUI function is a dispatcher
//      The user interface should have it.
//

idaman void ida_export init_kernel(
                  callui_t (idaapi*_callui)(ui_notification_t what,...),
                  int argc,
                  char *argv[]);

// Pointer to the user-interface dispatcher function
// This pointer is in the kernel
idaman callui_t ida_export_data (idaapi*callui)(ui_notification_t what,...);



// After calling init_kernel the ui must call init_database()
// This function will open the database specified in the command line
// If the database did not exist, a new database will be created and
// the input file will be loaded
// Returns: 0-ok, otherwise an exit code

idaman int ida_export init_database(int argc, char *argv[], int *newfile);


// The kernel termination function
// This function should be called to close the database

idaman void ida_export term_database(void);


// The kernel termination function
// This function should be called when IDA is going to exit

idaman void ida_export term_kernel(void);


// Display error message. Use error()

idaman void ida_export verror(const char *message, va_list va);


// Display hex dump in the messages window

idaman void ida_export vshow_hex(const void *dataptr,size_t len,const char *fmt, va_list va);


// Display hex dump of a file in the messages window

idaman void ida_export vshow_hex_file(linput_t *li, long pos, size_t count, const char *format, va_list va);


//--------------------------------------------------------------------------
//      K E R N E L   S E R V I C E S   F O R   U I
//--------------------------------------------------------------------------

//
//      The ui specific kernel services consist of several classes:
//
//              linearray_t
//
//              idaplace_t
//              enumplace_t
//              structplace_t
//              microplace_t
//
//      The classes provide means to access to the text generated by
//      the kernel for the disassembly window, enum and structure windows.
//

//      An abstract class which works with lines on the screen
//              generate/position/order
//      You should subclass it to make some real work.
//

class place_t                   // a place pointer
{
public:
  short lnnum;
  place_t(void) {}
  place_t(short ln) : lnnum(ln) {}
  virtual void idaapi print(void *ud,char *buf, size_t bufsize) const  = 0;
  virtual uval_t idaapi touval(void *ud) const                         = 0;
  virtual place_t *idaapi clone(void) const                            = 0;
  virtual void idaapi copyfrom(const place_t *from)                    = 0; // copy everything
  virtual place_t *idaapi makeplace(void *ud,uval_t x,short lnnum) const= 0; // returns ptr to static storage
  virtual int idaapi compare(const place_t *t2) const                  = 0; // compare everything except lnnum
  virtual void idaapi adjust(void *ud)                                 = 0;
  virtual bool idaapi prev(void *ud)                                   = 0;
  virtual bool idaapi next(void *ud)                                   = 0;
  virtual bool idaapi beginning(void *ud) const                        = 0;
  virtual bool idaapi ending(void *ud) const                           = 0;
  virtual int idaapi generate(void *ud,
                              char *lines[],
                              int maxsize,
                              int *default_lnnum,
                              color_t *pfx_color,
                              bgcolor_t *bgcolor) const                = 0;

};

// compare places and their lnnums
idaman int ida_export l_compare(const place_t *t1, const place_t *t2);

//--------------------------------------------------------------------------
#define define_place_exported_functions(classname)                                  \
class classname;                                                                    \
idaman void     ida_export classname ## __print(const classname *,void*,char*, size_t);    \
idaman uval_t   ida_export classname ## __touval(const classname *,void*);                 \
idaman place_t *ida_export classname ## __clone(const classname *);                        \
idaman void     ida_export classname ## __copyfrom(classname *,const place_t*);            \
idaman place_t *ida_export classname ## __makeplace(const classname *,void*,uval_t,short); \
idaman int      ida_export classname ## __compare(const classname *,const place_t*);       \
idaman void     ida_export classname ## __adjust(classname *,void*);                       \
idaman bool     ida_export classname ## __prev(classname *,void*);                         \
idaman bool     ida_export classname ## __next(classname *,void*);                         \
idaman bool     ida_export classname ## __beginning(const classname *,void*);              \
idaman bool     ida_export classname ## __ending(const classname *,void*);                 \
idaman int      ida_export classname ## __generate(const classname *,void*,char**,int,int*,color_t*,bgcolor_t*);

#define define_place_virtual_functions(class)                           \
  void idaapi print(void *ud,char *buf, size_t bufsize) const           \
        {        class ## __print(this,ud,buf,bufsize); }               \
  uval_t idaapi touval(void *ud) const                                  \
        { return class ## __touval(this,ud); }                          \
  place_t *idaapi clone(void) const                                     \
        { return class ## __clone(this); }                              \
  void idaapi copyfrom(const place_t *from)                             \
        {        class ## __copyfrom(this,from); }                      \
  place_t *idaapi makeplace(void *ud,uval_t x,short lnnum) const        \
        { return class ## __makeplace(this,ud,x,lnnum); }               \
  int  idaapi compare(const place_t *t2) const                          \
        { return class ## __compare(this,t2); }                         \
  void idaapi adjust(void *ud)                                          \
        {        class ## __adjust(this,ud); }                          \
  bool idaapi prev(void *ud)                                            \
        { return class ## __prev(this,ud); }                            \
  bool idaapi next(void *ud)                                            \
        { return class ## __next(this,ud); }                            \
  bool idaapi beginning(void *ud) const                                 \
        { return class ## __beginning(this,ud); }                       \
  bool idaapi ending (void *ud) const                                   \
        { return class ## __ending(this,ud); }                          \
  int idaapi generate (void *ud,char *lines[],int maxsize,int *lnnum,   \
                       color_t *pfx_color, bgcolor_t *bg_color) const   \
        { return class ## __generate(this,ud,lines,maxsize,lnnum,       \
                                                pfx_color, bg_color); }

//--------------------------------------------------------------------------
define_place_exported_functions(microplace_t)
class microplace_t : public place_t {             // a place pointer
public:
  int block;    // basic block number
  int n;        // microinstruction number
  microplace_t(void) {}
  microplace_t(int b, int _n) : block(b), n(_n), place_t(0) {}
  define_place_virtual_functions(microplace_t);
};

//--------------------------------------------------------------------------
// user defined data for linearray_t: int *flag
// *flag value is a combination of:
#define IDAPLACE_HEXDUMP 0x000F  // produce hex dump
#define IDAPLACE_STACK   0x0010  // produce 2/4/8 bytes per undefined item
                                 // (used to display the stack contents)
                                 // the number of displayed bytes depends on the stack bitness
// not used yet because it confuses users:
//#define IDAPLACE_SHOWPRF 0x0020  // display line prefixes
#define IDAPLACE_SEGADDR 0x0040  // display line prefixes with the segment part

#ifdef _IDA_HPP
inline int calc_default_idaplace_flags(void)
{
  int flags = 0;
//  if ( inf.s_showpref ) flags |= IDAPLACE_SHOWPRF;
  if ( inf.s_prefflag & PREF_SEGADR ) flags |= IDAPLACE_SEGADDR;
  return flags;
}
#endif // ifdef _IDA_HPP

define_place_exported_functions(idaplace_t)
class idaplace_t : public place_t {             // a place pointer
public:
  ea_t ea;
  idaplace_t(void) {}
  idaplace_t(ea_t x,short ln) : ea(x), place_t(ln) {}
  define_place_virtual_functions(idaplace_t);
};

//--------------------------------------------------------------------------
// user defined data for linearray_t: NULL
define_place_exported_functions(enumplace_t)
class enumplace_t : public place_t {            // a place pointer
public:
  size_t idx;
  bmask_t bmask;
  uval_t value;
  uchar serial;
  enumplace_t(void) {}
  enumplace_t(size_t i, bmask_t m, uval_t v, uchar s, short ln)
    : idx(i), bmask(m), value(v), serial(s), place_t(ln) {}
  define_place_virtual_functions(enumplace_t);
};

//--------------------------------------------------------------------------
// user defined data for linearray_t: ea_t *pea
// if pea != NULL then the function stack frame is displayed, *pea == function start
// else                normal structure list is displayed
define_place_exported_functions(structplace_t)
class structplace_t : public place_t {          // a place pointer
public:
  uval_t idx;
  uval_t offset;
  structplace_t(void) {}
  structplace_t(uval_t i, uval_t o, short ln) : idx(i), offset(o), place_t(ln) {}
  define_place_virtual_functions(structplace_t);
};

struct saved_structplace_t                      // the saved position in the database
{
  ushort lnnum;
  ushort x,y;
  uval_t idx;
  uval_t offset;
};

//----------------------------------------------------------------------
class twinpos_t                 // a pointer in a text window
{
public:
  place_t *at;
  int x;
  twinpos_t(void)                 {}
  twinpos_t(place_t *t)           { at=t; }
  twinpos_t(place_t *t,int x0)    { at=t; x=x0; }
  bool operator != (const twinpos_t &r)
  {
    if ( x != r.x ) return true;
    if ( (at == NULL) != (r.at == NULL) ) return true;
    if ( at != NULL && (at->compare(r.at) != 0 || at->lnnum != r.at->lnnum) ) return true;
    return false;
  }
  bool operator == (const twinpos_t &r) { return !(*this != r); }
};

class twinline_t                // a line in a text window
{
public:
  place_t *at;
  char *line;
  color_t prefix_color;
  bgcolor_t bg_color;
  bool is_default;		// is the default line of the current location?
  twinline_t(void) {}
  twinline_t(place_t *t,char *l,color_t pc, bgcolor_t bc)
  {
    at           = t;
    line         = l;
    prefix_color = pc;
    bg_color     = bc;
    is_default   = false;
  }
};

//----------------------------------------------------------------------
// Internal class, not documented
idaman int ida_export_data lnar_size; // Maximum number of lines for one item (one instruction or data)


#define DECLARE_LINEARRAY_HELPERS(decl) \
decl void  ida_export linearray_t_ctr      (linearray_t *, void *ud); \
decl void  ida_export linearray_t_dtr      (linearray_t *); \
decl void  ida_export linearray_t_set_place(linearray_t *, place_t *new_at); \
decl bool  ida_export linearray_t_beginning(linearray_t *); \
decl bool  ida_export linearray_t_ending   (linearray_t *); \
decl char *ida_export linearray_t_down     (linearray_t *); \
decl char *ida_export linearray_t_up       (linearray_t *);

class linearray_t;
DECLARE_LINEARRAY_HELPERS(idaman)

class linearray_t
{
  DECLARE_LINEARRAY_HELPERS(friend)
  void  _set_place(place_t *new_at);
  char *_down     (void);
  char *_up       (void);

  char **lines;                 // [lnar_size]; we own all these lines
  place_t *at;
  void *ud;                     // user defined data (UD)
                                // its meaning depends on the place_t used
  int linecnt;                  // number of lines
  color_t prefix_color;         // prefix color
  bgcolor_t bg_color;           // background color
  char *extra;			// the last line of the prevois location after moving down
  int dlnnum;			// default line number (if unknown, -1)

  void  getlines        (void);
  void  cleanup         (void);

public:

  linearray_t(void *ud)                      { linearray_t_ctr(this, ud); }
  ~linearray_t(void)                         { linearray_t_dtr(this); }

// this function must be called before calling any other member functions.
// it positions the array. linearray_t doesn't own place_t structures.
// the caller must take care of place_t objects.
  void set_place(place_t *new_at)            { linearray_t_set_place(this, new_at); }

// return the current place
// if called before down(), then returns place of line which will be returned by down()
// if called after up(), then returns place if line returned by up()
  place_t* get_place    (void) const         { return at; }

// get current color
// (the same behaviour as with get_place: good before down() and after up())
  bgcolor_t get_bg_color(void) const         { return bg_color; }
  bgcolor_t get_pfx_color(void) const        { return prefix_color; }

// get default line number
// (the same behaviour as with get_place: good before down() and after up())
  int   get_dlnnum      (void) const         { return dlnnum; }

// get number of lines for the current place
// (the same behaviour as with get_place: good before down() and after up())
  int   get_linecnt     (void) const         { return linecnt; }

// return pointer to user data
  void *userdata        (void) const         { return ud; }
// change the user data
  void set_userdata     (void *userd)        { ud = userd; }

// 1 if we are at the beginning
  bool beginning(void)                       { return linearray_t_beginning(this); }

// 1 if we are at the end
  bool ending(void)                          { return linearray_t_ending(this); }

// get a line from up/down directions
  char *down(void)         // place is ok BEFORE
        { return linearray_t_down(this); }
  char *up(void)           // place is ok AFTER
        { return linearray_t_up(this); }

};


//---------------------------------------------------------------------------
//      D E B U G G I N G   F U N C T I O N S
//---------------------------------------------------------------------------

idaman int ida_export_data debug;

#define IDA_DEBUG_DREFS         0x00000001      // drefs
#define IDA_DEBUG_OFFSET        0x00000002      // offsets
#define IDA_DEBUG_FLIRT         0x00000004      // flirt
#define IDA_DEBUG_IDP           0x00000008      // idp module
#define IDA_DEBUG_LDR           0x00000010      // ldr module
#define IDA_DEBUG_PLUGIN        0x00000020      // plugin module
#define IDA_DEBUG_IDS           0x00000040      // ids files
#define IDA_DEBUG_CONFIG        0x00000080      // config file
#define IDA_DEBUG_CHECKMEM      0x00000100      // check heap consistency
#define IDA_DEBUG_CHECKARG      0x00000200      // checkarg
#define IDA_DEBUG_DEMANGLE      0x00000400      // demangler
#define IDA_DEBUG_QUEUE         0x00000800      // queue
#define IDA_DEBUG_ROLLBACK      0x00001000      // rollback
#define IDA_DEBUG_ALREADY       0x00002000      // already data or code
#define IDA_DEBUG_TIL           0x00004000      // type system
#define IDA_DEBUG_NOTIFY        0x00008000      // show all notifications
#define IDA_DEBUG_DEBUGGER      0x00010000      // debugger
#define IDA_DEBUG_ALWAYS        0xFFFFFFFF      // everything

// to display debug messages use deb() function:

inline int deb(int when,              // IDA_DEBUG_...
                const char *format,
                ...)
{
  int nbytes = 0;
  if ( debug & when )
  {
    va_list va;
    va_start(va, format);
    nbytes = callui(ui_msg, format, va).i;
    va_end(va);
  }
  return nbytes;
}

// print the tick count from the last call to debug_time()
// the first time prints -1
#ifdef __NT__
#define debug_time()  ida_debug_time(__FILE__, __LINE__)
void ida_debug_time(const char *file, int line);
#else
#define debug_time()
#endif


// Check heap.
// If heap is corrupted, display a message, wait for a key and exit.

#define checkmem() ida_checkmem(__FILE__, __LINE__)

idaman void ida_export ida_checkmem(const char *file, int line);

// A macro for source text conditional debugging:

#ifndef DEBUG
#define debug(x)
#else
#define debug(x)  msg x
#endif


inline void show_hex(const void *dataptr,size_t len,const char *fmt,...)
{
  va_list va;
  va_start(va,fmt);
  vshow_hex(dataptr, len, fmt, va);
  va_end(va);
}

inline void show_hex_file(linput_t *li, long pos, size_t count,const char *fmt, ...)
{
  va_list va;
  va_start(va,fmt);
  vshow_hex_file(li, pos, count, fmt, va);
  va_end(va);
}

//-------------------------------------------------------------------------
//      U I   S E R V I C E  F U N C T I O N S
//-------------------------------------------------------------------------

// Common function prototypes
// These functions are inlined for the kernel
// They are not inlined for the user-interfaces

int askbuttons_cv(const char *Yes, const char *No, const char *Cancel,
                                int deflt, const char *format, va_list va);
char *vaskstr(int hist,const char *defval,const char *format,va_list va);
char *vasktext(size_t size,
               char *answer,
               const char *defval,
               const char *format,
               va_list va);
int vmsg(const char *format, va_list va);
void vwarning(const char *message, va_list va);
void vinfo(const char *message, va_list va);
void vnomem(const char *module, va_list va);


// generic chooser flags
#define CH_MODAL 0x01 // Modal chooser
#define CH_MULTI 0x02 // The chooser will allow multi-selection (only for GUI choosers).
                      // If multi-selection is enabled, a multi-selection 'del' callback
                      // will be called with:
                      //   ulong = START_SEL   before calling the first selected item
                      //   ulong =  1..n       for each selected item
                      //   ulong = END_SEL     after calling the last  selected item
                      // If the callback returns 'false', further processing
                      //   of selected items is cancelled.
#define CH_MULTI_EDIT 0x04
                      // The 'edit' callback will be called for all selected items
                      // using the START_SEL/END_SEL protocol.
                      // This bit implies CH_MULTI (valid only for gui)

// chooser multi-selection callback special events (passed as indexes to callbacks)
typedef long chooser_event_t;
const chooser_event_t
  EMPTY_SEL =  0, // no item was selected
  START_SEL = -1, // before calling the first selected item
  END_SEL   = -2; // after calling the last  selected item
// chooser multi-selection callback convenience macros
#define IS_EMPTY_SEL(n)     ((chooser_event_t)n == EMPTY_SEL)
#define IS_START_SEL(n)     ((chooser_event_t)n == START_SEL)
#define IS_END_SEL(n)       ((chooser_event_t)n == END_SEL)
#define IS_CHOOSER_EVENT(n) (IS_EMPTY_SEL(n) \
                          || IS_START_SEL(n) \
                          || IS_END_SEL(n))
#define IS_SEL(n)           (!IS_CHOOSER_EVENT(n))

// Generic list choosers. These functions display a window with a list and allow
// the user to select an item from the list.

// Generic list chooser (1-column)
//      Returns: -1   - the chooser was already open and is now active (only for non-modal choosers)
//               0    - the chooser was created but the user refused to choose anything
//               else - number of them selected item
ulong choose(
        int flags,                      // various flags: see above for description
        int x0,int y0,                  // x0=-1 for autoposition
        int x1,int y1,
        void *obj,                      // object to show
        int width,                      // Max width of lines
        ulong (idaapi*sizer)(void *obj),      // Number of items
        char *(idaapi*getl)(void *obj,ulong n,char *buf),// Description of n-th item (1..n)
                                        // 0-th item if header line
        const char *title,              // menu title (includes ptr to help)
        int icon,                       // number of the default icon to display
        ulong deflt=1,                  // starting item
        ulong (idaapi*del)(void *obj,ulong n)=NULL,   // callback for "Delete" (may be NULL)
                                                      // supports multi-selection scenario too
                                                      // returns: 1-ok, 0-failed
        void (idaapi*ins)(void *obj)=NULL,            // callback for "New" (may be NULL)
        ulong (idaapi*update)(void *obj,ulong n)=NULL,// callback for "Update"(may be NULL)
                                                      // update the whole list
                                                      // returns the new location of item 'n'
        void (idaapi*edit)(void *obj,ulong n)=NULL,   // callback for "Edit" (may be NULL)
        void (idaapi*enter)(void * obj,ulong n)=NULL, // callback for non-modal "Enter" (may be NULL)
        void (idaapi*destroy)(void *obj)=NULL,        // callback to call when the window is closed (may be NULL)
        const char * const *popup_names=NULL,         // Default: insert, delete, edit, refresh
        int (idaapi*get_icon)(void *obj,ulong n)=NULL); // callback for get_icon (may be NULL)


// convenience function: modal chooser
inline ulong choose(void *obj,
                    int width,
                    ulong (idaapi*sizer)(void *obj),
                    char *(idaapi*getl)(void *obj,ulong n,char *buf),
                    const char *title,
                    int icon=-1,
                    ulong deflt=1,
                    ulong (idaapi*del)(void *obj,ulong n)=NULL,
                    void (idaapi*ins)(void *obj)=NULL,
                    ulong (idaapi*update)(void *obj,ulong n)=NULL,
                    void (idaapi*edit)(void *obj,ulong n)=NULL,
                    void (idaapi*enter)(void * obj,ulong n)=NULL,
                    void (idaapi*destroy)(void *obj)=NULL,
                    const char * const *popup_names=NULL,
                    int (idaapi*get_icon)(void *obj,ulong n)=NULL)
{
  return choose(CH_MODAL,-1,-1,-1,-1, obj, width, sizer,
                getl, title, icon, deflt, del, ins,
                update, edit, enter, destroy, popup_names, get_icon);
}

// Generic list chooser (n-column)
// See choose() above for the description of the undescribed parameters
ulong choose2(
        int flags,
        int x0,int y0,                  // x0=-1 for autoposition
        int x1,int y1,
        void *obj,                      // object to show
        int ncol,                       // Number of columns
        const int *widths,              // Widths of columns (may be NULL)
        ulong (idaapi*sizer)(void *obj),
        void (idaapi*getl)(void *obj,ulong n,char * const *arrptr),
        const char *title,
        int icon,
        ulong deflt=1,
        ulong (idaapi*del)(void *obj,ulong n)=NULL,
        void (idaapi*ins)(void *obj)=NULL,
        ulong (idaapi*update)(void *obj,ulong n)=NULL,
        void (idaapi*edit)(void *obj,ulong n)=NULL,
        void (idaapi*enter)(void * obj,ulong n)=NULL,
        void (idaapi*destroy)(void *obj)=NULL,
        const char * const *popup_names=NULL,
        int (idaapi*get_icon)(void *obj,ulong n)=NULL);


// convenience function: modal chooser2
inline ulong choose2(void *obj,
                     int ncol,
                     const int *widths,
                     ulong (idaapi*sizer)(void *),
                     void (idaapi*getl)(void *,ulong,char*const*),
                     const char *title,
                     int icon=-1,
                     ulong deflt=1,
                     ulong (idaapi*del)(void *,ulong)=NULL,
                     void (idaapi*ins)(void *)=NULL,
                     ulong (idaapi*update)(void *,ulong)=NULL,
                     void (idaapi*edit)(void *,ulong)=NULL,
                     void (idaapi*enter)(void *,ulong)=NULL,
                     void (idaapi*destroy)(void *)=NULL,
                     const char * const *popup_names=NULL,
                     int (idaapi*get_icon)(void *obj,ulong n)=NULL)
{
  return choose2(CH_MODAL,-1,-1,-1,-1, obj, ncol, widths,
                 sizer, getl, title, icon, deflt, del,
                 ins, update, edit, enter, destroy, popup_names, get_icon);
}

// Callback function for menu commands
// If returns true, IDA will refresh the disassembly view and the list contents

typedef bool idaapi menu_item_callback_t(void *ud);


#ifndef __UI__         // Not for the UI

// Convenience functions offered by the user interface

inline bool jumpto(ea_t ea,int opnum=-1)   { return callui(ui_jumpto, ea, opnum).cnd; }
inline bool banner(int wait)               { return callui(ui_banner, wait).cnd;}
inline bool is_msg_inited(void)            { return callui(ui_is_msg_inited).cnd; }
inline void refresh_idaview(void)          { callui(ui_refreshmarked);}
inline void refresh_idaview_anyway(void)   { callui(ui_refresh);      }
inline void analyzer_options(void)         { callui(ui_analyzer_options); }
inline ea_t get_screen_ea(void)            { ea_t ea; callui(ui_screenea, &ea); return ea; }
inline bool get_cursor(int *x, int *y)     { return callui(ui_get_cursor, x, y).cnd; }
inline char *get_curline(void)             { return callui(ui_get_curline).cptr; }
inline bool read_selection(ea_t *ea1, ea_t *ea2) { return callui(ui_readsel, ea1, ea2).cnd; }
inline void unmark_selection(void)         { callui(ui_unmarksel); }
inline void open_url(const char *url)      { callui(ui_open_url, url); }
inline ea_t get_hexdump_ea(int hexdump_num) { ea_t ea; callui(ui_hexdumpea, &ea, hexdump_num); return ea; }
inline bool set_xml(const char *path, const char *name, const char *value) { return callui(ui_set_xml, path, name, value).cnd; }
inline bool get_xml(const char *path, value_t *value) { return callui(ui_get_xml, path, value).cnd; }
inline bool del_xml(const char *path)      { return callui(ui_del_xml, path).cnd; }
inline bool push_xml(const char *path)     { return callui(ui_push_xml, path).cnd; }
inline bool pop_xml(void)                  { return callui(ui_pop_xml).cnd; }
inline ushort get_key_code(const char *keyname) { return callui(ui_get_key_code, keyname).i16; }
inline void refresh_navband(bool force)     { callui(ui_refresh_navband, force); }
inline bool close_chooser(const char *title) { return callui(ui_close_chooser, title).cnd; }

// Add a menu item
//      menupath - path to the menu item after or before which the insertion will
//                 take place. Example: Debug/StartProcess
//                 Whitespace, punctuation are ignored.
//                 It is allowed to specify only the prefix of the menu item.
//                 Comparision is case insensitive.
//      name     - name of the menu item (~x~ is used to denote Alt-x hot letter)
//      hotkey   - hotkey for the menu item. may be NULL.
//      flags    - one of SETMENU_... consts
//      callback - function which gets called when the user selects it
//                 if the callback returns true, the screen will be refreshed
//      ud       - user data for the callback function
// If you want to modify the debugger menu, do it at the ui_debugger_menu_change
// event (ida might destroy your menu item if you do it elsewhere).
// You should not change the Edit, Plugins submenu.
// You should not modify the top level menu.

inline bool add_menu_item(
        const char *menupath,
        const char *name,
        const char *hotkey,
        int flags,
        menu_item_callback_t *callback,
        void *ud)
{
  return callui(ui_add_menu_item, menupath, name, hotkey, flags, callback, ud).cnd;
}


// Delete a menu item
//      menupath - path to the menu item. Example: Debug/Reset

inline bool del_menu_item(const char *menupath) { return callui(ui_del_menu_item, menupath).cnd; }


// Get VCL global variables

#if defined(__BORLANDC__)
namespace Forms
{
  class TApplication;
  class TScreen;
}

namespace Controls
{
  class TMouse;
};

inline size_t getvcl(Forms::TApplication **app,
                     Forms::TScreen **screen,
                     Controls::TMouse **mouse)
{
  return callui(ui_getvcl, app, screen, mouse).i;
}

#endif // __BORLANDC__

#ifdef _WINDOWS_
namespace Forms
{
  class TForm;
};
using Forms::TForm;

inline TForm *create_tform(const char *caption, HWND *handle)
{
  return (TForm *)callui(ui_create_tform, caption, handle).vptr;
}

inline void open_tform(TForm *form, int options)
{
  callui(ui_open_tform, form, options);
}

inline void close_tform(TForm *form, int options)
{
  callui(ui_close_tform, form, options);
}

inline void switchto_tform(TForm *form, bool take_focus)
{
  callui(ui_switchto_tform, form, take_focus);
}

inline TForm *find_tform(const char *caption)
{
  return (TForm *)callui(ui_find_tform, caption).vptr;
}

inline TForm *get_current_tform(void)
{
  return (TForm *)callui(ui_get_current_tform).vptr;
}

#endif


// Clear Ctrl-Break flag.

inline void clearBreak(void) { callui(ui_clearbreak); }


// Set Ctrl-Break flag.

inline void setBreak(void) { callui(ui_setbreak); }


// Check for Ctrl-Break.
// Returns: 1 - Ctrl-Break is detected, a message "Command cancelled" is displayed
//          2 - Ctrl-Break is detected again, a message is not displayed
//          0 - Ctrl-Break is not detected

inline int wasBreak(void) { return callui(ui_wasbreak).cnd; }


// Change autoanalysis indicator value
//      ea - linear address being analyzed
//      type - autoanalysis type

inline void showAuto(ea_t ea,atype_t type=0)
{
  callui(ui_showauto, ea, type);
}


// Show an address on the autoanalysis indicator
//      ea - linear address to display
// The address is displayed in the form " @:12345678"

inline void showAddr(ea_t ea) { showAuto(ea); }


// Change IDA status indicator value
//      st - new indicator status
// returns old indicator status

inline idastate_t setStat(idastate_t st)
{
  return callui(ui_setstate, st).i;
}


inline bool ui_load_new_file(const char *filename,
                             linput_t *li,
                             ushort neflags)
{
  return callui(ui_load_file, filename, li, neflags).cnd;
}


inline bool ui_run_debugger(const char *dbgopts,
                            const char *exename,
                            int argc,
                            char **argv)
{
  return callui(ui_run_dbg, dbgopts, exename, argc, argv).cnd;
}


// Add hotkey for IDC function
//      hotkey  - hotkey name
//      idcfunc - IDC function name
// returns: IDCHK_.. error codes (see above)

inline int add_idc_hotkey(const char *hotkey,const char *idcfunc)
{
  return callui(ui_add_idckey, hotkey, idcfunc).i;
}


// Delete IDC function hotkey
//      hotkey  - hotkey name
// returns: 1-ok, 0-failed

inline bool del_idc_hotkey(const char *hotkey)
{
  return callui(ui_del_idckey, hotkey).cnd;
}


// Pointer to idaview marker function.
// This pointer is initialized by callui(ui_get_marker)

extern void (idaapi*idaview_marker)(ea_t ea);


// Initialize pointer to idaview marker

inline void setup_idaview_marker(void)
{
  void *ptr = callui(ui_get_marker).vptr;
  if ( ptr != NULL )
    idaview_marker = (void (idaapi*)(ea_t))ptr;
}


// Mark the view to refresh - disassembly
// at 'ea' is changed.

inline void mark_idaview_for_refresh(ea_t ea)
{
  if ( idaview_marker != NULL )
    idaview_marker(ea);
}

// Mark the view to refresh unconditionally

inline void mark_idaview_for_refresh_anyway(void)
{
  if ( idaview_marker != NULL )
    idaview_marker(get_screen_ea());
}


inline char *choose_idasgn(void)
{
  return callui(ui_choose, chtype_idasgn).cptr;
}

inline bool choose_til(char *buf, size_t bufsize)
{
  return callui(ui_choose, chtype_idatil, buf, bufsize).cnd;
}

inline ea_t choose_entry(const char *title)
{
  ea_t ea;
  callui(ui_choose, chtype_entry, &ea, title);
  return ea;
}

inline ea_t choose_name(const char *title)
{
  ea_t ea;
  callui(ui_choose, chtype_name, &ea, title);
  return ea;
}

inline ea_t choose_stkvar_xref(func_t *pfn, member_t *mptr)
{
  ea_t ea;
  callui(ui_choose, chtype_stkvar_xref, &ea, pfn, mptr);
  return ea;
}

inline ea_t choose_xref(ea_t to)
{
  ea_t ea;
  callui(ui_choose, chtype_xref, &ea, to);
  return ea;
}

inline enum_t choose_enum(const char *title, enum_t default_id)
{
  enum_t enum_id = default_id;
  callui(ui_choose, chtype_enum, &enum_id, title);
  return enum_id;
}

inline enum_t choose_enum_by_value(const char *title, enum_t default_id,
                                                uval_t value, uchar *serial)
{
  enum_t enum_id = default_id;
  callui(ui_choose, chtype_enum_by_value, &enum_id, title, value, serial);
  return enum_id;
}

inline func_t *choose_func(const char *title, ea_t default_ea)
{
  return callui(ui_choose, chtype_func, title, default_ea).fptr;
}

inline segment_t *choose_segm(const char *title, ea_t default_ea)
{
  return callui(ui_choose, chtype_segm, title, default_ea).segptr;
}

inline segreg_t *choose_segreg(const char *title)
{
  return callui(ui_choose, chtype_segreg, title).sregptr;
}

inline struc_t *choose_struc(const char *title)
{
  return callui(ui_choose, chtype_struc, title).strptr;
}

inline int choose_struc_path(const char *title, tid_t strid,
                      uval_t offset, adiff_t delta, bool appzero, tid_t *path)
{
  return callui(ui_choose, chtype_strpath, title, strid,
                                            offset, delta, appzero, path).i;
}

// Generic list chooser (1-column)
// See the description of this function above in this file
// (to find it, search for "list choosers")
inline ulong choose(
        int flags,
        int x0,int y0,
        int x1,int y1,
        void *obj,
        int width,
        ulong (idaapi*sizer)(void *obj),
        char *(idaapi*getl)(void *obj,ulong n,char *buf),
        const char *title,
        int icon,
        ulong deflt,
        ulong (idaapi*del)(void *obj,ulong n),
        void (idaapi*ins)(void *obj),
        ulong (idaapi*update)(void *obj,ulong n),
        void (idaapi*edit)(void *obj,ulong n),
        void (idaapi*enter)(void * obj,ulong n),
        void (idaapi*destroy)(void *obj),
        const char * const *popup_names,
        int (idaapi*get_icon)(void *obj,ulong n))
{
  return callui(ui_choose, chtype_generic, flags, x0, y0, x1, y1, obj, width,
                sizer, getl, title, icon, deflt, del, ins, update,
                edit, enter, destroy, popup_names, get_icon).i32;
}


// Generic list chooser (n-column)
// See the description of this function above in this file
// (to find it, search for "list choosers")
inline ulong choose2(
        int flags,
        int x0,int y0,
        int x1,int y1,
        void *obj,
        int ncol,
        const int *widths,
        ulong (idaapi*sizer)(void *obj),
        void (idaapi*getl)(void *obj,ulong n,char * const *arrptr),
        const char *title,
        int icon,
        ulong deflt,
        ulong (idaapi*del)(void *obj,ulong n),
        void (idaapi*ins)(void *obj),
        ulong (idaapi*update)(void *obj,ulong n),
        void (idaapi*edit)(void *obj,ulong n),
        void (idaapi*enter)(void * obj,ulong n),
        void (idaapi*destroy)(void *obj),
        const char * const *popup_names,
        int (idaapi*get_icon)(void *obj,ulong n))
{
  return callui(ui_choose, chtype_generic2, flags, x0, y0, x1, y1, obj, ncol,
                widths, sizer, getl, title, icon, deflt, del, ins,
                update, edit, enter, destroy, popup_names, get_icon).i32;
}


// Display a dialog box with "Please wait..."

inline void show_wait_box_v(const char *format,va_list va)
{
  callui(ui_mbox, mbox_wait, format, va);
}


inline void show_wait_box(const char *format,...)
{
  va_list va;
  va_start(va, format);
  show_wait_box_v(format, va);
  va_end(va);
}


// Hide the "Please wait dialog box"

inline void hide_wait_box(void)
{
  // stupid watcom requires va_list should not be NULL
  callui(ui_mbox, mbox_hide, NULL, &callui);
}


// See AskUsingForm_c() function

inline int AskUsingForm_cv(const char *format, va_list va)
{
  return callui(ui_form, format, va).cnd;
}


// A helper function for the complex dialog box (form) callbacks:

inline void close_form(TView *fields[],int is_ok)
{
  callui(ui_close_form, fields, is_ok);
}


// Issue a beeping sound.

inline void beep(beep_t beep_type=beep_default)
{
  callui(ui_beep, beep_type);
}


// See askbuttons_c() function

inline int askbuttons_cv(const char *Yes, const char *No, const char *Cancel,
                                   int deflt, const char *format, va_list va)
{
  return callui(ui_askyn, Yes, No, Cancel, deflt, format, va).i;
}


// See askfile_c() function

inline char *askfile_cv(int savefile,
                        const char *defval,
                        const char *format,
                        va_list va)
{
  return callui(ui_askfile, savefile, defval, format, va).cptr;
}

// Show a message box asking to send the input file to support@datarescue.com
//      format - the reason why the input file is bad

inline void ask_for_feedback(const char *format,...)
{
  va_list va;
  va_start(va, format);
  callui(ui_mbox, mbox_feedback, format, va);
  va_end(va);
}


// Display a dialog box and wait for the user to input an identifier
//      defval  - default value. will be displayed initially in the input line.
//                may be NULL.
//      format  - number of message from IDA.HLP to display as question
//                in printf() style format
// If the user enters a non-valid identifier, this function displays a warning
// and allows to correct it.
// returns: NULL-if the user pressed Esc.
//          otherwise returns pointer to identifier.

inline char *askident(const char *defval,const char *format,...)
{
  va_list va;
  va_start(va, format);
  char *result = callui(ui_askident, defval, format, va).cptr;
  va_end(va);
  return result;
}


// Display a dialog box and wait for the user to input an address
//      addr    - in/out parameter. contains pointer to the address.
//      format  - printf() style format string with the question
// returns: 0-if the user pressed Esc.
//          1-ok, the user entered an address

inline int askaddr(ea_t *addr,const char *format,...)
{
  va_list va;
  va_start(va, format);
  bool cnd = callui(ui_askaddr, addr, format, va).cnd;
  va_end(va);
  return cnd;
}


// Display a dialog box and wait for the user to input an segment name
//      sel     - in/out parameter. contains selector of the segment
//      format  - printf() style format string with the question
// this function allows to enter segment register names, segment base
// paragraphs, segment names to denote a segment.
// returns: 0-if the user pressed Esc.
//          1-ok, the user entered an segment name

inline int askseg(sel_t *sel,const char *format,...)
{
  va_list va;
  va_start(va, format);
  bool code = callui(ui_askseg, sel, format, va).cnd;
  va_end(va);
  return code;
}


// Display a dialog box and wait for the user to input an number
//      value   - in/out parameter. contains pointer to the number
//      format  - printf() style format string with the question
// The number is represented in C-style.
// Actually this function allows to enter any IDC expression and
// properly calculates it.
// returns: 0-if the user pressed Esc.
//          1-ok, the user entered a valid number.

inline int asklong(sval_t *value, const char *format, ...)
{
  va_list va;
  va_start(va, format);
  bool code = callui(ui_asklong, value, format, va).cnd;
  va_end(va);
  return code;
}

inline char *vaskstr(int hist,const char *defval,const char *format,va_list va)
{
  return callui(ui_askstr, hist, defval, format, va).cptr;
}

inline char *vasktext(size_t size,
                      char *answer,
                      const char *defval,
                      const char *format,
                      va_list va)
{
  return callui(ui_asktext, size, answer, defval, format, va).cptr;
}

inline void vwarning(const char *message, va_list va)
{
  callui(ui_mbox, mbox_warning, message, va);
}

inline void vinfo(const char *message, va_list va)
{
  callui(ui_mbox, mbox_info, message, va);
}

inline void vnomem(const char *module, va_list va)
{
  callui(ui_mbox, mbox_nomem, module, va);
}

inline int vmsg(const char *format, va_list va)
{
  return callui(ui_msg, format, va).i;
}

inline bool display_copyright_warning(void)
{
  return callui(ui_copywarn).cnd;
}

#endif  // __UI__ END OF UI SERVICE FUNCTIONS

#ifdef __GUI__
bool idaapi set_xml(const char *path, const char *name, const char *value);
#endif
#if !defined(__UI__) || defined(__GUI__)
inline bool set_xml(const char *path, const char *name, int value)
{
  char buffer[12];
  qsnprintf(buffer, sizeof(buffer), "%d", value);
  return set_xml(path, name, buffer);
}
#endif

//---------------------------------------------------------------------------
//      E R R O R / W A R N I N G / I N F O   D I A L O G   B O X E S
//---------------------------------------------------------------------------

// If this variable is set, then dialog boxes will not appear on the screen.
// Warning/info messages are shown in the messages window.
// The default value of user input dialogs will be returned to the
// caller immediately.
// This variable is used to enable unattended work of ida.

idaman int ida_export_data batch;


idaman int ida_export_data errorexit;   // is 1 if we are exiting with from
                                        // error() function.
                                        // useful for atexit() functions.


// Display error dialog box and exit
//      message - printf() style message string.
//                See 'Format of dialog box' below for its format.
// Err() function does the same but the format string is taken from IDA.HLP

inline void error(const char *format,...)
{
  va_list va;
  va_start(va, format);
  verror(format, va);
  va_end(va);
}

inline void Err(help_t format,...)
{
  va_list va;
  va_start(va, format);
  verror(itext(format), va);
  va_end(va);
}


// Display warning dialog box and wait for the user to press Enter or Esc
//      message - printf() style message string.
//                See 'Format of dialog box' below for its format.
// This messagebox will by default contain a "Don't display this message again"
// checkbox if the message is repetitively displayed. If checked, the message
// won't be displayed anymore during the current IDA session.
// Warn() function does the same but the format string is taken from IDA.HLP

inline void warning(const char *message,...)
{
  va_list va;
  va_start(va, message);
  vwarning(message, va);
  va_end(va);
}

inline void Warn(help_t format,...)
{
  va_list va;
  va_start(va, format);
  vwarning(itext(format), va);
  va_end(va);
}


// Display info dialog box and wait for the user to press Enter or Esc
//      message - printf() style message string.
//                See 'Format of dialog box' below for its format.
// This messagebox will by default contain a "Don't display this message again"
// checkbox. If checked, the message will never be displayed anymore (state saved
// in the Windows registry or the idareg.cfg file for a non-Windows version).
// Info() function does the same but the format string is taken from IDA.HLP

inline void info(const char *message,...)
{
  va_list va;
  va_start(va, message);
  vinfo(message, va);
  va_end(va);
}

inline void Info(help_t format,...)
{
  va_list va;
  va_start(va, format);
  vinfo(itext(format), va);
  va_end(va);
}


// Display "no memory for module ..." dialog box and exit
//      module  - printf() style message string.
//                name of module.

inline void nomem(const char *module,...)
{
  va_list va;
  va_start(va, module);
  vnomem(module, va);
  va_end(va);
}


// Output a formatted string to messages window [analog of printf()]
//      format - printf() style message string.
// Message() function does the same but the format string is taken from IDA.HLP
// Returns: number of bytes output
//
// Everything appearing on the messages window may be written
// to a text file. For this the user should define an environment
// variable IDALOG:
//         set IDALOG=idalog.txt
//

inline int msg(const char *format,...)
{
  va_list va;
  va_start(va, format);
  int nbytes = vmsg(format, va);
  va_end(va);
  return nbytes;
}

inline int Message(help_t format,...)
{
  va_list va;
  va_start(va, format);
  int nbytes = vmsg(itext(format), va);
  va_end(va);
  return nbytes;
}


//----------------------------------------------------------------------
//      F O R M S  -  C O M P L E X   D I A L O G   B O X E S
//----------------------------------------------------------------------
// Display a dialog box and wait for the user.
//      form     - dialog box as a string
//                 See its format below
// return: 0 - the user pressed Esc or no memory to display
//             a dialog box (a warning is displayed in this case).
//             all variables retain their original values.
//         1 - ok, all input fields are filled and validated.
// If the form contains the NOBUTTON keyword, then the return values are
// the same as in the askyn() function: 1-yes, 0-no, -1-cancel

int AskUsingForm_cv(const char *form, va_list va);
inline int AskUsingForm_c(const char *form,...)
{
  va_list va;
  va_start(va, form);
  int code = AskUsingForm_cv(form, va);
  va_end(va);
  return code;
}

// The following callback function is called when the user presses a
// form-specific button (defined with a field type B).
//      fields - array of input fields
//      code   - code of the pressed button

typedef void (idaapi *formcb_t)(TView *fields[],int code); // callback for buttons


// From this callback you may call close_form() function.
// The form will be closed as soon as you return from the callback.
//      is_ok - 1: form is closed normally as if the user pressed Enter
//              0: form is closed abnormally as if the user pressed Esc

void close_form(TView *fields[],int is_ok);

//------------------------------------------------------------------------
/* Format of FORM text strings (actually they are mutliline strings
                                delimited by newline characters)

  Thw following keywords might appear at the beginning of the form
  (case insensitive):

  STARTITEM number

    where number is a number of input field the cursor will stand on.
    By default the cursor is in the first field of the dialog box.
    The input fields are numbered from 0 (the first field is field 0).

  BUTTON name caption

    Alternative caption for a button. It may contain the character
    to highlight in this form:  ~Y~es
    Valid button names are: YES, NO, CANCEL
    For example:
        BUTTON YES Please do
        BUTTON NO Nope
        BUTTON CANCEL NONE

    By default the NO button is not displayed. If it is displayed, then
    the return value of the function will be different!
    (see the function description)

    Empty text means that there won't be any corresponding button.
    (you may also use NONE as the caption to hide it)

    A * after the button name means that this button will be the default:

      BUTTON CANCEL* Cancel

  Next, if the dialog box is kept in IDA.HLP, the following may appear:
  (this defines help context for the whole dialog box)

  @hlpMessageName[]

  If the form is not in IDA.HLP file, then it may have a built-in
  help message. In this case the help screen should be enclosed in the
  following keywords:

  HELP
  ....
  ....
  ....
  ENDHELP

  Each keyword should be alone on a line.

  Next there must be the title line and 2 empty lines.
  All text in the dialog box text string is copied to the dialog without
  any modifications. There are two special cases:

        - format parameters
        - input fields

  Format parameters are used to specify variant parts of dialog box.
  They are specified as "%x" combination, where x is format specifier.
  All input field types (except B and K) are valid format specifiers.
  List of input field types is given below. Parameter for "%x" combination
  is taken from the list function input arguments (va_list). The corresponding
  argument should contain pointer (sic, pointer) to the value to be converted
  and displayed. For example, dialog box:

  ------ format:
        Sample dialog box


        This is sample dialog box for %A
        using address %$

        <~E~nter value:N:32:16::>

  ------

  should be called as
                char *string = "something";
                ea_t addr = someaddr;
                uval_t answer = 0;
                int ok = AskUsingForm_c(format, string, &addr, &answer);


  Input fields are represented by the following combination:

  <label:type:width:swidth:@hlp[]>

  where
        label - any text string serving as label for the input field
                the label may contain hotkey definition like this: "~O~pen"
                (O keystroke is hotkey here)
        type  - a character specifing type of input field.
                The form() function will perform initial validation of
                value specified by the user and convert it appropriately.
                See table of input field types below.
        width - decimal number specifying width of input field.
                this number may be omitted.
                For field of type 'B' this field contains code generated
                when the user presses the button.
        swidth -decimal number specifying width of visible part of input field.
                this number may be omitted.
        @hlp[]- help context for the input field. you may replace the
                help context with '::' (two colons) if you don't want to
                specify help context. The help context is a number of help
                screen from IDA.HLP file.


  Input field types                               va_list parameter
  -----------------                               -----------------

  A - ascii string                                char* at least MAXSTR size
  S - segment                                     sel_t*
  N - hex number, C notation                      uval_t*
  n - signed hex number, C notation               sval_t*
  L - default base (usually hex) number,          ulonglong*
      C notation
  l - default base (usually hex) number, signed,  longlong*
      C notation
  M - hex number, no "0x" prefix                  uval_t*
  D - decimal number                              sval_t*
  O - octal number, C notation                    sval_t*
  Y - binary number, "0b" prefix                  sval_t*
  H - char value, C notation                      sval_t*
  $ - address                                     ea_t*
  I - ident                                       char* at least MAXNAMELEN size
  B - button                                      formcb_t
  K - color button                                bgcolor_t*

  The n, N, D, O, Y, H fields interpret the input as an IDC expression.
  M and $ fields fallback to IDC if the input can not be interpreted
  correctly.

  The hint message can be specified before the label enclosed in '#':

  <#hint message#label:...>

  Radiobuttons and checkboxes are represented by:

  <label:type>
  <label:type>>         - end of block

  where valid types are C and R
  (you may use lowercase 'c' and 'r' if you need to create two radiobutton
  or checkbox groups on the same lines)

  field types           va_list parameter
  -----------           -----------------

  C - checkbox          ushort*                 bit mask of checkboxes
  R - radiobutton       ushort*                 number of radiobutton

  The box title and hint messages can be specified like this:

  <#item hint#title#box hint#label:type>

  The title and the box hint can be specified only in the first item of the box.
  If the hint doesn't exist, it should be specified as an empty hint (##title##)
  The subsequent items can have an item hint only:

  <#item hint#label:type>

  Initial values of input fields are specified in the corresponding
  input/output parameters (taken from va_list array).

  Ok, Cancel and (possibly) Help buttons are displayed at the bottom of
  the dialog box automatically. Their captions can be changed by the keywords
  described at the beginning of this page.

  Input field definition examples:

   <Kernel analyzer options ~1~:B:0:::>
   <~A~nalysis enabled:C>
   <~I~ndicator enabled:C>>
   <Names pre~f~ix  :A:15:15::>

*/

//---------------------------------------------------------------------------
//      Y E S / N O   D I A L O G  B O X
//---------------------------------------------------------------------------

// Display a dialog box and get choice from "Yes", "No", "Cancel"
//      deflt   - default choice:
//                  1 - Yes
//                  0 - No
//                 -1 - Cancel
//      format  - The question in printf() style format
// returns: 1=yes, 0=no, -1=cancel

inline int askyn_cv(int deflt,const char *format,va_list va)
{
  return askbuttons_cv(NULL, NULL, NULL, deflt, format, va);
}

inline int askyn_c(int deflt,const char *format,...)
{
  va_list va;
  va_start(va, format);
  int code = askyn_cv(deflt, format, va);
  va_end(va);
  return code;
}

inline int askyn_v(int deflt,help_t format,va_list va)
{
  return askyn_cv(deflt, itext(format), va);
}

inline int askyn(int deflt,help_t format,...)
{
  va_list va;
  va_start(va, format);
  int code = askyn_cv(deflt, itext(format), va);
  va_end(va);
  return code;
}


// Display a dialog box and get choice from maximum three possibilities
//      Yes             - text for the first button
//      No              - text for the second button
//      Cancel          - text for the third button
//                      for all buttons:
//                        "" or NULL - take the default name for the button
//                      use 'format' to hide the cancel button
//
//      deflt           - default choice:
//                         1 - Yes      (first button)
//                         0 - No       (second button)
//                        -1 - Cancel   (third button)
//      format          - printf-style format string for question
//                        See its format below
//      va              - parameters for the format string
// returns: 1=yes(first button), 0=no(second button), -1=cancel(third button)

inline int askbuttons_c(const char *Yes,
                        const char *No,
                        const char *Cancel,
                        int deflt, const char *format,...)
{
  va_list va;
  va_start(va, format);
  int code = askbuttons_cv(Yes, No, Cancel, deflt, format, va);
  va_end(va);
  return code;
}

//------------------------------------------------------------------------
/* Format of dialog box (actually they are mutliline strings
                         delimited by newline characters)

  The very first line of dialog box can specify a dialog box
  title if the following line appears:

  TITLE title string


  Then, the next line may contain an icon to display
  in the GUI version (ignored by the text version):

  ICON NONE          (no icon)
       INFO          (information icon)
       QUESTION      (question icon)
       WARNING       (warning icon)
       ERROR         (error icon)


  Then, the next line may contain a 'Don't display this message again'
  checkbox. If this checkbox is selected and the user didn't select cancel,
  the button he selected is saved and automatically returned.

  AUTOHIDE NONE      (no checkbox)
           DATABASE  (return value is saved to database)
           REGISTRY  (return value is saved to Windows registry or idareg.cfg
                      if non-Windows version)
           SESSION   (return value is saved for the current IDA session)


  To hide the cancel button the following keyword can be used:

  HIDECANCEL

  Please note that the user still can cancel the dialog box by pressing Esc
  or clicking on the 'close window' button.

  Finally, if the dialog box is kept in IDA.HLP, the following may appear
  to add a Help button (this defines help context for the whole dialog box):

  @hlpMessageName[]


  Each keyword should be alone on a line.

  Next, a format string must be specified.
  To center message lines in the text version, start them with '\3' character
  (currently ignored in the GUI version).
*/

//---------------------------------------------------------------------------
//      A S K   S T R I N G   O F   T E X T
//---------------------------------------------------------------------------

// Display a dialog box and wait for the user to input a text string
// Use this function to ask one-line text. For multiline input, use
// asktext() and asksmalltext() functions.
// This function will trim the trailing spaces.
//      hist    - category of history lines. an arbitrary number.
//                this number determines lines accessible in the history
//                of the user input (when he presses down arrow)
//                One of HIST_.. constants should be used here
//      defval  - default value. will be displayed initially in the input line.
//                may be NULL.
//      format  - printf() style format string with the question
// returns: NULL-if the user pressed Esc.
//          otherwise returns the entered value in a static storage.

inline char *askstr(int hist,const char *defval,const char *format,...)
{
  va_list va;
  va_start(va, format);
  char *result = vaskstr(hist, defval, format, va);
  va_end(va);
  return result;
}

// Input line history categories used by the kernel:

#define HIST_SEG    1           // segment names
#define HIST_CMT    2           // comments
#define HIST_SRCH   3           // search substrings
#define HIST_ADDR   4           // addresses
#define HIST_IDENT  5           // names
#define HIST_NUM    6           // numbers
#define HIST_FILE   7           // file names
#define HIST_TYPE   8           // type declarations


// Display a dialog box and wait for the user to input multiline text
//      size    - maximum size of text in bytes
//      answer  - output buffer. if you specify NULL then the answer
//                will be returned in a buffer allocated by
//                'new char' operator.
//      defval  - default value. will be displayed initially in the input line.
//                may be NULL.
//      format  - number of message from IDA.HLP to display as question
//                in printf() style format
// returns: NULL-if the user pressed Esc.
//          otherwise returns pointer to text.

inline char *asktext(size_t size,
                     char *answer,
                     const char *defval,
                     const char *format,
                     ...)
{
  va_list va;
  va_start(va, format);
  char *result = vasktext(size, answer, defval, format, va);
  va_end(va);
  return result;
}


//---------------------------------------------------------------------------
//      A S K   A D D R E S S E S ,   N A M E S ,   N U M B E R S ,   E T C .
//---------------------------------------------------------------------------

// Display a dialog box and wait for the user to input a file name
//      savefile- the entered file name will be used to save file
//      defval  - default value. will be displayed initially in the input line.
//                may be NULL. may be a wildcard file name.
//      format  - printf() style format string with the question
// This function displays a window with file names present in the directory
// pointed by 'defval'.
// returns: NULL-if the user pressed Esc.
//          otherwise the user entered a valid file name.

char *askfile_cv(int savefile,const char *defval,const char *format,va_list v);
inline char *askfile_c(int savefile,const char *defval,const char *format,...)
{
  va_list va;
  va_start(va, format);
  char *answer = askfile_cv(savefile, defval, format, va);
  va_end(va);
  return answer;
}


//---------------------------------------------------------------------------
//      S T R I N G   F U N C T I O N S
//---------------------------------------------------------------------------

// Add space characters to the string so that its length will be at least
// 'len' characters. Don't trim the string if it is longer than 'len'.
//      str - pointer to string to modify (may not be NULL)
//      len - wanted length of string
// This function may be applied to colored strings
// returns: pointer to the end of input string

idaman char *ida_export addblanks(char *str,int len);


// Remove trailing space characters from a string
//      str - pointer to string to modify (may be NULL)
// returns: pointer to input string

idaman char *ida_export trim(char *str);


// Skip whitespaces in the string

idaman const char *ida_export skipSpaces(const char *ptr);
inline char *skipSpaces(char *ptr)
  { return (char*)skipSpaces((const char *)ptr); }


// Find one string in another.
// Case insensivite analog of strstr()

idaman char *ida_export stristr(const char *s1, const char *s2);


// Find a line with the specified code in the strarray_t array
// If the last element of the array has code==0 then it is considered
// as the default entry
// If no default entry exists and the code is not found, strarray() returns ""

struct strarray_t
{
  int code;
  const char *text;
};

idaman const char *ida_export strarray(const strarray_t *array, size_t array_size, int code);


// Convert whitespace to tabulations
// This functin will stop the conversion as soon as a string or character constant
// is encountered

idaman void ida_export entab(char *string);


//---------------------------------------------------------------------------
//      C O N V E R S I O N S
//---------------------------------------------------------------------------


// Linear address to ascii string and vice versa

idaman size_t ida_export ea2str(ea_t ea, char *buf, size_t bufsize);
idaman bool ida_export str2ea(const char *str, ea_t *ea_ptr, ea_t screenEA);


// Number in C-notation to an address

idaman bool ida_export atoea(const char *str, ea_t *pea);


// Segment selector to ascii string and vice versa

idaman size_t ida_export stoa(ea_t from, sel_t seg, char *buf, size_t bufsize);
idaman int ida_export atos(const char *str, sel_t *seg); //0-fail,1-ok(hex),2-ok(segment name or reg)


// a number to ascii using default radix with leading zeroes.
// used to output number in binary line prefixes

// get number of ascii characters required to represent
// a number with the specified number of bytes and radix
// radix==0: use default radix, usually 16
// nbytes==0: use default number of bytes, usually 4 or 8 depending on __EA64__
// return the answer length

#define MAX_NUMBUF (128+8) // 16-byte value in binary base (0b00101010...)
idaman size_t ida_export b2a_width(int nbytes, int radix);
idaman size_t ida_export b2a32(ulong x, char *buf, size_t bufsize, int nbytes, int radix);
                                                        // nbytes: 1,2,3,4
idaman size_t ida_export b2a64(ulonglong x, char *buf, size_t bufsize, int nbytes, int radix);
                                                        // nbytes: 1,2,3,4,8

// a number to ascii, the nicest representation of the number without leading zeroes
// can be used to output some numbers in the instructions
// return the answer length

idaman size_t ida_export btoa32(char *buf, size_t bufsize, ulong x, int radix=0);
idaman size_t ida_export btoa64(char *buf, size_t bufsize, ulonglong x, int radix=0);
idaman size_t ida_export btoa128(char *buf, size_t bufsize, uint128 x, int radix=0);
idaman size_t ida_export btoa_width(int nbytes, flags_t flag, int n); // get max width
#ifdef __EA64__
#define b2a b2a64
#define btoa btoa64
#define atob atob64
#else
#define b2a b2a32
#define btoa btoa32
#define atob atob32
#endif


// a instruction operand immediate number to ascii
// the main function to output numbers in the instruction operands
// it prints the number with or without the leading zeroes depending on the flags
// this function is called from OutValue(), please use OutValue() if you can

idaman size_t ida_export numop2str(char *buf,
                                   size_t bufsize,
                                   ea_t ea,
                                   int n,
                                   ulonglong x,
                                   int nbytes,
                                   int radix=0);


// ascii to a number using the current assembler formats
// e.g. for ibmpc 12o is octal, 12h is hex etc.

idaman bool ida_export atob32(const char *str,ulong *x);     // returns 1-ok
idaman bool ida_export atob64(const char *str,ulonglong *x); // returns 1-ok


// auxillary function
// print displacement to a name (+disp or -disp) in the natural radix
//      buf  - pointer to the output buffer
//      end  - pointer to the end of the output buffer
//      disp - displacement to output. 0 leads to no modifications

idaman size_t ida_export print_disp(char *buf, char *end, adiff_t disp);


// String C-style conversions (convert \n to a newline and vice versa)

idaman char *ida_export str2user(char *dst,const char *src, size_t dstsize); // make a user representation
idaman char *ida_export user2str(char *dst,const char *src, size_t dstsize); // make an internal representation
idaman char ida_export back_char(const char *&p);            // Translate char after '\\'


// ASCII <-> RADIX50 conversions
//      r - pointer to radix50 string
//      p - pointer to ascii string
//      k - number of elements in the input string
//            (element of radix50 string is a word)
//            (element of ascii   string is a character)
// return: number of elements left unprocessed in the input string
//         because the input string contains unconvertible elements.
//         0-ok, all elements are converted

idaman int ida_export r50_to_asc(ushort *r, char *p, int k);
int                   asc_to_r50(char *p, ushort *r, int k);


// the following characters are allowed in ASCII strings, i.e.
// in order to find end of a string IDA looks for a character
// which doesn't belong to this array:

idaman char ida_export_data AsciiStringChars[256+1];


// Pack/unpack 2, 4, and 8 byte numbers into character string
// These functions encode numbers using an encoding similar to UTF.
// The smaller is the number, the better is packing.

inline uchar *idaapi pack_db(uchar *ptr, uchar *end, uchar x)
{
  if ( ptr < end )
    *ptr++ = x;
  return ptr;
}
inline uchar idaapi unpack_db(const uchar **pptr, const uchar *end)
{
  const uchar *ptr = *pptr;
  uchar x = 0;
  if ( ptr < end )
    x = *ptr++;
  *pptr = ptr;
  return x;
}

idaman uchar    *ida_export pack_dw(uchar *ptr, uchar *end, ushort x);
idaman uchar    *ida_export pack_dd(uchar *ptr, uchar *end, ulong x);
idaman uchar    *ida_export pack_dq(uchar *ptr, uchar *end, ulonglong x);
idaman ushort    ida_export unpack_dw(const uchar **pptr, const uchar *end);
idaman ulong     ida_export unpack_dd(const uchar **pptr, const uchar *end);
idaman ulonglong ida_export unpack_dq(const uchar **pptr, const uchar *end);
inline uchar *pack_ea(uchar *ptr, uchar *end, ea_t ea)
{
#ifdef __EA64__
  return pack_dq(ptr, end, ea);
#else
  return pack_dd(ptr, end, ea);
#endif
}
inline ea_t unpack_ea(const uchar **ptr, const uchar *end)
{
#ifdef __EA64__
  return unpack_dq(ptr, end);
#else
  return unpack_dd(ptr, end);
#endif
}

// pack/unpack a string
// unpack_ds will allocate the output string in the dynamic memory. The caller
// should use qfree() to deallocate it.
//   empty_null - if true, then return NULL for empty strings
//                otherwise return an empty string (not NULL)

idaman uchar *ida_export pack_ds(uchar *ptr, uchar *end, const char *x, size_t len=0);
idaman char *ida_export unpack_ds(const uchar **pptr, const uchar *end, bool empty_null);


// packed sizes
static const int ea_packed_size = sizeof(ea_t) + sizeof(ea_t)/4; // 5 or 10 bytes
static const int dq_packed_size = 10;
static const int dd_packed_size = 5;
static const int dw_packed_size = 3;
inline int ds_packed_size(const char *s) { return s ? int(strlen(s)+dd_packed_size) : 1; }


// Calculate CRC32. initial crc should be 0.
idaman ulong ida_export calc_crc32(ulong crc, const void *buf, size_t len);

// calculate an input source crc32
idaman ulong ida_export calc_file_crc32(linput_t *fp);

// match a string with a regular expression
// returns: 0-no match, 1-match, -1-error
idaman int ida_export regex_match(const char *str, const char *pattern, bool sense_case);


#pragma pack(pop)
#endif // __KERNWIN_HPP
