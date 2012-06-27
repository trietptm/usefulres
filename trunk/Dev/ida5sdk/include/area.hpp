/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-97 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su
 *                              FIDO:   2:5020/209
 *
 */

#ifndef _AREA_HPP
#define _AREA_HPP

//
// This file contains definition of "areacb_t".
// "areacb_t" is a base class used by many parts of IDA.
// "areacb_t" is a collection of address ranges in the program.
// "areacb_t" conceptually consists of separate area_t instances.
// An area is a non-empty contiguous range of addresses (specified by
// its start and end addresses, the end address is excluded from the
// range) with some characteristics. For example, the ensemble of program
// segments is represented by an "areacb_t" called "segs".
//
// Areas are stored in the Btree part of the IDA database.
// To learn more about Btrees (Balanced Trees):
//  http://www.bluerwhite.org/btree/
//
#include <help.h>
#pragma pack(push, 1)           // IDA uses 1 byte alignments!

//--------------------------------------------------------------------------
// Base class for an area. This class is used as a base class for
// a class with real information - see segment.hpp for example.
// The end address points beyond the area.

struct area_t
{
  ea_t startEA;
  ea_t endEA;                  // endEA excluded
  area_t(void) {}
  area_t(ea_t ea1, ea_t ea2)  { startEA = ea1; endEA = ea2; }
  int compare(const area_t &r) const { return startEA > r.startEA ? 1 : startEA < r.startEA ? -1 : 0; }
  bool operator ==(const area_t &r) const { return compare(r) == 0; }
  bool operator !=(const area_t &r) const { return compare(r) != 0; }
  bool operator > (const area_t &r) const { return compare(r) >  0; }
  bool operator < (const area_t &r) const { return compare(r) <  0; }
  bool contains(ea_t ea) const { return startEA <= ea && endEA > ea; }
  bool contains(const area_t &r) const { return r.startEA >= startEA && r.endEA <= endEA; }
  bool empty(void) const { return startEA >= endEA; }
  asize_t size(void) const { return endEA - startEA; }
  void intersect(const area_t &r)
  {
    if ( startEA < r.startEA ) startEA = r.startEA;
    if ( endEA   > r.endEA   ) endEA   = r.endEA;
    if ( endEA   < startEA   ) endEA   = startEA;
  }
};


// This class is used to store part of information about areacb_t.

class sarray;           // sorted array - keeps information in Btree.


// Size of internal cache of areas.

#define AREA_CACHE_SIZE 128


// Convenience shortcut, used internally

#define ANODE   netnode(areasCode)
#define ANODE2  netnode(cb->areasCode)


// Special tag used to mark long comments

#define AREA_LONG_COMMENT_TAG   0x01020304L     // internal


//--------------------------------------------------------------------------
// Helper functions. Should not be called directly!

#define AREA_HELPER_DEFINITIONS(decl) \
decl void    ida_export areacb_t_zero             (areacb_t *);\
decl void    ida_export areacb_t_terminate        (areacb_t *);\
decl void    ida_export areacb_t_save             (areacb_t *);\
decl bool    ida_export areacb_t_link             (areacb_t *,const char *file, const char *name, int infosize);\
decl bool    ida_export areacb_t_create           (areacb_t *,const char *file,const char *name,uint infosize);\
decl void    ida_export areacb_t_kill             (areacb_t *);\
decl bool    ida_export areacb_t_create_area      (areacb_t *,area_t *info);\
decl bool    ida_export areacb_t_update           (areacb_t *,area_t *info);\
decl area_t *ida_export areacb_t_get_area         (areacb_t *,ea_t ea);\
decl area_t *ida_export areacb_t_getn_area        (areacb_t *,unsigned int n);\
decl int     ida_export areacb_t_get_area_num     (areacb_t *,ea_t ea);\
decl ea_t    ida_export areacb_t_prepare_to_create(areacb_t *,ea_t start,ea_t end);\
decl int     ida_export areacb_t_get_next_area    (areacb_t *,ea_t ea);\
decl int     ida_export areacb_t_get_prev_area    (areacb_t *,ea_t ea);\
decl bool    ida_export areacb_t_del_area         (areacb_t *,ea_t ea, bool delcmt);\
decl bool    ida_export areacb_t_may_start_at     (areacb_t *,uint n,ea_t newstart);\
decl bool    ida_export areacb_t_may_end_at       (areacb_t *,uint n,ea_t newend);\
decl bool    ida_export areacb_t_set_start        (areacb_t *,uint n,ea_t newstart);\
decl bool    ida_export areacb_t_set_end          (areacb_t *,uint n,ea_t newend);\
decl bool    ida_export areacb_t_resize_areas     (areacb_t *,uint n,ea_t newstart);\
decl uint    ida_export areacb_t_get_area_qty     (areacb_t *);\
decl area_t *ida_export areacb_t_choose_area      (areacb_t *,int flags, int width, char *(idaapi*getl)(areacb_t *obj,ulong n,char *buf), const char *title, int icon, int x0,int y0,int x1,int y1, const char * const *popup_menus, ea_t defea);\
decl area_t *ida_export areacb_t_choose_area2     (areacb_t *,int flags, int ncol, const int *widths, void (idaapi*getl)(areacb_t *obj,ulong n,char * const *arrptr), const char *title, int icon, int x0,int y0,int x1,int y1, const char * const *popup_menus, ea_t defea);\
decl bool    ida_export areacb_t_set_area_cmt     (areacb_t *,const area_t *a, const char *cmt, bool repeatable);\
decl char   *ida_export areacb_t_get_area_cmt     (areacb_t *,const area_t *a, bool repeatable);\
decl int     ida_export areacb_t_move_areas       (areacb_t *,ea_t from,ea_t to, asize_t size, int (idaapi*area_mover)(area_t *a, adiff_t delta, void *ud), void *ud);\
decl void    ida_export areacb_t_make_hole        (areacb_t *,ea_t ea1, ea_t ea2, bool create_tail_area);\
decl int     ida_export areacb_t_for_all_areas    (areacb_t *,ea_t ea1, ea_t ea2, area_visitor_t *av, void *ud);

class areacb_t;
typedef int idaapi area_visitor_t(area_t *a, void *ud);
AREA_HELPER_DEFINITIONS(idaman)

//--------------------------------------------------------------------------
// Definition of areas control block. Each type of areas has its own
// control block. Access to all areas of one type is made using this
// control block.

class areacb_t
{
  AREA_HELPER_DEFINITIONS(friend)
// private definitions, I won't comment them thoroughly

  uval_t areasCode;             // code in the database
  ushort infosize;              // sizeof info for area
  area_t *lastInfo;             // pointer to the last area
  long lastNum;                 // the last area number (-1: no last area)
  sarray *sa;                   // sorted array on supvals
  area_t *cache[AREA_CACHE_SIZE];
  void allocate(const char *file, bool del_other); // allocate memory for lastInfo
  area_t *search(ea_t EA, long *n);     // search for area
  area_t *readArea(ea_t EA);            // read area info from base
                                        // exit if any error (with a message)
  int  findCache(ea_t ea);
  area_t *addCache(area_t *a);          // returns allocated in cache Area
  void delCache(ea_t ea);
  void free_cache(void);
  ea_t find_nth_start(int n);
  void build_optimizer(void);
  void move_area_comment(ea_t oldea, ea_t newea);
  bool pack_and_write_area(area_t *a);   // returns 1-ok, 0-failure
  bool move_away(int cur,
                 ea_t ea1,
                 ea_t ea2,
                 bool create_tail_area);

public:

// Read callback: read area from the database.
// This function is called when a (possibly packed) area is read from the database.
//      packed - stream of packed bytes
//      ebd    - ptr to the end of the stream
//      a      - place to put unpacked version of the area
// This callback may be NULL.

  void (idaapi *read_cb)(const uchar *packed, const uchar *end, area_t *a);


// Write callback: write area to the database.
// This function is called when an area is about to be writed to the database.
// It may pack the the area to the stream of bytes.
//      a       - area to be written
//      packbuf - buffer to hold packed version of the area
//      packend - ptr to the end of packbuf
// Returns: number of bytes in the packed form
// This callback may be NULL.

  size_t (idaapi *write_cb)(const area_t *a,uchar *packbuf, uchar *packend);


// Destroy callback: remove an area from the internal cache.
// This function is called when an area is freed from the cache.
// This callback may be NULL.

  void (idaapi *delcache_cb)(area_t *a);


// The following three callbacks are used in open_areas_window() function.
// When the user presses Ctrl-E key the following callback is called
// to edit the selected area.
// This callback may be NULL.

  int (idaapi *edit_cb)(area_t *a);


// Callback to handle "Del" keystroke in open_areas_window() function
// This callback may be NULL.

  int (idaapi *kill_cb)(area_t *a);


// Callback to handle "Ins" keystroke in open_areas_window() function
// This callback may be NULL.

  int (idaapi *new_cb)(void);


// Constructor. Initialized area control block. You need to link
// area control block to existing area information in Btree (link) or
// to create a new area information structure in Btree (create).

  void zero(void)               { areacb_t_zero(this); }
  areacb_t(void)                { zero(); }


// Destructor. Flushes all information to Btree, deallocates caches and
// unlinks area control block from Btree.

  void terminate(void)          { areacb_t_terminate(this); }
  ~areacb_t(void)               { terminate(); }


// Flush area control block to Btree.

   void save(void)   { areacb_t_save(this); }


// Link area control block to Btree. Allocate cache, etc.
// Btree should contain information about the specified areas.
// After calling this function you may work with areas.
//      file  - name of input file being disassembled.
//              Doesn't matter if useva==0.
//              This parameter is used to build name of the file
//              with the virtual array.
//      name  - name of area information in Btree.
//              The name should start with "$ " (yes, including a space)
//              You may use any name you like. For example, area control
//              block keeping information about separation of program regions
//              to different output files might be:
//              "$ here is info about output file areas"
//      infosize- size of a structure with actual area information
//              (size of class based on class area_t)
// This function properly terminates work with the previous area control
// block in Btree if the current class was used to access information.
//
// returns:1-ok,0-failure (no such node in Btree)

  bool link(const char *file,            // Access to existing areas
            const char *name,
            int infosize)
        { return areacb_t_link(this, file, name, infosize); }


// Create area information node in Btree.
// This function usually is used when a new file is loaded into the database.
// See link() for explanations of input parameteres.
// This function properly terminates work with the previous area control
// block in Btree if the current class was used to access information.
//
// returns:1-ok
//         0-failure (Btree already contains node with the specified name)

  bool create(const char *file,const char *name,uint infosize)
        { return areacb_t_create(this, file, name, infosize); }


// Delete area information in Btree.
// All information about the current type of areas is deleted.
// Deallocate cache.

  void kill(void)        { areacb_t_kill(this); }


// Create an area.
// The new area should not overlap with existing ones.
//      info  - structure containing information about a new area
//              startEA and endEA specify address range.
//              startEA should be lower than endEA
// returns 1-ok,0-failure, area overlaps with another area or bad address range.

  bool create_area(area_t *info)                // Create new area
        { return areacb_t_create_area(this, info); }


// Update information about area in Btree.
// This function can't change startEA and endEA fields.
// Its only purpose is to update additional characteristics of the area.

  bool update(area_t *info)                     // Update info about area_t.
        { return areacb_t_update(this, info); }


// Get pointer to area structure by address
//      ea - any address in the area
// returns NULL: no area occupies the specified address
//         otherwise returns pointer to area structure.

  area_t *get_area(ea_t ea)                   // Get area by EA
        { return areacb_t_get_area(this, ea); }


// Get pointer to area structure by its number
//      n - number of area (0..get_area_qty()-1)
// returns NULL: the specified area doesn't exist.
//         otherwise returns pointer to area structure.

  area_t *getn_area(unsigned int n)            // Get n-th area (0..n)
        { return areacb_t_getn_area(this, n); }


// Get number of area by address
//      ea - any address in the area
// returns -1: no area occupies the specified address
//         otherwise returns number of the specified area (0..get_area_qty()-1)

  int get_area_num(ea_t ea)           // Get area number. -1 - no such area
        { return areacb_t_get_area_num(this, ea); }


// Prepare to create a new area
// This function checks whether the new area overlap with an existing one
// and trims an existing area to make enough address range for the creation
// of the new area. If the trimming of the existing area doesn't make enough
// room for the new area, the existing area is simply deleted.
//      start   - start address of the new area
//      end     - end   address of the new area
// returns: an adjusted end address for the new area. The end address may
//          require some adjustment if another (next) area exists and occupies
//          some addresses from (start..end) range. In this case we don't
//          delete the existing area but adjust end address of the new area.

  ea_t prepare_to_create(ea_t start,ea_t end)// Prepare to create an area (sEA,eEA)
                                                 //  - trim previous area
                                                 //  - return max allowed end of area
        { return areacb_t_prepare_to_create(this, start, end); }


// Get number of the next area.
// This function returns number of the next (higher in the addressing space)
// area.
//      ea - any address in the program
// returns -1: no (more) areas
//         otherwise returns number in the range (0..get_area_qty()-1)

  int get_next_area(ea_t ea)         // Return next area for EA
                                                // -1 - no (more) areas
        { return areacb_t_get_next_area(this, ea); }


// Get number of the previous area.
// This function returns number of the previous (lower in the addressing space)
// area.
//      ea - any address in the program
// returns -1: no (more) areas
//         otherwise returns number in the range (0..get_area_qty()-1)

  int get_prev_area(ea_t ea)                  // Returns prev area for EA
                                                // Returns -1 if not found
        { return areacb_t_get_prev_area(this, ea); }


// Delete an area.
//      ea     - any address in the area
//      delcmt - delete area comments
//               you may choose not to delete comments if you want to
//               create a new area with the same start address immediately.
//               In this case the new area will inherit old area comments.
// returns 1-ok,0-failure (no such area)

  bool del_area(ea_t ea, bool delcmt=true)          // Delete area
        { return areacb_t_del_area(this, ea, delcmt); }


// Check if the specified area may start at the specified address.
// This function checks whether the specified area can be changed so
// that its start address would be 'newstart'.
//      n        - number of area to check
//      newstart - new start address for the area
// returns: 1-yes, it can
//          0-no
//                the specified area doesn't exist
//                new start address is higher or equal to end address
//                the area would overlap with another area

  bool may_start_at(uint n,ea_t newstart)      // can the area have 'newstart'
        { return areacb_t_may_start_at(this, n, newstart); }


// Check if the specified area may end at the specified address.
// This function checks whether the specified area can be changed so
// that its end address would be 'newend'.
//      n        - number of area to check
//      newend   - new end address for the area
// returns: 1-yes, it can
//          0-no
//                the specified area doesn't exist
//                new end address is lower or equal to start address
//                the area would overlap with another area

  bool may_end_at(uint n,ea_t newend)          // can the area have 'newend'
        { return areacb_t_may_end_at(this, n, newend); }



// Change start address of the area
//      n        - number of area to change
//      newstart - new start address for the area
// This function doesn't modify other areas.
// returns: 1-ok, area is changed
//          0-failure
//                the specified area doesn't exist
//                new start address is higher or equal to end address
//                the area would overlap with another area

  bool set_start(uint n, ea_t newstart)
        { return areacb_t_set_start(this, n, newstart); }


// Change end address of the area
//      n        - number of area to change
//      newend   - new end address for the area
// This function doesn't modify other areas.
// returns: 1-ok, area is changed
//          0-failure
//                the specified area doesn't exist
//                new end address is lower or equal to start address
//                the area would overlap with another area

  bool set_end(uint n, ea_t newend)
        { return areacb_t_set_end(this, n, newend); }


// Make a hole at the specified address by deleting or modifying
// existing areas
//      ea1, ea2 - range to clear
//      create_tail_area - in the case if there is a big area overlapping
//              the specified range, should it be divided in two areas?
//              if 'false', then it will be truncated and the tail
//              will be left without any covering area

  void make_hole(ea_t ea1, ea_t ea2, bool create_tail_area)
        { areacb_t_make_hole(this, ea1, ea2, create_tail_area); }


// Resize adjacent areas simultaneously
//      n        - number of area to change
//                 The adjacent (previous) area will be trimmed or expanded
//                 if it exists and two areas are contiguous.
//                 Otherwise this function behaves like set_start() function.
// returns: 1-ok,0-failure

  bool resize_areas(uint n,ea_t newstart)      // Resize adjacent areas
        { return areacb_t_resize_areas(this, n, newstart); }


// Get number of areas.
// returns: number of areas ( >= 0 )

  uint get_area_qty(void)                              // Get number of areas
        { return areacb_t_get_area_qty(this); }


// Let the user choose an area. (1-column chooser)
// This function displays a window with a list of areas
// and allows the user to choose an area from the list.
//      flags - see kernwin.hpp for choose() flags description and callbacks usage
//      width - width of the window
//      getl  - callback function to get text representation of an area
//                      obj - pointer to area control block
//                      n   - (number of area + 1). if n==0 then getl() should
//                            return text of a header line.
//                      buf - buffer for the text representation
//              getl() should return pointer to text representation string
//              (not nesessarily the same pointer as 'buf')
//      title - title of the window.
//      icon  - number of icon to display
//      defea - address which points to the default area. The cursor will be
//              position to this area.
//      (x0,y0,x1,y1) - window position on the screen
//                      -1 values specify default window position
//                      (txt:upper left corner of the screen)
//                      (gui:centered on the foreground window)
//      popup_menus - default is insert, delete, edit, refresh
// returns: NULL - the user pressed Esc.
//      otherwise - pointer to the selected area.

  area_t *choose_area(int flags,
                      int width,
                      char *(idaapi*getl)(areacb_t *obj,ulong n,char *buf),
                      const char *title,
                      int icon,
                      int x0=-1,int y0=-1,int x1=-1,int y1=-1,
                      const char * const *popup_menus=NULL,
                      ea_t defea=BADADDR)
  {
    return areacb_t_choose_area(this, flags, width, getl, title, icon,
                        x0, y0, x1, y1, popup_menus, defea);
  }


// Let the user choose an area. (n-column chooser)
// This function displays a window with a list of areas
// and allows the user to choose an area from the list.
//      flags - see kernwin.hpp for choose() flags description and callbacks usage
//      ncol  - number of columns
//      widths- widths of each column in characters (may be NULL)
//      getl  - callback function to get text representation of an area
//                      obj - pointer to area control block
//                      n   - (number of area + 1). if n==0 then getl() should
//                            return text of a header line.
//                      arrptr - array of buffers for the text representation
//      title - title of the window.
//      icon  - number of icon to display
//      defea - address which points to the default area. The cursor will be
//              position to this area.
//      (x0,y0,x1,y1) - window position on the screen
//                      -1 values specify default window position
//                      (txt:upper left corner of the screen)
//                      (gui:centered on the foreground window)
//      popup_menus - default is insert, delete, edit, refresh
// returns: NULL - the user cancelled the selection
//     otherwise - pointer to the selected area.

  area_t *choose_area2(int flags,
                      int ncol,
                      const int *widths,
                      void (idaapi*getl)(areacb_t *obj,ulong n,char * const *arrptr),
                      const char *title,
                      int icon,
                      int x0=-1,int y0=-1,int x1=-1,int y1=-1,
                      const char * const *popup_menus=NULL,
                      ea_t defea=BADADDR)
  {
    return areacb_t_choose_area2(this, flags, ncol, widths, getl, title, icon,
                        x0, y0, x1, y1, popup_menus, defea);
  }


// Find previous gap in areas.
// This function finds a gap between areas. Only enabled addresses
// (see bytes.hpp for explanations on addressing) are used in the search.
//      ea - any linear address
// returns: BADADDR - no previous gap is found
//      otherwise returns maximal address in the previous gap

  ea_t find_prev_gap(ea_t ea);        // find prev/next gaps in the enabled addresses


// Find next gap in areas.
// This function finds a gap between areas. Only enabled addresses
// (see bytes.hpp for explanations on addressing) are used in the search.
//      ea - any linear address
// returns: BADADDR - no next gap is found
//      otherwise returns start address of the next gap

  ea_t find_next_gap(ea_t ea);        // if not found, returns BADADDR


// Set area comment.
// This function sets area comment.
//      a          - pointer to area structure (may be NULL)
//      repratable - 0: set regular comment
//                   1: set repeatable comment

  bool set_area_cmt(const area_t *a, const char *cmt, bool repeatable)
        { return areacb_t_set_area_cmt(this, a, cmt, repeatable); }


// Delete area comment.
// Each area may have its comment (a function may have a comment, for example)
// This function deletes such a comment
//      a          - pointer to area structure (may be NULL)
//      repratable - 0: delete regular comment
//                   1: delete repeatable comment

  void del_area_cmt(const area_t *a, bool repeatable)
                { set_area_cmt(a, "", repeatable); }


// Get area comment.
// This function gets area comment.
//      a          - pointer to area structure (may be NULL)
//      repratable - 0: get regular comment
//                   1: get repeatable comment
// The caller must qfree() the result.

  char *get_area_cmt(const area_t *a, bool repeatable)
        { return areacb_t_get_area_cmt(this, a, repeatable); }


// Move area information to the specified addresses
// Returns: 0 if ok, otherwise the code returned by area_mover

  int move_areas(ea_t from,
                 ea_t to,
                 asize_t size,
                 int (idaapi *area_mover)(area_t *a, adiff_t delta, void *ud)=NULL,
                 void *ud=NULL)
        { return areacb_t_move_areas(this, from, to, size, area_mover, ud); }

// Call a function for all areas in the specified range
// Stop the enumeration if the function returns non-zero
// Returns: 0 if all areas were visited, otherwise the code returned
//          by the callback

  int for_all_areas(ea_t ea1, ea_t ea2, area_visitor_t *av, void *ud)
        { return areacb_t_for_all_areas(this, ea1, ea2, av, ud); }

};

#pragma pack(pop)
#endif  // _AREA_HPP
