/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-2001 by Ilfak Guilfanov, <ig@datarescue.com>
 *      ALL RIGHTS RESERVED.
 *
 *
 *      Graph drawing support
 *
 */

#ifndef __GDLDRAW_HPP
#define __GDLDRAW_HPP

#include <area.hpp>
#include <algorithm>
#include <vector>
#include <deque>
#include <map>

#pragma pack(push, 4)

//-------------------------------------------------------------------------
// forward declarations:
class node_iterator;
class flow_chart_t;
#define DECLARE_HELPER(decl)                                        \
decl node_iterator &ida_export node_iterator_goup(node_iterator *); \
decl void ida_export create_flow_chart(flow_chart_t &);

DECLARE_HELPER(idaman)

//-------------------------------------------------------------------------
// node iterator (used to draw graphs)
class node_iterator
{
  DECLARE_HELPER(friend)
  friend class gdl_graph_t;
  const gdl_graph_t *g;
  int i;
  node_iterator &_goup(void);
  node_iterator &goup(void) { return node_iterator_goup(this); }
public:
  node_iterator(const gdl_graph_t *_g, int n) : g(_g), i(n) {}
  node_iterator &operator++(void) { i++; return goup(); }
  bool operator==(const node_iterator &n) const { return i == n.i && g == n.g; }
  bool operator!=(const node_iterator &n) const { return !(*this == n); }
  int operator*(void) const { return i; }
};

//-------------------------------------------------------------------------
// gdl graph interface - includes only functions required to draw it
class gdl_graph_t
{
  virtual char *idaapi get_node_label(int /*n*/, char *buf, int /*bufsize*/) const { buf[0] = '\0'; return buf; }
  virtual void idaapi print_graph_attributes(FILE * /*fp*/) const {}
  virtual bool idaapi print_node(FILE * /*fp*/, int /*n*/) const { return false; }
  virtual bool idaapi print_edge(FILE * /*fp*/, int /*i*/, int /*j*/) const { return false; }
  virtual void idaapi print_node_attributes(FILE * /*fp*/, int /*n*/) const {}
public:
  virtual idaapi ~gdl_graph_t(void) {}
  virtual size_t idaapi size(void) const = 0;                    // number of the max node number
  virtual size_t idaapi node_qty(void) const { return size(); }  // number of alive nodes
  virtual bool idaapi exists(int /*node*/) const { return true; }
  virtual int  idaapi entry(void) const { return 0; }
  virtual int  idaapi exit(void) const { return size()-1; }
  virtual int  idaapi nsucc(int node) const = 0;
  virtual int  idaapi npred(int node) const = 0;
  virtual int  idaapi succ(int node, int i) const = 0;
  virtual int  idaapi pred(int node, int i) const = 0;
  virtual bool idaapi empty(void) const { return node_qty() == 0; }
          void idaapi gen_gdl(FILE *fp) const;
          void idaapi gen_gdl(const char *file) const;
          int  idaapi nedge(int node, bool ispred) const { return ispred ? npred(node) : nsucc(node); }
          int  idaapi edge(int node, int i, bool ispred) const { return ispred ? pred(node, i) : succ(node, i); }
          int  idaapi front(void) { return *begin(); }
  node_iterator idaapi begin(void) const { return node_iterator(this, 0).goup(); }
  node_iterator idaapi end(void)   const { return node_iterator(this, size()); }
};


// Create GDL file for graph
idaman void ida_export gen_gdl(const gdl_graph_t *g, const char *fname);


// Display GDL file by calling wingraph32
// The exact name of the grapher is taken from the configuration file
// and set up by setup_graph_subsystem()
// Returns error code from os, 0-ok

idaman int ida_export display_gdl(const char *fname);


//-------------------------------------------------------------------------
// Build and display program graphs

// Build and display a flow graph
//      title       - graph title
//      pfn         - function to graph
//      ea1, ea2    - if pfn == NULL, then address range
//      print_names - print labels for each block?
// returns: success. if fails, a warning message is displayed on the screen

idaman bool ida_export display_flow_graph(const char *title,
                                          func_t *pfn,
                                          ea_t ea1, ea_t ea2,
                                          bool print_names);


// Build and display a simple function call graph
//      wait        - message to display during graph building
//      title       - graph title
//      ignore_libfuncs - don't include library functions in the graph
// returns: success. if fails, a warning message is displayed on the screen

idaman bool ida_export display_simple_call_chart(const char *wait,
                                                 const char *title,
                                                 bool ignore_libfuncs);


// Build and display a complex xref graph
//      wait        - message to display during graph building
//      title       - graph title
//      ea1, ea2    - address range
//      flags       - combination of CHART_... constants (see below)
//      recursion_depth - optional limit of recursion
// returns: success. if fails, a warning message is displayed on the screen

idaman bool ida_export display_complex_call_chart(const char *wait,
                                                  const char *title,
                                                  ea_t ea1,
                                                  ea_t ea2,
                                                  int flags,
                                                  long recursion_depth=-1);


#define CHART_REFERENCING      0x001 // references to the addresses in the list
#define CHART_REFERENCED       0x002 // references from the addresses in the list
#define CHART_RECURSIVE        0x004 // analyze added blocks
#define CHART_FOLLOW_DIRECTION 0x008 // analyze references to added blocks only in the direction of the reference who discovered the current block
#define CHART_IGNORE_XTRN      0x010
#define CHART_IGNORE_DATA_BSS  0x020
#define CHART_IGNORE_LIB_TO    0x040 // ignore references to library functions
#define CHART_IGNORE_LIB_FROM  0x080 // ignore references from library functions
#define CHART_PRINT_COMMENTS   0x100
#define CHART_PRINT_DOTS       0x200 // print dots if xrefs exist outside of the range recursion depth


// Setup the user-defined graph colors and graph viewer program.
// This function is called by the GUI at the beginning, so no need to call
// it again.

idaman void ida_export setup_graph_subsystem(const char *grapher, bgcolor_t (idaapi *get_graph_color)(int color));


//--------------------------------------------------------------------------
//
//      The following definitions are for the kernel and the user interface only.
//      Third party plugins or modules should not use them.
//

class cancellable_graph_t : public gdl_graph_t
{
public:
  mutable bool cancelled;
  cancellable_graph_t(void) : cancelled(false) {}
  bool idaapi check_cancel(void) const;
};

//--------------------------------------------------------------------------
class graph_intseq_t : public std::vector<int>
{
public:
  void add(int x) { push_back(x); }
  void add_unique(int x)
  {
    if ( std::find(begin(), end(), x) == end() )
      add(x);
  }
};

struct basic_block_t : public area_t
{
  graph_intseq_t succ;  // we cheat here: we know that gen_gdl uses only succ
                        // and doesn't use pred. So we don't maintain preds at all
};

typedef std::map<ea_t, int> bn_t;    // block numbers


class flow_chart_t : public cancellable_graph_t
{
public:
  typedef std::vector<basic_block_t> blocks_t;
  DECLARE_HELPER(friend)
  std::string title;
  area_t bounds;
  func_t *pfn;
  int flags;
#define FC_PRINT 0x0001 // print names
#define FC_NOEXT 0x0002 // do not compute external blocks
  blocks_t blocks;
  bn_t bn;              // block numbers
  int nproper;          // number of basic blocks belonging to the specified area

  idaapi flow_chart_t(void) : flags(0) {}
  idaapi flow_chart_t(const char *_title, func_t *_pfn, ea_t _ea1, ea_t _ea2, bool _print_names)
    : title(_title), pfn(_pfn), bounds(_ea1, _ea2), flags(_print_names ? FC_PRINT : 0)
  {
    refresh();
  }
  void idaapi create(const char *_title, func_t *_pfn, ea_t _ea1, ea_t _ea2, bool _print_names)
  {
    title = _title;
    pfn   = _pfn;
    bounds = area_t(_ea1, _ea2);
    if ( _print_names )
      flags |= FC_PRINT;
    refresh();
  }
  void idaapi refresh(void) { create_flow_chart(*this); }
  void idaapi print_graph_attributes(FILE * /*fp*/) const;
  bool idaapi print_node(FILE * /*fp*/, int /*n*/) const;
  bool idaapi print_edge(FILE * /*fp*/, int /*i*/, int /*j*/) const;
  void idaapi print_node_attributes(FILE * /*fp*/, int /*n*/) const {}
  int  idaapi nsucc(int node) const { return blocks[node].succ.size(); }
  int  idaapi npred(int node) const { return 0; }
  int  idaapi succ(int node, int i) const { return blocks[node].succ[i]; }
  int  idaapi pred(int node, int i) const { return -1; }
  bool idaapi print_names(void) const { return (flags & FC_PRINT) != 0; }
  char *idaapi get_node_label(int /*n*/, char *buf, int /*bufsize*/) const { return NULL; }
  size_t idaapi size(void) const { return blocks.size(); }
};

#pragma pack(pop)
#endif

