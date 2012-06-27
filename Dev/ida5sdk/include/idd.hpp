/*
 *      Interactive disassembler (IDA).
 *      Copyright (c) 1990-98 by Ilfak Guilfanov.
 *      ALL RIGHTS RESERVED.
 *                              E-mail: ig@estar.msk.su, ig@datarescue.com
 *                              FIDO:   2:5020/209
 *
 */

#ifndef _IDD_HPP
#define _IDD_HPP
#pragma pack(push, 4)

//
//      This file contains definition of the interface to IDD modules
//      The interface consists of structures describing the target
//      debugged processor and a debugging API.


//      Versions history:
// 1    24.10.02        Initial version
// 2    17.09.03        FPU support
// 3    29.09.04        network error codes are added
// 4    15.10.04        breakpoint handling is improved
// 5    15.02.05        added remote file access functions
// 6    25.02.05        set_exception_info is added
// 7    21.03.05        structure alignment is changed to support ARM
// 8    13.04.05        added processor name field
// 9    23.05.05        added map_address

#define         IDD_INTERFACE_VERSION   9


//====================================================================
//
//                       Process and Threads
//

typedef int process_id_t;
typedef int thread_id_t;

#define NO_PROCESS        0xFFFFFFFF // No process
#define PROCESS_NO_THREAD 0          // No thread
                                     // in PROCESS_START this value
                                     // can be used to specify that
                                     // the main thread has not been created
                                     // It will be initializated later
                                     // by a THREAD_START event.

struct process_info_t
{
  process_id_t pid;
  char name[MAXSTR];
};

//====================================================================
//
//                          Registers
//

typedef unsigned char register_class_t; // Each register is associated to
                                        // a register class.
                                        // example: "segment", "mmx", ...

struct register_info_t
{
  const char *name;                   // Register full name
  ulong flags;                        // Register special features
#define REGISTER_READONLY 0x0001      //   the user can't modify the current value of this register
#define REGISTER_IP       0x0002      //   instruction pointer
#define REGISTER_SP       0x0004      //   stack pointer
#define REGISTER_FP       0x0008      //   frame pointer
#define REGISTER_ADDRESS  0x0010      // Register can contain an address
  register_class_t register_class;
  char dtyp;                          // Register size (dt_... constants)
  const char *const *bit_strings;     // Strings corresponding to each bit of the register
                                      // (NULL = no bit, same name = multi-bits mask)
  int bit_strings_default;            // Mask of default bits
};

//====================================================================
//
//                           Memory
//

struct memory_info_t : public area_t
{
  memory_info_t(void) : perm(0) { name[0] = '\0'; sclass[0] = '\0'; }
  uchar perm;                  // Memory area permissions (0-no information): see segment.hpp
  char name[64];               // Memory area name (null string => kernel will give an appropriate name)
  char sclass[64];             // Memory area class name (null string => kernel will give an appropriate name)
};

//====================================================================
//
//                         Debug events
//

enum event_id_t
{
  NO_EVENT       = 0x00000000, // Not an interesting event
  PROCESS_START  = 0x00000001, // New process started
  PROCESS_EXIT   = 0x00000002, // Process stopped
  THREAD_START   = 0x00000004, // New thread started
  THREAD_EXIT    = 0x00000008, // Thread stopped
  BREAKPOINT     = 0x00000010, // Breakpoint reached
  STEP           = 0x00000020, // One instruction executed
  EXCEPTION      = 0x00000040, // Exception
  LIBRARY_LOAD   = 0x00000080, // New library loaded
  LIBRARY_UNLOAD = 0x00000100, // Library unloaded
  INFORMATION    = 0x00000200, // User-defined information
                               // This event can be used to return empty information
                               // This will cause IDA to call get_debug_event()
                               // immediately once more
  SYSCALL        = 0x00000400, // Syscall (not used yet)
  WINMESSAGE     = 0x00000800, // Window message (not used yet)
  PROCESS_ATTACH = 0x00001000, // Attached to running process
  PROCESS_DETACH = 0x00002000, // Detached from process
/*
  SIGNAL,
  DEBUG_STRING
  ...
*/
};


// Those structures describe particular debug events

struct module_info_t
{
  char name[MAXSTR];    // full name of the module.
  ea_t base;            // module base address. if unknown pass BADADDR
  asize_t size;         // module size. if unknown pass 0
  ea_t rebase_to;       // if not BADADDR, then rebase the program to the specified address
};

struct e_breakpoint_t
{
  ea_t hea;             // Possible address referenced by hardware breakpoints
  ea_t kea;             // Address of the triggered bpt from the kernel's point
                        // of view (for some systems with special memory mappings,
                        // the triggered ea might be different from event ea).
                        // Use to BADADDR for flat memory model.
};

struct e_exception_t
{
  int code;          // Exception code
  bool can_cont;     // Execution of the process can continue after this exception?
  ea_t ea;           // Possible address referenced by the exception
  char info[MAXSTR]; // Exception message
};

// This structure is used only when detailed information
//   on a debug event is needed.
struct debug_event_t
{
  debug_event_t(void) : eid(NO_EVENT) {}
  event_id_t   eid;        // Event code (used to decipher 'info' union)
  process_id_t pid;        // Process where the event occured
  thread_id_t  tid;        // Thread where the event occured
  ea_t ea;                 // Address where the event occured
  bool handled;            // Is event handled by the debugger?
                           // (from the system's point of view)
  union
  {
    module_info_t modinfo;         // PROCESS_START, PROCESS_ATTACH, LIBRARY_LOAD
    int exit_code;                 // PROCESS_EXIT, THREAD_EXIT
    char info[MAXSTR];             // LIBRARY_UNLOAD (unloaded library name)
                                   // INFORMATION (will be displayed in the
                                   //              messages window if not empty)
    e_breakpoint_t bpt;            // BREAKPOINT
    e_exception_t exc;             // EXCEPTION
  };
};

// Hardware breakpoint types
typedef int bpttype_t;
const bpttype_t
  BPT_EXEC  =  0,             // Execute instruction
  BPT_WRITE =  1,             // Write access
  BPT_RDWR  =  3,             // Read/write access
  BPT_SOFT  =  4;             // Software breakpoint


// Exception information
struct exception_info_t
{
  int code;
  ulong flags;
#define EXC_BREAK  0x0001 // break on the exception
#define EXC_HANDLE 0x0002 // should be handled by the debugger?
  bool break_on(void) const { return (flags & EXC_BREAK) != 0; }
  bool handle(void) const { return (flags & EXC_HANDLE) != 0; }
  qstring name;         // Exception standard name
  qstring desc;         // Long message used to display info about the exception
  exception_info_t(void) {}
  exception_info_t(int _code, ulong _flags, const char *_name, const char *_desc)
    : code(_code), flags(_flags), name(_name), desc(_desc) {}
};

// Instruction operand information
struct idd_opinfo_t
{
  ea_t addr;            // operand address (BADADDR - no address)
  uval_t value;         // operand value
  bool modified;        // the operand is modified (written) by the instruction
};

// register value
struct regval_t
{
#ifndef __WATCOMC__     // bad compiler
  union
  {
#endif
    ulonglong ival;     // 8:  integer value
    ushort    fval[6];  // 12: floating point value in the internal representation (see ieee.h)
#ifndef __WATCOMC__
  };
#endif
  regval_t(void) : ival(~ulonglong(0)) {}
};

//====================================================================
//
//     This structure describes a debugger API module.
//     (functions needed to debug a process on a specific
//      operating system)
//
//     The address of this structure must be put into the 'dbg' variable by
//     the init() function of the debugger plugin

struct debugger_t
{
  int version;                        // Expected kernel version,
                                      //   should be IDD_INTERFACE_VERSION
  char *name;                         // Short debugger name like win32 or linux
  int id;                             // Debugger API module id
#define DEBUGGER_ID_X86_IA32_WIN32_USER              0 // Userland win32 processes (win32 debugging APIs)
// #define DEBUGGER_ID_X86_IA32_WIN32_KERNEL           // ?
// #define DEBUGGER_ID_X86_IA32_WIN32_USER_WINDBG      // Translate functions to Microsoft WinDBG commands
// #define DEBUGGER_ID_X86_IA32_WIN32_KERNEL_WINDBG    // "
// #define DEBUGGER_ID_X86_IA32_WIN32_USER_SOFTICE     // Forward functions to a SoftIce plugin ?
// #define DEBUGGER_ID_X86_IA32_WIN32_KERNEL_SOFTICE   // "
#define DEBUGGER_ID_X86_IA32_LINUX_USER              1 // Userland linux processes (ptrace())
// #define DEBUGGER_ID_X86_IA32_LINUX_KERNEL           // ?
// #define DEBUGGER_ID_X86_IA32_BOCHS                  // Forward functions to Bochs emulator ?
// #define DEBUGGER_ID_JAVA
#define DEBUGGER_ID_ARM_WINCE_USER                   2 // Windows CE ARM
// ...
  const char *processor;              // Required processor name
                                      // Used for instant debugging to load correct
                                      // processor module

  ulong flags;                              // Debugger module special features
#define DBG_FLAG_REMOTE      0x01           // Remote debugger (requires remote host name unless DBG_FLAG_NOHOST)
#define DBG_FLAG_NOHOST      0x02           // Remote debugger with does not require host name
                                            // (a unique device connected to the machine)
#define DBG_FLAG_FAKE_ATTACH 0x04           // PROCESS_ATTACH is a fake event
                                            // and does not suspend the execution
#define DBG_FLAG_HWDATBPT_ONE 0x08          // Hardware data breakpoints are
                                            // one byte size by default
  bool is_remote(void) const { return (flags & DBG_FLAG_REMOTE) != 0; }
  bool must_have_hostname(void) const
    { return (flags & (DBG_FLAG_REMOTE|DBG_FLAG_NOHOST)) == DBG_FLAG_REMOTE; }

  char*           *register_classes;        // Array of register class names
  int             register_classes_default; // Mask of default printed register classes
  register_info_t *registers;               // Array of registers
  int             registers_size;           // Number of registers

  int             memory_page_size;         // Size of a memory page

  const uchar     *bpt_bytes;               // Array of bytes for a breakpoint instruction
  int             bpt_size;                 // Size of this array

// Init/term (for local debuggers the parameters are ignored)

  bool (idaapi* init_debugger)(const char *hostname, int portnum, const char *password);

#if !defined(__WATCOMC__) && !defined(_MSC_VER)  // these compilers complain :(
  static const int default_port_number = 23946;
#define DEBUGGER_PORT_NUMBER debugger_t::default_port_number
#else
#define DEBUGGER_PORT_NUMBER 23946
#endif

  bool (idaapi* term_debugger)(void);

  // Return information about the n-th "compatible" running process.
  // If n is 0, the processes list is reinitialized.
  // 1-ok, 0-failed, -1-network error
  int (idaapi* process_get_info)(int n, process_info_t *info);

  // Start an executable to debug
  // 1 - ok, 0 - failed, -2 - file not found (ask for process options)
  // 1|CRC32_MISMATCH - ok, but the input file crc does not match
  // -1 - network error
  int (idaapi* start_process)(const char *path,
                              const char *args,
                              const char *startdir,
                              ulong input_file_crc32);
#define CRC32_MISMATCH  0x40000000      // crc32 mismatch bit

  // Attach to an existing running process
  // 1-ok, 0-failed, -1-network error
  // event_id should be equal to -1 if not attaching to a crashed process
  int (idaapi* attach_process)(process_id_t pid, int event_id);

  // Detach from the debugged process
  // 1-ok, 0-failed, -1-network error
  int (idaapi* detach_process)(void);

  // rebase database if the debugged program has been rebased by the system
  void (idaapi *rebase_if_required_to)(ea_t new_base);

  // Prepare to pause the process
  // This function will prepare to pause the process
  // Normally the next get_debug_event() will pause the process
  // If the process is sleeping then the pause will not occur
  // until the process wakes up. The interface should take care of
  // this situation.
  // If this function is absent, then it won't be possible to pause the program
  // 1-ok, 0-failed, -1-network error
  int (idaapi* prepare_to_pause_process)(void);

  // Stop the process.
  // 1-ok, 0-failed, -1-network error
  int (idaapi* exit_process)(void);

//-------------------------------------------
// The following functions manipulate events.
//
// Get a pending debug event and suspend the process
// returns: 1-got event, 0-no event, -1-error
// This function will be called regularly by IDA.
  int (idaapi* get_debug_event)(debug_event_t *event, bool ida_is_idle);

// Continue after handling the event
// 0-ok, 1-failed, -1-network error
  int (idaapi* continue_after_event)(const debug_event_t *event);

// Set exception handling
  void (idaapi* set_exception_info)(const exception_info_t *info, int qty);

// The following function will be called by the kernel each time
// when it has stopped the debugger process for some reason,
// refreshed the database and the screen.
// The debugger module may add information to the database if it wants.
// The reason for introducing this function is that when an event line
// LOAD_DLL happens, the database does not reflect the memory state yet
// and therefore we can't add information about the dll into the database
// in the get_debug_event() function.
// Only when the kernel has adjusted the database we can do it.
// Example: for imported PE DLLs we will add the exported function
// names to the database.
// This function pointer may be absent, i.e. NULL.
  void (idaapi *stopped_at_debug_event)(bool dlls_added);

  // The following functions manipulate threads.
  // 1-ok, 0-failed, -1-network error
  int (idaapi* thread_suspend) (thread_id_t tid); // Suspend a running thread
  int (idaapi* thread_continue)(thread_id_t tid); // Resume a suspended thread
  int (idaapi* thread_set_step)(thread_id_t tid); // Run one instruction in the thread

  // The following functions manipulate registers.
  // 1-ok, 0-failed, -1-network error
  int (idaapi* thread_read_registers)(thread_id_t tid, regval_t *values, int count); // Return current register values
  int (idaapi* thread_write_register)(thread_id_t tid, int reg_idx, const regval_t *value); // Write one register value
  // Get information about the base of a segment register
  // Currently used by the IBM PC module to resolve references like fs:0
  //   tid        - thread id
  //   sreg_value - value of the segment register (returned by get_reg_val())
  //   answer     - pointer to the answer. can't be NULL.
  // 1-ok, 0-failed, -1-network error
  int (idaapi* thread_get_sreg_base)(thread_id_t tid, int sreg_value, ea_t *answer);

//
// The following functions manipulate bytes in the memory.
//
  // Get information on the memory areas
  // Allocates and fills areas and qty
  // The caller should qfree() it
  // Returns: -2-no changes, 1-ok, 0-the kernel will try slow algorithm (not recommended!)
  // -1 means that the process does not exist anymore
  int (idaapi* get_memory_info)(memory_info_t **areas, int *qty);

  // Read process memory
  // Returns number of read bytes
  // 0 mean read error
  // -1 means that the process does not exist anymore
  ssize_t (idaapi* read_memory)(ea_t ea, void *buffer, size_t size);

  // Write process memory
  // Returns number of written bytes, -1-fatal error
  ssize_t (idaapi* write_memory)(ea_t ea, const void *buffer, size_t size);

//
// The following functions manipulate breakpoints.
//

  // Is it possible to set breakpoint?
  // Returns: BPT_...
  int (idaapi* is_ok_bpt)(bpttype_t type, ea_t ea, int len);
#define BPT_OK           0
#define BPT_INTERNAL_ERR 1
#define BPT_BAD_TYPE     2
#define BPT_BAD_ALIGN    3
#define BPT_BAD_ADDR     4
#define BPT_BAD_LEN      5
#define BPT_TOO_MANY     6
  // Add a breakpoint
  // Note: for software breakpoints, the kernel will save/restore
  // original byte values, the module should just write the bpt opcode.
  // 1-ok, 0-failed, -1-network error
  int (idaapi* add_bpt)(bpttype_t type, ea_t ea, int len);
  // Remove a breakpoint
  // Note: orig_bytes are used only for software breakpoints
  // to restore the old byte values overwritten by the breakpoint
  // For hardware breakpoints it must be NULL
  // 1-ok, 0-failed, -1-network error
  int (idaapi* del_bpt)(ea_t ea, const uchar *orig_bytes, int len);

// Open/close/read a remote file
  int  (idaapi* open_file)(const char *file, ulong *fsize, bool readonly); // -1-error
  void (idaapi* close_file)(int fn);
  ssize_t (idaapi* read_file)(int fn, ulong off, void *buf, size_t size);

// Map process address
// This function may be absent
//      ea    - address to map
// Returns: mapped address or BADADDR

  ea_t (idaapi *map_address)(ea_t ea);
};

#ifdef __BORLANDC__
#if sizeof(debugger_t) % 4
#error "Size of debugger_t is incorrect"
#endif
#endif

#pragma pack(pop)
#endif // _IDD_HPP
