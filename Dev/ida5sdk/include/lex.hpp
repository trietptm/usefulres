/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-99 by Ilfak Guilfanov, <ig@datarescue.com>
 *      ALL RIGHTS RESERVED.
 *
 */

#ifndef LEX_HPP
#define LEX_HPP
#pragma pack(push, 1)   // IDA uses 1 byte alignments!

typedef ushort lxtype;
const lxtype
                        // all separators have their ASCII codes as lxtype
  lx_end      = 1,      // no more tokens
  lx_ident    = 2,      // ident
  lx_number   = 3,      // long constant
  lx_string   = 4,      // string constant (token_t.chr != 0 => unicode string)
  lx_char     = 5,      // char constant
  lx_typename = 6,      // user-defined type
  lx_float    = 7,      // IEEE floating point constant
  lx_key      = 128;    // keywords start. All keys are lx_key + keynum
                        // Two char seps are: (c1 + (c2 << 8))
                        // Three char seps: <<=,>>= are
                        //      ('<' + ('<'<<8)) + '='
                        //      ('>' + ('>'<<8)) + '='

struct token_t
{
  lxtype type;
  char str[MAXSTR];             // idents & strings
  sval_t num;                   // long & char constants
  bool unicode;                 // (lx_string: != 0 => unicode string)
  ushort fnum[6];               // floating point constant
};

class lexer_t;                  // lexical analyzer, opaque structure

typedef const char *idaapi lxgets_t(lexer_t *lx, error_t *res);
                                        // Lx next line getter function type
                                        // When no more lines left, this function
                                        // should set *res to -1.
                                        // and return NULL;

typedef error_t lx_resolver_t(lexer_t *lx, void *ud, token_t *curtok, sval_t *res);
                                        // preprocessor callback for unknown tokens.
                                        // will be called when preprocessor calculates
                                        // the value of #if expression
typedef int idaapi lx_preprocessor_cb(void *ud, const char *fname, int nl);
                                        // preprocessor callback
                                        // for debugging. it will be called each
                                        // time when file/line number is changed
                                        // It returns an error code (0-ok)
typedef int idaapi lx_pragma_cb(void *ud, const char *line);
                                        // callback for #pragma directives
                                        // It returns an error code (0-ok)

typedef int idaapi lx_macro_cb(         // Callback for #define directives
        void *ud,
        const char *name,               // It returns an error code (0-ok)
        const char *body,
        int nargs,
        bool isfunc,
        bool is_new_macro);

// Create new lexical analyzer and set its keyword table
// if keys==NULL, then set the default C keyword table

idaman lexer_t *ida_export create_lexer(const char *const *keys,
                                        size_t size,
                                        void *ud=NULL);


// Destroy a lexical analyzer

idaman void ida_export destroy_lexer(lexer_t *lx);


// define a macro
idaman error_t ida_export lex_define(lexer_t *lx,
                                     const char *macro,
                                     const char *body,
                                     int nargs=0,
                                     bool isfunc=false);

// undefine a macro
idaman void ida_export lex_undef(lexer_t *lx,
                                 const char *macro);

// enumerate all macros
// until cb returns non-zero
idaman int ida_export lex_enum_macros(
        lexer_t *lx,
        int idaapi cb(const char *name, const char *body, int nargs, bool isfunc, void *ud),
        void *ud);

// debug: get text representation of token
idaman const char *ida_export lxascii(const token_t *t, char *buf, size_t bufsize);

//--------- String oriented functions -------------------------------------

struct macro_t;
// set the input line and optionally the macro table
// if macros==NULL, the macro table will not be changed
idaman error_t ida_export lxini(lexer_t *lx,
                                const char *line,
                                macro_t **macros=NULL);
const char *lxptr(lexer_t *lx);         // get pointer to next char
const char *lxprev(lexer_t *lx);        // get pointer to prev char
void set_lxprev(lexer_t *lx, const char *ptr);
idaman error_t ida_export line_lxget(lexer_t *lx, token_t *t);
                                        // get next token from string
                                        // 0 - Ok else error Number
//--------- File oriented functions ---------------------------------------
idaman const char *ida_export lxgets(lexer_t *lx, error_t *res);
                                        // standard file oriented 'gets'
idaman error_t ida_export lxgetsini(lexer_t *lx, const char *file);
                                        //  - initialization
                                        // file may be NULL
idaman char*   ida_export lxgetserr(lexer_t *lx, long *linenum,char **lineptr, int level=0); //  - error handling
                                        // if level > 0, then return information
                                        // about the enclosing file which
                                        // included the current one
idaman void    ida_export lxgetstrm(lexer_t *lx, bool del_macros); //  - termination

lxgets_t *lxset_gets(lexer_t *lx, lxgets_t *new_func); // Set pointer to string getter function
idaman error_t ida_export lxget(lexer_t *lx, token_t *t);   // get next token using given string getter function
                                        // else error Number

lx_preprocessor_cb *set_preprocessor_cb(lexer_t *lx, lx_preprocessor_cb *cb);
lx_pragma_cb *set_pragma_cb(lexer_t *lx, lx_pragma_cb *cb);
lx_macro_cb *set_macro_cb(lexer_t *lx, lx_macro_cb *cb);
void *set_cb_data(lexer_t *lx, void *user_data);

// Evaluate a macro to a numeric value
error_t lx_eval(lexer_t *lx, const char *macro, sval_t *answer, lx_resolver_t *cb);

// Retrieve the next token from the #if expression.
// This function must be used only from the lx_resolver_t callback
void ce_lxget(lexer_t *lx);

// return the current macro table and destroy it
macro_t **extract_macro_table(lexer_t *lx);

// destroy a macro table
void free_macro_table(macro_t **macros);

// Functions to push/pop macro definitions
error_t push_lex_macro(lexer_t *lx, const char *name);
error_t pop_lex_macro(lexer_t *lx, const char *name);

int idaapi lex_set_cmttype(lexer_t *lx, int lex_cmttype);   // Returns old settings. Initial settings are C+CPP
#define LEX_CMT_C   0x0001     // Comments like /*..*/
#define LEX_CMT_PAS 0x0002     // Comments like (*..*)
#define LEX_CMT_CPP 0x0004     // Comments like //


// Get text representation of an IDC error code

void ExprGetError(lexer_t *lx,
                  error_t code,
                  token_t *ahead,
                  char *buf,
                  size_t bufsize);

#pragma pack(pop)
#endif  //  LEX_HPP
