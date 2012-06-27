#ifndef VM_HPP
#define VM_HPP
#pragma pack(push, 1)           // IDA uses 1 byte alignments!

/*

        Class "Virtual Memory"

        This class is used by "Virtual Array" class.
        It should never be used directly.

        Created at 25-Oct-90 by Ilfak Guilfanov.

*/

#include <stdio.h>              // we need declaration of FILE *

typedef ushort vm_pagenum_t;
const vm_pagenum_t VM_BADPAGE = vm_pagenum_t(-1);

#ifndef declare
#define declare(x,y)    x ## y
#endif
#define declareVM(type)  declare(VM,type)

#define implementVM(type)                                                    \
class declareVM(type) : public VM                                            \
{                                                                            \
public:                                                                      \
  type *vread(vm_pagenum_t pagenum) { return (type *)VM::vread(pagenum); }   \
  type *valloc(vm_pagenum_t pagenum) { return (type *)VM::valloc(pagenum); } \
  void vwrite(type *buffer) { VM::vwrite(buffer); }                          \
  void vfree(type *buffer) { VM::vfree(buffer); }                            \
  bool lock(type *buffer) { return VM::lock(buffer); }                       \
  void unlock(type *buffer) { VM::unlock(buffer); }                          \
  vm_pagenum_t pagenum(type *buffer) { return VM::pagenum(buffer); }         \
  void setpage(type *buffer, vm_pagenum_t n){ VM::setpage(buffer,n); }       \
  bool swap(type *buf1, type *buf2){ return VM::swap(buf1,buf2); }           \
}

const int HASH_SIZE  = 512;

#define DATALEN 1

class VM_page
{
  friend class VM;
  VM_page *hash_next;   // Next in Hash List    DON'T MOVE hash_next !!!
  VM_page *time_prev;   // Prev by time
  VM_page *time_next;   // Next by time
  vm_pagenum_t Number;  // Page Number
  char dirty;           // Is page dirty ?
  char locked;          // Is page locked ?
  char data[DATALEN];

  friend VM_page *PagePtr(void *buf);
};

inline VM_page *PagePtr(void *buf)
  { return  (VM_page *)((char *)buf - sizeof(VM_page) + DATALEN); }

enum VMerror {  NoClose,NoSeek,NoRead,
                NoWrite,NoLock,CantWrite,VMfatal };

void idaapi VmDefaultHandler(VM *v,VMerror code,vm_pagenum_t);
extern void (idaapi *VMerrorHandler)(VM *v,VMerror code,vm_pagenum_t pagenum);

class VM
{
public:

        int linkTo(     int ALLpages,           // Max num of pages in memory
                        const char *file,       // File name
                        bool RWmode,            // Mode for open
                        size_t psize);          // Size of pages
                                                // returns eOk-ok,else -1

        VM(void)
        {
          handle = NULL;
          filename = NULL;
          Core = NULL;
        }

        friend bool isOk(VM *v)         { return v->handle != NULL; }
        size_t PageSize(void)           { return Psize; }

        ~VM(void);

        void vflush(void);               // sync memory to file
        void vclose(void);               // flush & forget memory contents
                                        // (except locked pages)
        void saveOne(void);             // save one page to disk

        bool lock(vm_pagenum_t pagenum)
        {
          if ( locks > 0 )
          {
            VM_page *found = searchhash(pagenum);
            if ( found == NULL )
              VMerrorHandler(this, NoLock, pagenum);
            if ( !found->locked ) locks--;
            found->locked++;
            return true;
          }
          return false;
        }
        void unlock(vm_pagenum_t pagenum)
        {
          VM_page *found = searchhash(pagenum);
          if ( found == NULL )
            VMerrorHandler(this, NoLock, pagenum);
          if ( found->locked > 0 )
          {
            found->locked--;
            if ( !found->locked ) locks++;
          }
        }

        void vchsize(ulong size);         // set VM file size, 0 - Ok

        const char *file(void) const
          { return filename; }

//---------------------- End of PUBLIC definitions ----------------------*/

protected:
        void *vread(vm_pagenum_t pagenum)        { return _read(pagenum, true); }
        // Alloc buffer for NEW page
        void *valloc(vm_pagenum_t pagenum)       { return _read(pagenum, false); }
        void vwrite(void *buffer)        { PagePtr(buffer)->dirty = true; }
        void  vfree(void *buffer);       // Free page (don't flush it)
        bool lock(void *buffer)
        {
          if ( locks > 0 )
          {
            VM_page *ptr = PagePtr(buffer);
            if ( !ptr->locked ) locks--;
            ptr->locked++;
            return true;
          }
          return false;
        }
        void unlock(void *buffer)
        {
          VM_page *ptr = PagePtr(buffer);
          if ( ptr->locked > 0 )
          {
            ptr->locked--;
            if ( !ptr->locked ) locks++;
          }
        }
        vm_pagenum_t pagenum(void *buffer) { return PagePtr(buffer)->Number; }
        void setpage(void *buffer, vm_pagenum_t newnum);
        bool swap(void *buf1,void *buf2);        // Swap two buffers
                                                 // Returns 0 if pages are locked
private:

  size_t Psize;         // page size
  int locks;            // number of available locks
  FILE *handle;         // Handle for file
  VM_page *hashtable[HASH_SIZE];
  VM_page *head;                // By time
  void *Core;                   // START of our MEMORY
  char *filename;

        size_t PageLength(void)
                { return sizeof(VM_page) - DATALEN + Psize; }
        VM_page *AddOne(VM_page *ptr)
                { return (VM_page *)((char *)ptr + PageLength()); }
        void DoWrite(VM_page *P);
        void DoRead (VM_page *P);

        void *_read(vm_pagenum_t pagenum, bool readflag);

        VM_page *searchhash(vm_pagenum_t pagenum);
        void inserthash(VM_page *ptr);
        void removehash(vm_pagenum_t pagenum);
};

#pragma pack(pop)
#endif // VM_HPP
