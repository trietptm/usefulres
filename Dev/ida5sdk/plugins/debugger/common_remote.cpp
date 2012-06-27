
#include <err.h>

char input_file_path[QMAXPATH];
bool debug_debugger;
vector<exception_info_t> exceptions;

#include "common_remote.hpp"

//----------------------------------------------------------------------
// Display a system error message. This function always returns false
bool deberr(const char *format, ...)
{
  if ( debug_debugger )
  {
    int code = SYSTEM_SPECIFIC_ERRNO;
    va_list va;
    va_start(va, format);
    rpc_svmsg(0, format, va);
    va_end(va);
    msg(": %s\n", winerr(code));
  }
  return false;
}

//--------------------------------------------------------------------------
static bool check_input_file_crc32(ulong orig_crc)
{
  if ( orig_crc == 0 ) return true; // the database has no crc
  linput_t *li = open_linput(input_file_path, false);
  if ( li == NULL ) return false;
  ulong crc = calc_file_crc32(li);
  close_linput(li);
  return crc == orig_crc;
}

//--------------------------------------------------------------------------
static const exception_info_t *find_exception(int code)
{
  for ( size_t i=0; i < exceptions.size(); i++ )
    if ( exceptions[i].code == code )
      return &exceptions[i];
  return NULL;
}

//--------------------------------------------------------------------------
bool get_exception_name(int code, char *buf, size_t bufsize)
{
  const exception_info_t *ei = find_exception(code);
  if ( ei != NULL )
  {
    qstrncpy(buf, ei->name.c_str(), bufsize);
    return true;
  }
  qsnprintf(buf, bufsize, "%08lX", code);
  return false;
}

//--------------------------------------------------------------------------
static char *get_event_name(event_id_t id)
{
  switch ( id )
  {
    case NO_EVENT:       return "NO_EVENT";
    case THREAD_START:   return "THREAD_START";
    case STEP:           return "STEP";
    case SYSCALL:        return "SYSCALL";
    case WINMESSAGE:     return "WINMESSAGE";
    case PROCESS_DETACH: return "PROCESS_DETACH";
    case PROCESS_START:  return "PROCESS_START";
    case PROCESS_ATTACH: return "PROCESS_ATTACH";
    case LIBRARY_LOAD:   return "LIBRARY_LOAD";
    case PROCESS_EXIT:   return "PROCESS_EXIT";
    case THREAD_EXIT:    return "THREAD_EXIT";
    case BREAKPOINT:     return "BREAKPOINT";
    case EXCEPTION:      return "EXCEPTION";
    case LIBRARY_UNLOAD: return "LIBRARY_UNLOAD";
    case INFORMATION:    return "INFORMATION";
  }
  return "???";
}

//--------------------------------------------------------------------------
char *debug_event_str(const debug_event_t *ev, char *buf, size_t bufsize)
{
  char *ptr = buf;
  char *end = buf + bufsize;
  ptr += qsnprintf(ptr, end-ptr, "%s ea=%a",
                get_event_name(ev->eid),
                ev->ea);
  switch ( ev->eid )
  {
    case PROCESS_START:  // New process started
    case PROCESS_ATTACH: // Attached to running process
    case LIBRARY_LOAD:   // New library loaded
      ptr += qsnprintf(ptr, end-ptr, " base=%a size=%a rebase=%a name=%s",
        ev->modinfo.base,
        ev->modinfo.size,
        ev->modinfo.rebase_to,
        ev->modinfo.name);
      break;
    case PROCESS_EXIT:   // Process stopped
    case THREAD_EXIT:    // Thread stopped
      ptr += qsnprintf(ptr, end-ptr, " exit_code=%d", ev->exit_code);
      break;
    case BREAKPOINT:     // Breakpoint reached
      ptr += qsnprintf(ptr, end-ptr, " hea=%a kea=%a", ev->bpt.hea, ev->bpt.kea);
      break;
    case EXCEPTION:      // Exception
      ptr += qsnprintf(ptr, end-ptr, " code=%x can_cont=%d ea=%a info=%s",
        ev->exc.code,
        ev->exc.can_cont,
        ev->exc.ea,
        ev->exc.info);
      break;
    case LIBRARY_UNLOAD: // Library unloaded
    case INFORMATION:    // User-defined information
      APPEND(ptr, end, ev->info);
      break;
    default:
      break;
  }
  qsnprintf(ptr, end-ptr, " pid=%d tid=%d handled=%d",
         ev->pid,
         ev->tid,
         ev->handled);
  return buf;
}

//--------------------------------------------------------------------------
char *debug_event_str(const debug_event_t *ev)
{
  static char buf[MAXSTR];
  return debug_event_str(ev, buf, sizeof(buf));
}

//--------------------------------------------------------------------------
static memory_info_t *old_mi = NULL;
static int old_n = 0;

static bool same_as_oldmemcfg(memory_info_t *mi, int n)
{
  if ( n == old_n )
  {
    if ( (mi != NULL) == (old_mi != NULL) )
    {
      if ( mi != NULL )
      {
        for ( int i=0; i < n; i++ )
        {
          if ( mi[i].startEA !=     old_mi[i].startEA
            || mi[i].endEA   !=     old_mi[i].endEA
            || mi[i].perm    !=     old_mi[i].perm
            || strcmp(mi[i].name,   old_mi[i].name) != 0
            || strcmp(mi[i].sclass, old_mi[i].sclass) != 0 )
          {
//            msg("%s:%s %a %a %d\n", mi[i].name, mi[i].sclass, mi[i].startEA, mi[i].endEA, mi[i].perm);
//            msg("%s:%s %a %a %d\n", old_mi[i].name, old_mi[i].sclass, old_mi[i].startEA, old_mi[i].endEA, old_mi[i].perm);
            return false;
          }
        }
      }
//      msg("memcfg the same n=%d\n", n);
      return true;
    }
  }
//  msg("%d %d\n", n, old_n);
  return false;
}

//--------------------------------------------------------------------------
static void save_oldmemcfg(memory_info_t *mi, int n)
{
  qfree(old_mi);
  if ( n == 0 )
  {
    old_mi = NULL;
    old_n = 0;
  }
  else
  {
    size_t size = n * sizeof(memory_info_t);
    old_mi = (memory_info_t *)qalloc(size);
    if ( old_mi == NULL ) nomem("save_old_memcfg");
    memcpy(old_mi, mi, size);
    old_n = n;
  }
}

//--------------------------------------------------------------------------
void idaapi remote_set_exception_info(const exception_info_t *table, int qty)
{
  exceptions.clear();
  for ( int i=0; i < qty; i++ )
    exceptions.push_back(*table++);
}
