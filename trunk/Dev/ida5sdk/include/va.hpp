/*
 *              Class "Virtual Array of Longs"
 *
 *              Written by Ilfak Guilfanov, 1991
 *              Latest modifications at 1997
 *              Copyright (c), All Rights Reserved.
 */

#ifndef VARRAY_HPP
#define VARRAY_HPP
#pragma pack(push, 1)   // IDA uses 1 byte alignments!

#include <pro.h>
#include <vm.hpp>

implementVM(ulong);

// internal classes
struct vaptr_t          // Chunk definition
{
  ea_t start;           // Start address of the chunk
  ea_t end;             // End address of the chunk (not included)
  uval_t offset;        // Offset in the file
                        // Offsets should be in increasing order
  void correct_offset(int pagesize)
  {
    int psize = pagesize / sizeof(ulong);
    offset = offset - offset%pagesize + (start%psize)*sizeof(ulong);
  }
  inline asize_t size(void) const { return end - start; }
};


#define VA_MAGIC "Va0"

struct vaheader_t                       // First bytes of VA file (header)
{
  char magic[4];                        // Must be VA_MAGIC
  ushort nchunks;                       // Number of chunks
  ushort eof;                           // Number of last used page + 1
};

// Callback function to Varray:scan, test functions

typedef bool idaapi va_test(ulong, void *ud);

// Main class declared in the file:
class Varray
{
public:

        Varray(void)    { Pages = NULL; }
        ~Varray(void)   { vclose(); }

        int     linkTo  (const char *file,int Psize,int PoolSize);
                                        // Psize - how many longs
                                        // PoolSize - how many pages in the cache
                                        // if file doesn't exist, it will be
                                        // created
        void    vclose   (void);
//
//      Address space functions
//
        error_t enable  (ea_t start,ea_t end);        // 0-ok,else error
        error_t disable (ea_t start,ea_t end);        // 0-ok,else error
        bool    enabled (ea_t addr)                   // is enabled ?
                                { return getoff(addr) != 0; }
        ea_t nextaddr  (ea_t addr); // get next enabled addr
                                    // if not exist, returns -1
        ea_t prevaddr  (ea_t addr); // get prev enabled addr
                                    // if not exist, returns -1
        ea_t prevchunk (ea_t addr); // return prev chunk last addr
                                    // if not exist, returns -1
        ea_t nextchunk (ea_t addr); // return next chunk first addr
                                    // if not exist, returns -1
        asize_t size   (ea_t addr); // return size of the chunk
                                    // addr may be any address in the chunk
        ea_t getstart  (ea_t addr); // get start address of the chunk
                                    // if illegal address given, returns -1
        int movechunk  (ea_t from, ea_t to, asize_t size);
                             // move information from one address to another. returns VAMOVE_...
#define VAMOVE_OK        0   // all ok
#define VAMOVE_BADFROM   -1  // the from address is not enabled
#define VAMOVE_TOOBIG    -2  // the range to move is too big
#define VAMOVE_OVERLAP   -3  // the target address range is already occupied
#define VAMOVE_TOOMANY   -4  // too many chunks are defined, can't move
        int check_move_args(ea_t from, ea_t to, asize_t size); // returns VAMOVE_...

//
//      Read/Write functions
//
        ulong   vread   (ea_t ea)            { return *Raddr(ea); }
        void    vwrite  (ea_t ea, ulong val) { *Waddr(ea)  =  val; }
        void    setbits (ea_t ea, ulong bit) { *Waddr(ea) |=  bit; }
        void    clrbits (ea_t ea, ulong bit) { *Waddr(ea) &= ~bit; }
        ulong*  Waddr   (ea_t ea);       // return &flags for ea, mark page as dirty
        ulong*  Raddr   (ea_t ea);       // return &flags for ea
        void    vflush  (void)          { Pages->vflush(); }

        void memset(ea_t start, asize_t size, ulong x);
        void memcpy(ea_t start, asize_t size, Varray &src, ea_t srcstart);
        ea_t memcmp(ea_t start, asize_t size, Varray &v2, ea_t v2start);
                                                // returns -1 - if equal
                                                // else address where mismatches
        ea_t memscn (ea_t start, asize_t size, ulong x);
        ea_t memtst (ea_t start, asize_t size, va_test *test, void *ud);
        ea_t memtstr(ea_t start, asize_t size, va_test *test, void *ud);

        ulong  *vread    (ea_t start,      ulong *buffer, size_t size);
        void    vwrite   (ea_t start,const ulong *buffer, size_t size);

        void    shright (ea_t from);    // Shift right tail of array
        void    shleft  (ea_t from);    // Shift left  tail of array

//
//      Sorted array functions (obsolete for 64-bit ea_t)!!!
//
#ifndef __EA64__
        ea_t bsearch(ea_t ea);          // Binary search
                                        // Returns index to >= ea
                                        // Attention: this func may return
                                        // pointer past array !
        bool addsorted(ea_t ea);        // Add an element to a sorted array.
                                        // If element exists, return 0
                                        // else 1
        bool delsorted(ea_t ea);        // Del element from a sorted array
                                        // If doesn't exist, return 0
                                        // else return 1
#endif

        void vadump(const char *msg, bool ea_sort_order); // debugging
        char *check(bool ea_sort_order); // check internal consistency

private:
  declareVM(ulong) *Pages;
  vaheader_t *header;                   // Header page
  ea_t lastea;                          // Last accessed address
  vaptr_t *lastvp;                      //  vptr of it
  ulong lastoff;                        //  offset of it
  ushort lastPage;
  ulong *lPage;
  size_t psize;                         // how many items can be put into a page
  size_t pagesize;                      // page size in bytes

//
// scan the Varray forward. call 'perform' for each page. If perform()
// returns >=0, then this is an index in the page, return address with this index.
// if perform() < 0, continue...
// Returns the address calculated by the index returned by perform().
//      perform() args: page - pointer to the page
//                      s    - size of the page
// 'change' - will perform() change the pages
//
  ea_t vascan(
        ea_t _start,
        asize_t size,
        ssize_t (idaapi*perform)(ulong *page, ssize_t s, void *ud),
        bool change,
        void *ud);
  ea_t vascanr(
        ea_t _start,
        asize_t size,
        ssize_t (idaapi*perform)(ulong *page, ssize_t s, void *ud),
        bool change,
        void *ud);

  ulong getoff(ea_t addr);
  int shiftPages(uval_t offset, int op, int np, bool changecur);
  void move_vm_chunk(int p1, int p2, int n);
  void swap(int x1, int x2, int n);
  vaptr_t *split(vaptr_t *cvp, ea_t from, ea_t end);
  void split_page(int kp, ea_t from);
  void merge_if_necessary(vaptr_t *vp);

  int add_pages(uval_t offset, int npages, bool changecur)
    { return shiftPages(offset, 0, npages, changecur); }
  int del_pages(uval_t offset, int npages, bool changecur)
    { return shiftPages(offset, npages, 0, changecur); }

  vaptr_t *getv0(void) const { return (vaptr_t *)(header + 1); }
  int getidx(const vaptr_t *vp) const { return int(vp - getv0()); }
  bool is_first(const vaptr_t *vp) const { return getidx(vp) == 0; }
  bool is_last(const vaptr_t *vp) const;
};

// Structure types used internally by Varray
struct vaidx_info_t
{
  Varray *source;
  ea_t index;
};

struct vascan_info_t
{
  va_test *test;
  void *ud;
  vascan_info_t(va_test *_test, void *_ud) : test(_test), ud(_ud) {}
};

#pragma pack(pop)
#endif // VARRAY_HPP
