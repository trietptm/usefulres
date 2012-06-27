/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-99 by Ilfak Guilfanov, <ig@datarescue.com>
 *      ALL RIGHTS RESERVED.
 *
 */

#ifndef _EXPR_H
#define _EXPR_H
#pragma pack(push, 1)   // IDA uses 1 byte alignments!

/*
        This file contains functions that deal with C-like expressions
        and built-in IDC language.
*/

/*-------------------------------------------------------------------------*/
/*      T y p e  D e f i n i t i o n s                                     */
/*-------------------------------------------------------------------------*/

/*      Attention!!!
        While allocating value_t variables in the system stack
        (automatic variables), don't forget to initialize them:

                value_t v; v.vtype = VT_LONG;

*/

class value_t          /* Result of expression */
{
public:
  char vtype;           /* Type                 */
#define  VT_STR         1
#define  VT_LONG        2
#define  VT_FLOAT       3
#define  VT_WILD        4       // used only in function arg type declarations
  union
  {
    char *str;          /* T_str        */
    sval_t num;         /* T_long       */
    ushort e[6];        /* T_flt        */
  };
};


struct extfun_t                 /* Element of functions table */
{
  const char *name;             /* Name of function */
  error_t (idaapi *fp)(value_t *argv,value_t *res); /* Pointer to the Function */
  const char *args;             /* Type of arguments. Terminated with 0 */
                                /* VT_WILD means a function with arbitrary
                                   number of arguments. Actual number of
                                   arguments will be passed in res->num */
  int flags;                    /* Function description flags           */
#define EXTFUN_BASE 0x0001      /*  - requires open database            */
};

struct funcset_t
{
  int qnty;             /* Quantity of functions */
  extfun_t *f;          /* Functions table */
  error_t (idaapi *startup)(void);
  error_t (idaapi *shutdown)(void);
};

class lexer_t;
/*-------------------------------------------------------------------------*/

// Array of built-in IDA functions

idaman funcset_t ida_export_data IDCFuncs; // external functions


// Add/remove a built-in IDC function
//      name - function name to modify
//      fp   - pointer to the function which will handle this IDC function
//             == NULL: remove the specified function
//      args - prototype of the function, zero terminated array of VT_...
// returns: success
// This function does not modify the predefined kernel functions
// Example:
//
//  static const char myfunc5_args[] = { VT_LONG, VT_STR, 0 };
//  static error_t idaapi myfunc5(value_t *argv, value_t *res)
//  {
//    msg("myfunc is called with arg0=%a and arg1=%s\n", argv[0].num, argv[1].str);
//    res->num = 5;     // let's return 5
//    return eOk;
//  }
//
//  after:
//      set_idc_func("MyFunc5", myfunc5, myfunc5_args);
//  there is a new IDC function which can be called like this:
//      MyFunc5(0x123, "test");

idaman bool ida_export set_idc_func(const char *name,
                                    error_t (idaapi *fp)(value_t *argv,value_t *res),
                                    const char *args);


/*-------------------------------------------------------------------------*/
// Convert IDC variable to a long (32/64bit) number
// Returns: v = 0 if impossible to convert to long

idaman error_t ida_export VarLong(value_t *v);


// Convert IDC variable to a long number
// Returns: v = 0         if IDC variable = "false" string
//          v = 1         if IDC variable = "true" string
//          v = number    if IDC variable is number or string containing a number
//          eTypeConflict if IDC variable = empty string

idaman error_t ida_export VarNum(value_t *v);


// Convert IDC variable to a text string

idaman error_t ida_export VarString(value_t *v);


// Convert IDC variable to a floating point

idaman void ida_export VarFloat(value_t *v);


// Free storage used by VT_STR type IDC variables

idaman void ida_export VarFree(value_t *v);            // frees string value only


/*-------------------------------------------------------------------------*/

// Get name of directory that contains IDC scripts.
// This directory is pointed by IDCPATH environment variable or
// it is in IDC subdirectory in IDA directory

idaman const char *ida_export get_idcpath(void);


// set or append a header path
//      path - list of directories to add (separated by ';')
//             may be NULL, in this case nothing is added
//      add  - true: append
//             false: remove old pathes
// return: true if success, false if no memory
// IDA looks for the include files in the appended header pathes
// then in the ida executable directory

idaman bool ida_export set_header_path(const char *path, bool add);


// Get full name of IDC file name.
// Search for file in list of include directories, IDCPATH directory
// and the current directory
//      buf - buffer for the answer
//      bufsize - its size
//      file - file name without full path
// Returns: NULL is file not found
//          otherwise returns pointer to buf

idaman char *ida_export get_idc_filename(char *buf, size_t bufsize, const char *file);


// Compile and execute "main" function from system file
//      file    - file name with IDC function(s)
//                The file will be searched in
//                      - the current directory
//                      - IDA.EXE directory
//                      - in PATH
//      flag    - 1: display warning if the file is not found
//                0: don't complain if file doesn't exist
// returns: 1-ok, file is compiled and executed
//          0-failure, compilation or execution error, warning is displayed

idaman bool ida_export dosysfile(bool complain_if_no_file, const char *file);


// Compile and calculate IDC expression
//      where - the current linear address in the addressing space of the
//              program being disassembled. If will be used to resolve
//              names of local variables etc.
//              if not applicable, then should be BADADDR
//      line  - a text line with IDC expression
//      res   - pointer to result. The result will be converted
//              to 32/64bit number. Use calcexpr() if you
//              need the result of another type.
//      errbuf- buffer for the error message
//      errbufsize - size of errbuf
// returns: true-ok, false-error, see errbuf

idaman bool ida_export calcexpr_long(ea_t where,
                                      const char *line,
                                      sval_t *res,
                                      char *errbuf,
                                      size_t errbufsize);

inline bool idaapi calcexpr_long(ea_t where,
                                  const char *line,
                                  uval_t *res,
                                  char *errbuf,
                                  size_t errbufsize)
{
  return calcexpr_long(where, line, (sval_t *)res, errbuf, errbufsize);
}


// Compile and calculate IDC expression
//      where - the current linear address in the addressing space of the
//              program being disassembled. If will be used to resolve
//              names of local variables etc.
//              if not applicable, then should be BADADDR
//      line  - a text line with IDC expression
//      rv    - pointer to result value. Its original value is discarded.
//      errbuf- buffer for the error message
//      errbufsize - size of errbuf
// returns: true-ok, false-error, see errbuf

idaman bool ida_export calcexpr(ea_t where,
                                 const char *line,
                                 value_t *rv,
                                 char *errbuf,
                                 size_t errbufsize);


// Compile and execute IDC expression.
//      line  - a text line with IDC expression
// returns: 1-ok
//          0-failure, a warning message is disaplayed

idaman bool ida_export execute(const char *line);


// Compile a text file with IDC function(s)
//      file       - name of file to compile
//                   if NULL, then "File not found" is returned.
//      del_macros - true to delete macros at the end of compilation.
//      errbuf     - buffer for the error message
//      errbufsize - size of errbuf
// returns: true-ok, false-error, see errbuf

idaman bool ida_export CompileEx(const char *file,
                                 bool del_macros,
                                 char *errbuf,
                                 size_t errbufsize);

inline bool ida_export Compile(const char *file,
                               char *errbuf,
                               size_t errbufsize)
{
  return CompileEx(file, true, errbuf, errbufsize);
}


// Compile one text line with IDC function(s)
//      line     - line with IDC function(s) (can't be NULL!)
//      errbuf   - buffer for the error message
//      errbufsize - size of errbuf
//      _getname - callback function to get values of undefined variables
//                 This function will be called if IDC function contains
//                 references to a undefined variable. May be NULL.
// returns: true-ok, false-error, see errbuf

idaman bool ida_export CompileLine(
                const char *line,
                char *errbuf,
                size_t errbufsize,
                uval_t (idaapi*_getname)(const char *name)=NULL);


// Execute a compiled IDC function.
//      fname   - function name
//      argsnum - number of parameters to pass to 'fname'
//                This number should be equal to number of parameters
//                the function expects.
//      args    - array of parameters
//      result  - ptr to value_t to hold result of the function.
//                'result' should not have VT_STR type, otherwise its
//                value will be discarded and the string won't be freed.
//                You may pass NULL if you are not interested in the returned
//                value.
//      errbuf  - buffer for the error message
//      errbufsize - size of errbuf
// returns: true-ok, false-error, see errbuf

idaman bool ida_export Run(
                const char *fname,              // function name
                int argsnum,
                const value_t args[],
                value_t *result,                // may be NULL. Any previous
                                                // value is DISCARDED (not freed)
                char *errbuf,
                size_t errbufsize);


// Compile and execute IDC function(s) on one line of text
//      line     - text of IDC functions
//      func     - function name to execute
//      getname  - callback function to get values of undefined variables
//                 This function will be called if IDC function contains
//                 references to a undefined variable. May be NULL.
//      argsnum  - number of parameters to pass to 'fname'
//                 This number should be equal to the number of parameters
//                 the function expects.
//      args     - array of parameters
//      result   - ptr to value_t to hold result of the function.
//                 'result' should not have VT_STR type, otherwise its
//                 value will be discarded and the string won't be freed.
//                 You may pass NULL if you are not interested in the returned
//                 value.
//      errbuf   - buffer for the error message
//      errbufsize - size of errbuf
// returns: true-ok, false-error, see errbuf

idaman bool ida_export ExecuteLine(
                const char *line,
                const char *func,
                uval_t (idaapi*getname)(const char *name),
                int argsnum,
                const value_t args[],
                value_t *result,                // may be NULL. Any previous
                                                // value is DISCARDED (not freed)
                char *errbuf,
                size_t errbufsize);


// Compile and execute IDC function(s) from file
//      file     - text file containing text of IDC functions
//      func     - function name to execute
//      getname  - callback function to get values of undefined variables
//                 This function will be called if IDC function contains
//                 references to a undefined variable. May be NULL.
//      argsnum - number of parameters to pass to 'fname'
//                This number should be equal to number of parameters
//                the function expects.
//      args    - array of parameters
//      result  - ptr to value_t to hold result of the function.
//                'result' should not have VT_STR type, otherwise its
//                value will be discarded and the string won't be freed.
//                You may pass NULL if you are not interested in the returned
//                value.
//      errbuf  - buffer for the error message
//      errbufsize - size of errbuf
// returns: true-ok, false-error, see errbuf

idaman bool ida_export ExecuteFile(
                const char *file,
                const char *func,
                int argsnum,
                const value_t args[],
                value_t *result,                // may be NULL. Any previous
                                                // value is DISCARDED (not freed)
                char *errbuf,
                size_t errbufsize);


// Add a compiled IDC function to the pool of compiled functions.
// This function makes the input function available to be executed.
//      name - name of the function
//      narg - number of the function parameteres
//      body - compiled body of the function
//      len  - length of the function body in bytes.

idaman void ida_export set_idc_func_body(
                const char *name,
                int narg,
                const uchar *body,
                size_t len);


// Get the body of a compiled IDC function
//      name - name of the function
//      narg - pointer to the number of the function parameteres (out)
//      len  - out: length of the function body (may be NULL)
// returns: pointer to the buffer with the function body
//             buffer will be allocated using qalloc()
//          NULL - failed (no such defined function)

idaman uchar *ida_export get_idc_func_body(
                const char *name,
                int *narg,
                size_t *len);


/*-------------------------------------------------------------------------*/

extern int idc_stacksize;       // Total number of local variables
extern int idc_calldepth;       // Maximal function call depth

int expr_printf(value_t *argv, value_t *r);
int expr_sprintf(value_t *argv, value_t *r);
int expr_printfer(int (*outer)(void *,char), void *ud, value_t *argv, value_t *r);


void idaapi init_idc(void);
void idaapi term_idc(void);
void del_idc_userfuncs(void);

extfun_t *find_builtin_idc_func(const char *name);

extern lexer_t *idc_lx;

#pragma pack(pop)
#endif /* _EXPR_H */
