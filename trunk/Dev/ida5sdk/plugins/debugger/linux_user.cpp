/*
 *  This is a userland linux debugger module
 *
 *  Functions unique for Linux
 *
 *  It can be compiled by gcc
 *
 */

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/user.h>

#include <pro.h>
#include <fpro.h>
#include <err.h>
#include <ida.hpp>
#include <idp.hpp>
#include <idd.hpp>
#include <name.hpp>
#include <bytes.hpp>
#include <loader.hpp>
#include <diskio.hpp>
#include "symelf.hpp"

#include <algorithm>
#include <vector>
#include <deque>
#include <map>
#include <set>

using std::vector;
using std::pair;
using std::make_pair;
#define INVALID_HANDLE_VALUE (-1)
typedef ulong DWORD;
typedef int HANDLE;

#include "idarpc.hpp"
#include "deb_pc.hpp"

#include "pc_remote.cpp"
#include "common_remote.cpp"

//--------------------------------------------------------------------------
//
//      DEBUGGER INTERNAL DATA
//
//--------------------------------------------------------------------------
// processes list information
typedef std::vector<process_info_t> processes_t;
static processes_t processes;

// debugged process information
static std::string process_path;
static HANDLE      process_handle = INVALID_HANDLE_VALUE;
static HANDLE      thread_handle  = INVALID_HANDLE_VALUE;

static bool is_dll;             // Is dynamic library?
static bool exited;             // Did the process exit?

// image information
struct image_info_t
{
  image_info_t() : base(BADADDR), imagesize(0) {}
  image_info_t(ea_t _base, ulong _imagesize, string _name) : base(_base), imagesize(_imagesize), name(_name) { }
  ea_t base;
  ulong imagesize;
  string name;
};

typedef std::map<ea_t, image_info_t> images_t; // key: image base address
static images_t dlls; // list of loaded DLLs
static images_t images; // list of detected PE images
static images_t thread_areas; // list of areas related to threads
static images_t class_areas;  // list of areas related to class names

typedef std::set<ea_t> easet_t;         // set of addresses
static easet_t dlls_to_import;          // list of dlls to import information from

// thread information
struct thread_info_t
{
  thread_info_t(int t, int count)
    : tid(t), suspend_count(count), child_signum(0), single_step(false),
      pending_sigstop(false) {}
  int tid;
  int suspend_count;
  int child_signum;
  bool single_step;
  bool pending_sigstop;
};

typedef std::map<HANDLE, thread_info_t> threads_t; // (tid -> info)
static threads_t threads;

typedef std::deque<debug_event_t> eventlist_t;
static eventlist_t events;

static std::set<ea_t> bpts;     // breakpoint list

static FILE *mapfp = NULL;      // map file handle
static int mem_handle = -1;     // memory file handle

static bool blocked;            // Is the process blocked (must say PTRACE_CONT)?
static int num_pending;         // Number of pending events to be processed by the ui
static bool attaching;          // Handling events linked to PTRACE_ATTACH, don't run the program

static bool set_hwbpts(int hThread);
static int get_memory_info(memory_info_t **areas, int *qty, bool suspend);
static bool _sure_resume_thread(thread_info_t &ti);
//--------------------------------------------------------------------------
static const char *get_ptrace_name(__ptrace_request request)
{
  switch ( request )
  {
    case PTRACE_TRACEME:    return "PTRACE_TRACEME";   /* Indicate that the process making this request should be traced.
                                                          All signals received by this process can be intercepted by its
                                                          parent, and its parent can use the other `ptrace' requests.  */
    case PTRACE_PEEKTEXT:   return "PTRACE_PEEKTEXT";  /* Return the word in the process's text space at address ADDR.  */
    case PTRACE_PEEKDATA:   return "PTRACE_PEEKDATA";  /* Return the word in the process's data space at address ADDR.  */
    case PTRACE_PEEKUSER:   return "PTRACE_PEEKUSER";  /* Return the word in the process's user area at offset ADDR.  */
    case PTRACE_POKETEXT:   return "PTRACE_POKETEXT";  /* Write the word DATA into the process's text space at address ADDR.  */
    case PTRACE_POKEDATA:   return "PTRACE_POKEDATA";  /* Write the word DATA into the process's data space at address ADDR.  */
    case PTRACE_POKEUSER:   return "PTRACE_POKEUSER";  /* Write the word DATA into the process's user area at offset ADDR.  */
    case PTRACE_CONT:       return "PTRACE_CONT";      /* Continue the process.  */
    case PTRACE_KILL:       return "PTRACE_KILL";      /* Kill the process.  */
    case PTRACE_SINGLESTEP: return "PTRACE_SINGLESTEP";/* Single step the process. This is not supported on all machines.  */
    case PTRACE_GETREGS:    return "PTRACE_GETREGS";   /* Get all general purpose registers used by a processes. This is not supported on all machines.  */
    case PTRACE_SETREGS:    return "PTRACE_SETREGS";   /* Set all general purpose registers used by a processes. This is not supported on all machines.  */
    case PTRACE_GETFPREGS:  return "PTRACE_GETFPREGS"; /* Get all floating point registers used by a processes. This is not supported on all machines.  */
    case PTRACE_SETFPREGS:  return "PTRACE_SETFPREGS"; /* Set all floating point registers used by a processes. This is not supported on all machines.  */
    case PTRACE_ATTACH:     return "PTRACE_ATTACH";    /* Attach to a process that is already running. */
    case PTRACE_DETACH:     return "PTRACE_DETACH";    /* Detach from a process attached to with PTRACE_ATTACH.  */
    case PTRACE_GETFPXREGS: return "PTRACE_GETFPXREGS";/* Get all extended floating point registers used by a processes. This is not supported on all machines.  */
    case PTRACE_SETFPXREGS: return "PTRACE_SETFPXREGS";/* Set all extended floating point registers used by a processes. This is not supported on all machines.  */
    case PTRACE_SYSCALL:    return "PTRACE_SYSCALL";   /* Continue and stop at the next (return from) syscall.  */
  }
  return "?";
}

//--------------------------------------------------------------------------
static long qptrace(__ptrace_request request, pid_t pid, void *addr, void *data)
{
  long code = ptrace(request, pid, addr, data);
  if ( request != PTRACE_PEEKTEXT
    && request != PTRACE_PEEKUSER
    && (request != PTRACE_POKETEXT
     && request != PTRACE_SETREGS
     && request != PTRACE_SETFPREGS
     && request != PTRACE_GETFPREGS
     && request != PTRACE_GETREGS
     || code != 0) )
  {
    int saved_errno = errno;
    debdeb("%s(%u, 0x%X, 0x%X) => 0x%X", get_ptrace_name(request), pid, addr, data, code);
    if ( code == -1 )
      deberr("");
    else
      debdeb("\n");
    errno = saved_errno;
  }
  return code;
}

//--------------------------------------------------------------------------
static thread_info_t &get_thread(thread_id_t tid)
{
  threads_t::iterator p = threads.find(tid);
  if ( p == threads.end() )
    error("get_thread: can not find thread %d\n", tid);
  return p->second;
}

//--------------------------------------------------------------------------
static ea_t get_ip(thread_id_t tid)
{
  const size_t eip_offset = qoffsetof(user, regs)
                          + qoffsetof(user_regs_struct, eip);

  return qptrace(PTRACE_PEEKUSER, tid, (void *)eip_offset, 0);
}

//--------------------------------------------------------------------------
static ulong get_dr(thread_id_t tid, int idx)
{
  uchar *offset = (uchar *)qoffsetof(user, u_debugreg) + idx*4;
  ulong value = qptrace(PTRACE_PEEKUSER, tid, (void *)offset, 0);
//  printf("dr%d => %08lX\n", idx, value);
  return value;
}

//--------------------------------------------------------------------------
static bool set_dr(thread_id_t tid, int idx, ulong value)
{
  uchar *offset = (uchar *)qoffsetof(user, u_debugreg) + idx*4;

  if ( value == BADADDR )
    value = 0;          // linux does not accept too high values
  return qptrace(PTRACE_POKEUSER, tid, offset, (void *)value) == 0;
}

//--------------------------------------------------------------------------
static pid_t qwait(int *status, bool hang)
{
  int flags = hang ? 0 : WNOHANG;
  pid_t pid = waitpid(-1, status, flags);
  if ( (pid == -1 || pid == 0) && errno == ECHILD )
  {
    pid = waitpid(-1, status, flags | __WCLONE);
    if ( pid != -1 && pid != 0 )
      debdeb("------------ __WCLONE %d\n", pid);
  }
  return pid;
}

//--------------------------------------------------------------------------
inline void enqueue_event(const debug_event_t &ev, bool in_front)
{
  if ( in_front )
    events.push_front(ev);
  else
    events.push_back(ev);
  num_pending++;
}

//--------------------------------------------------------------------------
// timeout in microseconds
// 0 - no timeout, return immediately
// -1 - wait forever
// returns: 1-ok, 0-failed
static int get_debug_event(debug_event_t *event, int timeout)
{
  debdeb("waiting, blocked=%d numpend=%d timeout=%d...\n", blocked, num_pending, timeout);
  if ( blocked )
    debdeb("ERROR: process is BLOCKED before qwait()!!!\n");
  int status;
  pid_t pid = qwait(&status, timeout == -1);
  if ( timeout > 0 && (pid == 0 || pid == -1) )
  {
    usleep(timeout);
    pid = qwait(&status, false);
  }
  if ( pid == -1 || pid == 0 )
    return false;

  thread_info_t &ti = get_thread(pid);
  event->pid     = process_handle;
  event->tid     = pid;
  event->ea      = get_ip(event->tid);
  event->handled = true;
  if ( WIFSTOPPED(status) )
  {
    blocked = true;
    int code = WSTOPSIG(status);
    debdeb("%s (stopped)\n", strsignal(code));
    if ( ti.pending_sigstop && code == SIGSTOP )
    {
      debdeb("got pending SIGSTOP, good!\n");
      ti.pending_sigstop = false;
      return false;
    }
    event->eid = EXCEPTION;
    event->exc.code     = code;
    event->exc.can_cont = true;
    event->exc.ea       = BADADDR;
    bool suspend;
    const exception_info_t *ei = find_exception(code);
    if ( ei != NULL )
    {
      qsnprintf(event->exc.info, sizeof(event->exc.info), "got %s signal (%s)", ei->name.c_str(), ei->desc.c_str());
      suspend = ei->break_on();
      if ( ei->handle() )
        code = 0;               // mask the signal
      else
        suspend = false;        // do not stop if the signal will be handled by the app
    }
    else
    {
      qsnprintf(event->exc.info, sizeof(event->exc.info), "got unknown signal #%d", code);
      suspend = true;
    }
    switch ( event->exc.code )
    {
      case SIGTRAP:
        {
          ulong dr6 = get_dr(event->tid, 6);
          bool hwbpt_found = false;
          for ( int i=0; i < MAX_BPT; i++ )
          {
            if ( dr6 & (1<<i) )  // Hardware breakpoint 'i'
            {
              if ( hwbpt_ea[i] == get_dr(event->tid, i) )
              {
                event->eid     = BREAKPOINT;
                event->bpt.hea = hwbpt_ea[i];
                event->bpt.kea = BADADDR;
                set_dr(event->tid, 6, 0); // Clear the status bits
                hwbpt_found = true;
                break;
              }
            }
          }
          if ( !hwbpt_found )
          {
            if ( bpts.find(event->ea-1) != bpts.end() )
            {
              event->eid     = BREAKPOINT;
              event->bpt.hea = BADADDR;
              event->bpt.kea = BADADDR;
              event->ea--;
            }
            else
            {
              event->eid = STEP;
            }
          }
        }
        code = 0;
        break;
      case __SIGRTMIN:          // LinuxThreads: restart thread
        event->eid = THREAD_START;
        break;
      case __SIGRTMIN+1:        // LinuxThreads: cancel thread
        event->eid = THREAD_EXIT;
        event->exit_code = 0;   // FIXME ????
        break;
      case __SIGRTMIN+2:        // LinuxThreads: debug thread
        debdeb("linuxthreads::debug_thread: don't know what to do\n!");
        break;
    }
    ti.child_signum = code;
    if ( !suspend && event->eid == EXCEPTION )
    {
      _sure_resume_thread(ti);
      return false;
    }
  }
  else if ( WIFSIGNALED(status) )
  {
    debdeb("%s (terminated)\n", strsignal(WSTOPSIG(status)));
    event->eid = PROCESS_EXIT;
    event->exit_code = WSTOPSIG(status);
    exited = true;
  }
  else
  {
    debdeb("%d (exited)\n", WEXITSTATUS(status));
    event->eid = PROCESS_EXIT;
    event->exit_code = WEXITSTATUS(status);
    exited = true;
  }
  debdeb("low got event: %s, signum=%d\n", debug_event_str(event), ti.child_signum);
  ti.single_step = false;
  return true;
}

//--------------------------------------------------------------------------
// generate LIBRARY_LOAD events
static void gen_libload_events(void)
{
  memory_info_t *areas;
  int qty;
  if ( get_memory_info(&areas, &qty, false) > 0 )
    qfree(areas);
}

//--------------------------------------------------------------------------
int idaapi remote_get_debug_event(debug_event_t *event, bool ida_is_idle)
{
  if ( event == NULL )
    return false;

  while ( true )
  {
    // are there any pending events?
    if ( !events.empty() )
    {
      // temporary hack?
      // we must return LIBRARY_LOAD events before anything else
      if ( blocked && events.front().eid != STEP )
        gen_libload_events();
      // get the first event and return it
      *event = events.front();
      events.pop_front();
      if ( event->eid == PROCESS_ATTACH )
        attaching = false;        // finally attached to it
      debdeb("GDE1: %s\n", debug_event_str(event));
      return true;
    }

    debug_event_t ev;
    if ( !get_debug_event(&ev, ida_is_idle ? TIMEOUT * 1000 : 0) )
      break;
    enqueue_event(ev, false);
  }
  return false;
}

//--------------------------------------------------------------------------
// Make sure that the thread is suspended
static bool _sure_suspend_thread(thread_info_t &ti)
{
  if ( !blocked )
  {
    debug_event_t ev;
    if ( get_debug_event(&ev, 0) )      // don't wait
    {
      ti.single_step = false;
      enqueue_event(ev, false);         // got an event, the process is suspended
      debdeb("process is suspended since we got an event\n");
    }
    else if ( !blocked )                // another SIGSTOP might have arrived?
    {
      debdeb("kill SIGSTOP %d\n", ti.tid);
      if ( kill(ti.tid, SIGSTOP) != 0 )
      {
        deberr("kill(%d)", ti.tid);
        return false;
      }
      ti.pending_sigstop = true;
      if ( get_debug_event(&ev, -1) )   // wait forever, return an event
      {
        debdeb("did not get SIGSTOP, will get it later hopefully\n");
        enqueue_event(ev, false);
      }
    }
  }
  ti.suspend_count++;
  return true;
}

inline void sure_suspend_thread(pair<const HANDLE, thread_info_t> &i)
{
  _sure_suspend_thread(i.second);
}

//--------------------------------------------------------------------------
// Note: this function just resets the actions of sure_suspend_thread
// If the thread was already suspended before calling sure_suspend_thread
// then it will stay in the suspended state
static bool _sure_resume_thread(thread_info_t &ti)
{
  if ( ti.suspend_count > 0 )
    ti.suspend_count--;

  if ( ti.suspend_count == 0 && num_pending == 0 && blocked && !attaching )
  {
    __ptrace_request request = ti.single_step ? PTRACE_SINGLESTEP : PTRACE_CONT;
    if ( qptrace(request, ti.tid, 0, (void *)ti.child_signum) != 0 )
      return false;
    blocked = false;
  }
  return true;
}

inline void sure_resume_thread(pair<const HANDLE, thread_info_t> &i)
{
  _sure_resume_thread(i.second);
}

//--------------------------------------------------------------------------
inline void suspend_all_threads(void)
{
  for_each(threads.begin(), threads.end(), sure_suspend_thread);
}

//--------------------------------------------------------------------------
inline void resume_all_threads(void)
{
  for_each(threads.begin(), threads.end(), sure_resume_thread);
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_continue_after_event(const debug_event_t *event)
{
  if ( event == NULL )
    return false;

  debdeb("%d: continue after event %s\n", num_pending, debug_event_str(event));
  num_pending--;

  thread_info_t &t = get_thread(event->tid);
  if ( event->eid != THREAD_START
    && event->eid != THREAD_EXIT
    && event->eid != LIBRARY_LOAD
    && event->eid != LIBRARY_UNLOAD
    && (event->eid != EXCEPTION || event->handled) )
  {
//    debdeb("event->id=%d, erasing child_signum\n", event->id);
    t.child_signum = 0;
  }
  return _sure_resume_thread(t);
}

//--------------------------------------------------------------------------
static int _read_memory(ea_t ea, void *buffer, int size, bool suspend=false)
{
  if ( exited || process_handle == INVALID_HANDLE_VALUE ) return -1;

//  debdeb("READ MEMORY: START\n");
  // stop all threads before accessing the process memory
  if ( suspend )
    suspend_all_threads();
  if ( exited ) return -1;

  int read_size = pread64(mem_handle, buffer, size, ea);
//  debdeb("pread64 %d:%x:%d => %d\n", mem_handle, ea, size, read_size);
  if ( read_size != size )
  {
    uchar *ptr = (uchar *)buffer;
    read_size = 0;
    while ( size > 0 )
    {
      const int shift = ea & 3;
      int part = shift;
      if ( part == 0 ) part = sizeof(int);
      if ( part > size ) part = size;
      int v = qptrace(PTRACE_PEEKTEXT, process_handle, (void *)(ulong)(ea-shift), 0);
      if ( errno != 0 )
        break;
      if ( part == sizeof(int) )
      {
        *(int*)ptr = v;
      }
      else
      {
        v >>= shift*8;
        for ( int i=0; i < part; i++ )
        {
          ptr[i] = uchar(v);
          v >>= 8;
        }
      }
      ptr  += part;
      ea   += part;
      size -= part;
    }
  }

  if ( suspend )
    resume_all_threads();
//  debdeb("READ MEMORY: END\n");
  return read_size > 0 ? read_size : 0;
}

//--------------------------------------------------------------------------
static int _write_memory(ea_t ea, const void *buffer, int size, bool suspend=false)
{
  if ( exited || process_handle == INVALID_HANDLE_VALUE ) return -1;

//  debdeb("WRITE MEMORY: START, blocked: %d, #threads: %d\n", blocked, threads.size());
  // stop all threads before accessing the process memory
  if ( suspend )
    suspend_all_threads();
  if ( exited ) return -1;

  if ( size == 1 )      // might be a breakpoint add/del
  {
    if ( size >= sizeof(bpt_code) && memcmp(buffer, bpt_code, sizeof(bpt_code)) == 0 )
    {
      bpts.insert(ea);
    }
    else if ( bpts.find(ea) != bpts.end() )
    {
      bpts.erase(ea);
    }
  }

  int ok = size;
  const uchar *ptr = (const uchar *)buffer;
  errno = 0;
  while ( size > 0 )
  {
    const int shift = ea & 3;
    int part = shift;
    if ( part == 0 ) part = sizeof(int);
    if ( part > size ) part = size;
    int word = *(int *)ptr;
    if ( part != sizeof(int) )
    {
      int old = qptrace(PTRACE_PEEKTEXT, process_handle, (void *)(ulong)(ea-shift), 0);
      if ( errno != 0 )
      {
        ok = 0;
        break;
      }
      unsigned int mask = ~0;
      mask >>= (sizeof(int) - part)*8;
      mask <<= shift*8;
      word <<= shift*8;
      word &= mask;
      word |= old & ~mask;
    }
    if ( qptrace(PTRACE_POKETEXT, process_handle, (void *)(ulong)(ea-shift), (void *)word) == -1 )
    {
      ok = 0;
      break;
    }
    ptr  += part;
    ea   += part;
    size -= part;
  }

  if ( suspend )
    resume_all_threads();
//  debdeb("WRITE MEMORY: END\n");
  return ok;
}

//--------------------------------------------------------------------------
ssize_t idaapi remote_write_memory(ea_t ea, const void *buffer, size_t size)
{
  return _write_memory(ea, buffer, size, true);
}

//--------------------------------------------------------------------------
ssize_t idaapi remote_read_memory(ea_t ea, void *buffer, size_t size)
{
  return _read_memory(ea, buffer, size, true);
}

//--------------------------------------------------------------------------
static ulong add_dll(image_info_t &ii)
{
  dlls.insert(make_pair(ii.base, ii));
  dlls_to_import.insert(ii.base);
  return ii.imagesize;
}

//--------------------------------------------------------------------------
struct name_info_t
{
  ea_t imagebase;
  vector<ea_t> addrs;
  vector<char *> names;
};

static void idaapi add_elf_symbol(ea_t ea, const char *name, void *ud)
{
  name_info_t &ni = *(name_info_t *)ud;
  ni.addrs.push_back(ea+ni.imagebase);
  ni.names.push_back(qstrdup(name));
//  printf("add name %x -> %s\n", ea+ni.imagebase, name);
}

static bool import_dll(const char *dllname, linput_t *li, ea_t imagebase, name_info_t &ni)
{
  ni.imagebase = imagebase;
  return load_elf_symbols(li, add_elf_symbol, &ni);
}

//--------------------------------------------------------------------------
static bool import_dll_to_database(ea_t imagebase, name_info_t &ni)
{
  images_t::iterator p = dlls.find(imagebase);
  if ( p == dlls.end() )
    error("import_dll_to_database: can't find dll name");
  const char *dllname = p->second.name.c_str();

  linput_t *li = open_linput(dllname, false);
  if ( li == NULL )
    return false;

  // prepare nice name prefix for exported functions names
  char prefix[MAXSTR];
  qstrncpy(prefix, qbasename(dllname), sizeof(prefix));
  char *ptr = strrchr(prefix, '.');
  if ( ptr != NULL ) *ptr = '\0';

  bool ok = import_dll(prefix, li, imagebase, ni);
  close_linput(li);
  return ok;
}

//--------------------------------------------------------------------------
void idaapi remote_stopped_at_debug_event(void)
{
  // we will take advantage of this event to import information
  // about the exported functions from the loaded dlls
  name_info_t ni;
  easet_t::iterator p;
  for ( p=dlls_to_import.begin(); p != dlls_to_import.end(); )
  {
    import_dll_to_database(*p, ni);
    dlls_to_import.erase(p++);
  }
  rpc_set_debug_names(&ni.addrs[0], &ni.names[0], ni.addrs.size());
  for ( int i=0; i < ni.names.size(); i++ )
    qfree(ni.names[i]);
}

//--------------------------------------------------------------------------
static void cleanup(void)
{
  process_handle = INVALID_HANDLE_VALUE;
  is_dll = false;
  exited = false;

  threads.clear();
  dlls.clear();
  dlls_to_import.clear();
  images.clear();
  thread_areas.clear();
  class_areas.clear();
  events.clear();
  if ( mapfp != NULL )
  {
    qfclose(mapfp);
    mapfp = NULL;
  }
  if ( mem_handle != -1 )
  {
    close(mem_handle);
    mem_handle = -1;
  }
  blocked = false;
  num_pending = 0;
  attaching = false;
  bpts.clear();

  save_oldmemcfg(NULL, 0);
}

//--------------------------------------------------------------------------
//
//      DEBUGGER INTERFACE FUNCTIONS
//
//--------------------------------------------------------------------------
inline const char *skipword(const char *ptr)
{
  while ( !isspace(*ptr) && *ptr !='\0' )
    ptr++;
  return ptr;
}

//--------------------------------------------------------------------------
static void refresh_process_list(void)
{
  processes.clear();
  static const char command[] = "/bin/ps ax";
  FILE *ps = popen(command, "r");
  if ( ps != NULL )
  {
    char line[MAXSTR];
    qfgets(line, sizeof(line), ps);     // skip the header
    while ( qfgets(line, sizeof(line), ps) )
    {
      line[strlen(line)-1] = '\0';        // remove last '\n'
      process_info_t info;
      const char *ptr = skipSpaces(line);
      for ( int i=0; i < 4; i++ )
        ptr = skipSpaces(skipword(ptr));
      if ( strcmp(ptr, command) == 0 ) continue;
      info.pid = atoi(line);
      qstrncpy(info.name, ptr, sizeof(info.name));
      processes.push_back(info);
    }
    qfclose(ps);
  }
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
// input is valid only if n==0
int idaapi remote_process_get_info(int n, const char *input, process_info_t *info)
{
  if ( n == 0 )
  {
    qstrncpy(input_file_path, input, sizeof(input_file_path));
    refresh_process_list();
  }

  if ( n < 0 || n >= processes.size() )
    return false;

  if ( info != NULL )
    *info = processes[n];
  return true;
}

//--------------------------------------------------------------------------
static char **prepare_arguments(const char *path, const char *args)
{
  int i = 0;
  char **argv = NULL;
  while ( 1 )
  {
    string s;
    if ( i == 0 )
    {
      s = path;
    }
    else
    {
      while ( isspace(*args) ) args++;
      if ( *args == '\0' ) break;
      char quote = (*args == '"' || *args == '\'') ? *args++ : 0;
      while ( (quote ? *args != quote : !isspace(*args)) && *args != '\0' )
      {
        if ( *args == '\\' && args[1] != '\0' )
          args++;
        s += *args++;
      }
    }
    argv = (char **)qrealloc(argv, sizeof(char *)*(i+2));
    if ( argv == NULL ) nomem("prepare_arguments");
    argv[i++] = qstrdup(s.c_str());
  }
  argv[i] = NULL;
}

//--------------------------------------------------------------------------
static FILE *open_proc_file(const char *fname)
{
  char path[QMAXPATH];
  qsnprintf(path, sizeof(path), "/proc/%u/%s", process_handle, fname);
  FILE *fp = fopenRT(path);
  if ( fp == NULL )
    error("Could not open %s: %s\n", path, winerr(errno));
  return fp;
}

//--------------------------------------------------------------------------
// Returns the file name assciated with pid
static char *get_exec_fname(int pid, char *buf, size_t bufsize)
{
  char path[QMAXPATH];
  qsnprintf(path, sizeof(path), "/proc/%u/exe", pid);
  int len = readlink(path, buf, bufsize-1);
  if ( len > 0 )
    buf[len] = '\0';
  else
    qstrncpy(buf, path, bufsize);
  return buf;
}

//--------------------------------------------------------------------------
static bool handle_process_start(pid_t pid)
{
  process_handle = pid;
  threads.insert(make_pair(process_handle, thread_info_t(process_handle, 1)));
  int status;
  waitpid(process_handle, &status, 0); // (should succeed) consume SIGSTOP
  blocked = true;

  debug_event_t ev;
  ev.eid     = PROCESS_START;
  ev.pid     = process_handle;
  ev.tid     = process_handle;
  ev.ea      = BADADDR;
  ev.handled = true;
  get_exec_fname(process_handle, ev.modinfo.name, sizeof(ev.modinfo.name));
  ev.modinfo.base = BADADDR;
  ev.modinfo.size = 0;
  ev.modinfo.rebase_to = BADADDR;

  char fname[QMAXPATH];
  qsnprintf(fname, sizeof(fname), "/proc/%u/maps", process_handle);
  mapfp = fopenRT(fname);
  if ( mapfp == NULL )
    return false;               // if fails, the process did not start
//    error("Could not open %s: %s\n", fname, winerr(errno));

  qsnprintf(fname, sizeof(fname), "/proc/%u/mem", process_handle);
  mem_handle = open(fname, O_RDONLY|O_BINARY);
  if ( mem_handle == -1 )
    error("Could not open %s: %s\n", fname, winerr(errno));

  // find the executable base
  memory_info_t *miv;
  int qty;
  if ( get_memory_info(&miv, &qty, false) > 0 )
  {
    for ( int i=0; i < qty; i++ )
    {
      if ( strcmp(miv[i].name, ev.modinfo.name) == 0 )
      {
        ev.modinfo.rebase_to = miv[i].startEA;
        break;
      }
    }
    qfree(miv);
  }
  enqueue_event(ev, false);
  return true;
}

//--------------------------------------------------------------------------
int idaapi remote_start_process(const char *path,
                                const char *args,
                                const char *startdir,
                                int flags,
                                const char *input_path,
                                ulong input_file_crc32)
{
  // input file specified in the database does not exist
  if ( input_path[0] != '\0' && !qfileexist(input_path) )
    return -2;

  // temporary thing, later we will retrieve the real file name
  // based on the process id
  qstrncpy(input_file_path, input_path, sizeof(input_file_path));
  is_dll = (flags & DBG_PROC_IS_DLL) != 0;

  if ( !qfileexist(path) ) return -1;

  int mismatch = 0;
  if ( !check_input_file_crc32(input_file_crc32) )
    mismatch = CRC32_MISMATCH;

  if ( startdir[0] != '\0' && chdir(startdir) == -1 )
  {
    debdeb("chdir: %s\n", winerr(errno));
    return -1;
  }

  cleanup();

  int child_pid = fork();
  if ( child_pid == -1 )
  {
    debdeb("fork: %s\n", winerr(errno));
    return -1;
  }
  if ( child_pid == 0 ) // child
  {
    if ( qptrace(PTRACE_TRACEME, 0, 0, 0) == -1 )
      error("TRACEME: %s", winerr(errno));
    char **argv = prepare_arguments(path, args);
//    for ( char **ptr=argv; *ptr != NULL; ptr++ )
//      printf("ARG: %s\n", *ptr);
    for (int i = getdtablesize(); i > 2; i--) close(i); // free ida-resources
    int code = execv(path, argv);
    qprintf("execv: %s\n", winerr(errno));
    _exit(1);
  }
  else                  // parent
  {
    if ( !handle_process_start(child_pid) )
      return -1;
  }

  return 1 | mismatch;
}


//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_attach_process(process_id_t pid, int /*event_id*/)
{
  if ( qptrace(PTRACE_ATTACH, pid, NULL, NULL) == 0
    && handle_process_start(pid) )
  {
    gen_libload_events();

    // finally generate the attach event
    debug_event_t ev;
    ev.eid     = PROCESS_ATTACH;
    ev.pid     = process_handle;
    ev.tid     = process_handle;
    ev.ea      = get_ip(process_handle);
    ev.handled = true;
    get_exec_fname(process_handle, ev.modinfo.name, sizeof(ev.modinfo.name));
    ev.modinfo.base = BADADDR;
    ev.modinfo.size = 0;
    ev.modinfo.rebase_to = BADADDR;
    enqueue_event(ev, false);

    // block the process until all generated events are processed
    attaching = true;
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_detach_process(void)
{
  if ( qptrace(PTRACE_DETACH, process_handle, NULL, NULL) == 0 )
  {
    debug_event_t ev;
    ev.eid     = PROCESS_DETACH;
    ev.pid     = process_handle;
    ev.tid     = process_handle;
    ev.ea      = BADADDR;
    ev.handled = true;
    enqueue_event(ev, false);
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_prepare_to_pause_process(void)
{
  debdeb("remote_prepare_to_pause_process\n");
  for ( threads_t::iterator p=threads.begin(); p != threads.end(); ++p )
    kill(p->first, SIGSTOP);
  return true;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_exit_process(void)
{
  if ( blocked )
  {
    blocked = false;
    return qptrace(PTRACE_KILL, process_handle, 0, (void *)SIGKILL) == 0;
  }
  else
  {
    return kill(process_handle, SIGKILL) == 0;
  }
}

//--------------------------------------------------------------------------
// Set hardware breakpoints for one thread
static bool set_hwbpts(int hThread)
{
  uchar *offset = (uchar *)qoffsetof(user, u_debugreg);

  bool ok = set_dr(hThread, 0, hwbpt_ea[0])
         && set_dr(hThread, 1, hwbpt_ea[1])
         && set_dr(hThread, 2, hwbpt_ea[2])
         && set_dr(hThread, 3, hwbpt_ea[3])
         && set_dr(hThread, 6, 0)
         && set_dr(hThread, 7, dr7);
//  printf("set_hwbpts: DR0=%08lX DR1=%08lX DR2=%08lX DR3=%08lX DR7=%08lX => %d\n",
//         hwbpt_ea[0],
//         hwbpt_ea[1],
//         hwbpt_ea[2],
//         hwbpt_ea[3],
//         dr7,
//         ok);
  return ok;
}

//--------------------------------------------------------------------------
static bool refresh_hwbpts(void)
{
  for ( threads_t::iterator p=threads.begin(); p != threads.end(); ++p )
    if ( !set_hwbpts(p->second.tid) )
      return false;
  return true;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_add_bpt(bpttype_t type, ea_t ea, int len)
{
  if ( type == BPT_SOFT )
  {
    static const uchar cc[] = { 0xCC };
    return remote_write_memory(ea, &cc, 1) == 1;
  }

  int i = find_hwbpt_slot(ea);    // get slot number
  if ( i != -1 )
  {
    hwbpt_ea[i] = ea;

    int lenc;                               // length code used by the processor
    if ( len == 1 ) lenc = 0;
    if ( len == 2 ) lenc = 1;
    if ( len == 4 ) lenc = 3;

    dr7 |= (1 << (i*2));            // enable local breakpoint
    dr7 |= (type << (16+(i*2)));    // set breakpoint type
    dr7 |= (lenc << (18+(i*2)));    // set breakpoint length

    return refresh_hwbpts();
  }
  return false;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_del_bpt(ea_t ea, const uchar *orig_bytes, int len)
{
  if ( orig_bytes != NULL )
    return remote_write_memory(ea, orig_bytes, len) == len;

  for ( int i=0; i < MAX_BPT; i++ )
  {
    if ( hwbpt_ea[i] == ea )
    {
      hwbpt_ea[i] = BADADDR;            // clean the address
      dr7 &= ~(3 << (i*2));             // clean the enable bits
      dr7 &= ~(0xF << (i*2+16));        // clean the length and type
      return refresh_hwbpts();
    }
  }
  return false;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_thread_get_sreg_base(thread_id_t tid, int sreg_value, ea_t *pea)
{
    return false;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_thread_suspend(thread_id_t tid)
{
  debdeb("remote_thread_suspend %d\n", tid);
  thread_info_t &t = get_thread(tid);
  return _sure_suspend_thread(t);
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_thread_continue(thread_id_t tid)
{
  debdeb("remote_thread_continue %d\n", tid);
  thread_info_t &t = get_thread(tid);
  return _sure_resume_thread(t);
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_thread_set_step(thread_id_t tid)
{
  thread_info_t &t = get_thread(tid);
  t.single_step = true;
  return true;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_thread_read_registers(thread_id_t tid, regval_t *values, int count)
{
  if ( values == NULL ) return 0;

  struct user_regs_struct regs;
  if ( qptrace(PTRACE_GETREGS, process_handle, 0, &regs) != 0 )
    return false;

  memset(values, 0, count * sizeof(regval_t)); // force null bytes at the end of floating point registers.
                                               // we need this to properly detect register modifications,
                                               // as we compare the whole regval_t structure !
  values[R_EAX   ].ival = ulong(regs.eax);
  values[R_EBX   ].ival = ulong(regs.ebx);
  values[R_ECX   ].ival = ulong(regs.ecx);
  values[R_EDX   ].ival = ulong(regs.edx);
  values[R_ESI   ].ival = ulong(regs.esi);
  values[R_EDI   ].ival = ulong(regs.edi);
  values[R_EBP   ].ival = ulong(regs.ebp);
  values[R_ESP   ].ival = ulong(regs.esp);
  values[R_EIP   ].ival = ulong(regs.eip);
  values[R_EFLAGS].ival = ulong(regs.eflags);
  values[R_CS    ].ival = ulong(regs.xcs);
  values[R_DS    ].ival = ulong(regs.xds);
  values[R_ES    ].ival = ulong(regs.xes);
  values[R_FS    ].ival = ulong(regs.xfs);
  values[R_GS    ].ival = ulong(regs.xgs);
  values[R_SS    ].ival = ulong(regs.xss);

  struct user_fpregs_struct i387;
  if ( qptrace(PTRACE_GETFPREGS, process_handle, 0, &i387) != 0 )
    return false;

  for (int i = 0; i < FPU_REGS_COUNT; i++)
  {
    uchar *fpu_float = (uchar *)i387.st_space;
    fpu_float += i * 10;
    *(long double *)values[R_ST0+i].fval = *(long double *)fpu_float;
  }
  values[R_CTRL].ival = ulong(i387.cwd);
  values[R_STAT].ival = ulong(i387.swd);
  values[R_TAGS].ival = ulong(i387.twd);

  return true;
}

//--------------------------------------------------------------------------
// 1-ok, 0-failed
int idaapi remote_thread_write_register(thread_id_t tid, int reg_idx, const regval_t *value)
{
  if ( value == NULL )
    return false;

  bool ret = false;

  if (reg_idx < R_ST0 || reg_idx >= R_CS)
  {
    // get the original context, to be sure to not modify unwanted registers
    struct user_regs_struct regs;
    if ( qptrace(PTRACE_GETREGS, process_handle, 0, &regs) != 0 )
      return false;

    switch (reg_idx)
    {
      case R_CS:     regs.xcs    = value->ival; break;
      case R_DS:     regs.xds    = value->ival; break;
      case R_ES:     regs.xes    = value->ival; break;
      case R_FS:     regs.xfs    = value->ival; break;
      case R_GS:     regs.xgs    = value->ival; break;
      case R_SS:     regs.xss    = value->ival; break;
      case R_EAX:    regs.eax    = value->ival; break;
      case R_EBX:    regs.ebx    = value->ival; break;
      case R_ECX:    regs.ecx    = value->ival; break;
      case R_EDX:    regs.edx    = value->ival; break;
      case R_ESI:    regs.esi    = value->ival; break;
      case R_EDI:    regs.edi    = value->ival; break;
      case R_EBP:    regs.ebp    = value->ival; break;
      case R_ESP:    regs.esp    = value->ival; break;
      case R_EIP:    regs.eip    = value->ival; break;
      case R_EFLAGS: regs.eflags = value->ival; break;
    }
    ret = qptrace(PTRACE_SETREGS, tid, 0, &regs) != -1;
  }
  else // FPU related register
  {
    // get the original context, to be sure to not modify unwanted registers
    struct user_fpregs_struct i387;
    if ( qptrace(PTRACE_GETFPREGS, process_handle, 0, &i387) != 0 )
      return false;

    if (reg_idx >= R_ST0+FPU_REGS_COUNT) // FPU status registers
    {
      switch (reg_idx)
      {
        case R_CTRL:   i387.cwd = value->ival; break;
        case R_STAT:   i387.swd = value->ival; break;
        case R_TAGS:   i387.twd = value->ival; break;
      }
    }
    else // FPU floating point register
    {
      uchar *fpu_float = (uchar *)i387.st_space;
      fpu_float += (reg_idx-R_ST0) * 10;
      *(long double *)fpu_float = *(long double *)&value->fval;
    }
    ret = qptrace(PTRACE_SETFPREGS, tid, 0, &i387) != -1;
  }
  return ret;
}

//--------------------------------------------------------------------------
// find a dll in the memory information array
static const memory_info_t *find_dll(const char *name,
                                     const memory_info_t *miv,
                                     int n)
{
  for ( int i=0; i < n; i++ )
    if ( strcmp(name, miv[i].name) == 0 )
      return miv + i;
  return NULL;
}

//--------------------------------------------------------------------------
static void handle_dll_movements(const memory_info_t *miv, int n)
{
  // unload missing dlls
  images_t::iterator p;
  for ( p=dlls.begin(); p != dlls.end(); )
  {
    if ( find_dll(p->second.name.c_str(), miv, n) == NULL )
    {
      debug_event_t ev;
      ev.eid     = LIBRARY_UNLOAD;
      ev.pid     = process_handle;
      ev.tid     = process_handle;
      ev.ea      = BADADDR;
      ev.handled = true;
      qstrncpy(ev.info, p->second.name.c_str(), sizeof(ev.info));
      enqueue_event(ev, true);
      dlls.erase(p++);
    }
    else
    {
      ++p;
    }
  }
  // load new dlls
  for ( int i=0; i < n; i++ )
  {
    // ignore unnamed dlls
    if ( miv[i].name[0] == '\0' ) continue;
    // put full path name into 'input_file_path'
    if ( input_file_path[0] == '\0' )
      get_exec_fname(process_handle, input_file_path, sizeof(input_file_path));
    // ignore the input file
    if ( strcmp(miv[i].name, input_file_path) == 0 ) continue;

    // ignore if dll already exists
    ea_t base = miv[i].startEA;
    p = dlls.find(base);
    if ( p != dlls.end() ) continue;

    // ignore memory chunks which do not correspond to an ELF header
    char magic[4];
    if ( remote_read_memory(base, &magic, 4) != 4 ) continue;
    if ( memcmp(magic, "\x7F\x45\x4C\x46", 4) != 0 ) continue;

    debdeb("found new dll: %x %s\n", base, miv[i].name);

    debug_event_t ev;
    ev.eid      = LIBRARY_LOAD;
    ev.pid     = process_handle;
    ev.tid     = process_handle;
    ev.ea      = base;
    ev.handled = true;
    qstrncpy(ev.modinfo.name, miv[i].name, sizeof(ev.modinfo.name));
    ev.modinfo.base = base;
    ev.modinfo.size = 0;                // we don't know the exact size
    ev.modinfo.rebase_to = BADADDR;
    enqueue_event(ev, true);

    image_info_t ii(ev.modinfo.base,
                    ev.modinfo.size,
                    ev.modinfo.name);
    add_dll(ii);
  }
}

//--------------------------------------------------------------------------
static bool read_mapping(ea_t *ea1,
                         ea_t *ea2,
                         char *perm,
                         ea_t *offset,
                         char *device,
                         ulonglong *inode,
                         char *filename)
{
  int code = qfscanf(mapfp, "%a-%a %s %a %s %llx",
		    ea1, ea2, perm, offset, device, inode);
  filename[0] = '\0';
  if ( code > 0 && code != EOF && *inode != 0 )
    code = qfscanf(mapfp, "%[^\n]\n", filename);
  else
    qfscanf(mapfp, "\n");
  return code != 0 && code != EOF;
}

//--------------------------------------------------------------------------
static int get_memory_info(memory_info_t **areas, int *qty, bool suspend)
{
  if ( suspend )
    suspend_all_threads();
  if ( exited ) return -1;

  memory_info_t *miv = NULL;
  int n = 0;

  ea_t ea1, ea2, offset;
  ulonglong inode;
  char perm[8], device[8], fname[QMAXPATH];
  rewind(mapfp);
  while ( read_mapping(&ea1, &ea2, perm, &offset, device, &inode, fname) )
  {
    // for some reason linux lists some areas twice
    // ignore them
    int i;
    for ( i=0; i < n; i++ )
      if ( miv[i].startEA == ea1 )
        break;
    if ( i != n )
      continue;

    memory_info_t mi;
    mi.startEA = ea1;
    mi.endEA   = ea2;
    qstrncpy(mi.name, trim(skipSpaces(fname)), sizeof(mi.name));
    if ( strchr(perm, 'r') != NULL ) mi.perm |= SEGPERM_READ;
    if ( strchr(perm, 'w') != NULL ) mi.perm |= SEGPERM_WRITE;
    if ( strchr(perm, 'x') != NULL ) mi.perm |= SEGPERM_EXEC;

    miv = (memory_info_t *)qrealloc(miv, (n+1)*sizeof(memory_info_t));
    if ( miv == NULL )
      nomem("get_memory_info");
    miv[n++] = mi;
  }
  handle_dll_movements(miv, n);
  if ( suspend )
    resume_all_threads();
  *areas = miv;
  *qty = n;
  return 1;
}

//--------------------------------------------------------------------------
int idaapi remote_get_memory_info(memory_info_t **areas, int *qty)
{
  int code = get_memory_info(areas, qty, true);
  if ( code == 1 )
  {
    if ( same_as_oldmemcfg(*areas, *qty) )
    {
      qfree(*areas);
      code = -2;
    }
    else
    {
      save_oldmemcfg(*areas, *qty);
    }
  }
  return code;
}

//--------------------------------------------------------------------------
int idaapi remote_init(bool _debug_debugger)
{
  // remember if the input is a dll
  cleanup();
  cleanup_hwbpts();
  debug_debugger = _debug_debugger;
  return 3; // process_get_info, detach
}

//--------------------------------------------------------------------------
void idaapi remote_term(void)
{
  cleanup();
  cleanup_hwbpts();
  input_file_path[0] = '\0';
  /* ... */
}

//--------------------------------------------------------------------------
bool idaapi thread_get_fs_base(thread_id_t tid, int reg_idx, ea_t *pea)
{
  qnotused(tid);
  qnotused(reg_idx);
  qnotused(pea);
  return false;
}

//--------------------------------------------------------------------------
int idaapi remote_ioctl(int fn, const void *in, size_t insize, void **, ssize_t *)
{
  if ( fn == 0 )  // chmod +x
  {
    char *fname = (char *)in;
    qstatbuf st;
    qstat(fname, &st);
    int mode = st.st_mode | S_IXUSR|S_IXGRP|S_IXOTH;
    chmod(fname, mode);
  }
  return 0;
}
