/*
       IDA Pro remote debugger server
*/

#include <pro.h>
#include <fpro.h>
#ifndef UNDER_CE
#include <signal.h>
#endif

static bool verbose = false;

#ifdef __NT__
#  if defined(__AMD64__)
#    define SYSTEM "Windows64"
#  elif defined(UNDER_CE)
#    define SYSTEM "WindowsCE"
#    define USE_ASYNC
#  else
#    define SYSTEM "Windows32"
#  endif
#  ifdef USE_ASYNC
#    define DEBUGGER_ID    DEBUGGER_ID_ARM_WINCE_USER
#  else
#    define socklen_t int
#    include <winsock2.h>
#    define DEBUGGER_ID    DEBUGGER_ID_X86_IA32_WIN32_USER
#  endif
#endif

#ifdef __LINUX__
#define SYSTEM "Linux"
#define DEBUGGER_ID    DEBUGGER_ID_X86_IA32_LINUX_USER
#include <sys/socket.h>
#include <netinet/in.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#define closesocket(s)           close(s)
#ifdef LIBWRAP
extern "C" const char *check_connection(int);
#endif // LIBWRAP
#endif // __LINUX__

//--------------------------------------------------------------------------
static FILE *channels[16] = { 0 };

static void close_all_channels(void)
{
  for ( int i=0; i < qnumber(channels); i++ )
    if ( channels[i] != NULL )
      qfclose(channels[i]);
  memset(channels, 0, sizeof(channels));
}

static int find_free_channel(void)
{
  for ( int i=0; i < qnumber(channels); i++ )
    if ( channels[i] == NULL )
      return i;
  return -1;
}

//--------------------------------------------------------------------------
#define REMOTE_DEBUGGER
#define DEBUGGER_SERVER
#include "idarpc.cpp"

#ifndef USE_ASYNC
static SOCKET listen_socket = INVALID_SOCKET;
//--------------------------------------------------------------------------
void neterr(const char *module)
{
  int code = irs_error(irs);
  qeprintf("%s: %s\n", module, winerr(code));
  exit(1);
}

//--------------------------------------------------------------------------
static void NT_CDECL shutdown_gracefully(int signum)
{
  qeprintf("got signal #%d\n", signum);
  if ( irs != NULL ) term_server_irs(irs);
  if ( listen_socket != INVALID_SOCKET )
    closesocket(listen_socket);
  _exit(1);
}
#endif

//--------------------------------------------------------------------------
static void handle_session(const char *password)
{
  lprintf("=========================================================\n"
         "Accepting incoming connection...\n");
  string open = prepare_rpc_packet(RPC_OPEN);
  append_long(open, IDD_INTERFACE_VERSION);
  append_long(open, DEBUGGER_ID);
  append_long(open, sizeof(ea_t));
  rpc_packet_t *rp = process_request(open, true);
  const uchar *answer = (uchar *)(rp+1);
  const uchar *end = answer + rp->length;
  bool send_response = true;
  bool ok = extract_long(&answer, end);
  if ( !ok )
  {
    lprintf("Incompatible IDA Pro version\n");
    send_response = false;
  }
  else if ( password != NULL )
  {
    char *pass = extract_str(&answer, end);
    if ( strcmp(pass, password) != '\0' )
    {
      lprintf("Bad password\n");
      ok = false;
    }
  }
  qfree(rp);

  if ( send_response )
  {
    poll_debug_events = false;
    has_pending_event = false;

    open = prepare_rpc_packet(RPC_OK);
    append_long(open, ok);
    send_request(open);

    if ( ok )
    {
      string cmd;
      qfree(process_request(cmd, true));
    }
  }
  network_error_code = 0;
  close_all_channels();
  lprintf("Closing incoming connection...\n");
}

//--------------------------------------------------------------------------
// For Pocket PC, we create a DLL
// This DLL should never exit(), of course, but just close the connection
#ifdef UNDER_CE

#include "rapi/rapi.h"

static bool in_use = false;
static uchar *ptr;

static int display_exception(int code, EXCEPTION_POINTERS *ep)
{
  CONTEXT &ctx = *(ep->ContextRecord);
  EXCEPTION_RECORD &er = *(ep->ExceptionRecord);
  char name[MAXSTR];
  get_exception_name(er.ExceptionCode, name, sizeof(name));

  // find our imagebase
  ptr = (uchar*)(size_t(ptr) & ~0xFFF); // point to the beginning of a page
  while ( !IsBadReadPtr(ptr, 2) )
    ptr -= 0x1000;

  msg("%08lX: debugger server %s (BASE %08lX)\n", ctx.Pc-(ulong)ptr, name, ptr);

  DEBUG_CONTEXT(ctx);
//  show_exception_record(er);
  return EXCEPTION_EXECUTE_HANDLER;
//  return EXCEPTION_CONTINUE_SEARCH;
}

//--------------------------------------------------------------------------
static DWORD calc_our_crc32(const char *fname)
{
  linput_t *li = open_linput(fname, false);
  DWORD crc32 = calc_file_crc32(li);
  close_linput(li);
  return crc32;
}

//--------------------------------------------------------------------------
extern "C"
{
BOOL WINAPI SetKMode(BOOL fMode);
DWORD WINAPI SetProcPermissions(DWORD newperms);
};

class get_permissions_t
{
  DWORD dwPerm;
  BOOL bMode;
public:
  get_permissions_t(void)
  {
    bMode = SetKMode(TRUE);
    dwPerm = SetProcPermissions(0xFFFFFFFF);
  }
  ~get_permissions_t(void)
  {
    SetProcPermissions(dwPerm);
    SetKMode(bMode);
  }
};

//--------------------------------------------------------------------------
extern "C" __declspec(dllexport)
int ida_server(DWORD dwInput, BYTE* pInput,
               DWORD* pcbOutput, BYTE** ppOutput,
               IRAPIStream* pStream)
{
  lprintf("IDA " SYSTEM " remote debug server. Version 1.%d.\n"
         "Copyright Datarescue 2004-2006\n", IDD_INTERFACE_VERSION);
  // check our crc32
  DWORD crc32 = calc_our_crc32((char *)pInput);
  DWORD dummy = 0;
  pStream->Write(&crc32, sizeof(crc32), &dummy);
  if ( dummy != sizeof(crc32) )
  {
ERR:
    pStream->Release();
//    lprintf("Debugger server checksum mismatch - shutting down\n");
    return ERROR_CRC;
  }
  DWORD ok;
  dummy = 0;
  pStream->Read(&ok, sizeof(ok), &dummy);
  if ( dummy != sizeof(ok) || ok != 1 )
    goto ERR;

  // only one instance is allowed
  if ( in_use )
  {
    static const char busy[] = "ERROR_BUSY";
    pStream->Write(busy, sizeof(busy), &dummy);
    pStream->Release();
    return ERROR_BUSY;
  }
  in_use = true;

  ptr = (uchar*)ida_server;
  __try
  {
    get_permissions_t all_permissions;
    irs = init_server_irs(pStream);
    if ( irs == NULL )
      return 0;
    handle_session(NULL);
  }
  __except ( display_exception(GetExceptionCode(), GetExceptionInformation()) )
  {
  }
  term_server_irs(irs);

  in_use = false;
  return 0;
}

#else
//--------------------------------------------------------------------------
// debugger remote server - TCP/IP mode
int main(int argc, char *argv[])
{
  const char *password = NULL;
  int port_number = DEBUGGER_PORT_NUMBER;
  lprintf("IDA " SYSTEM " remote debug server. Version 1.%d. Copyright Datarescue 2004-2006\n", IDD_INTERFACE_VERSION);
  while ( argc > 1 && (argv[1][0] == '-' || argv[1][0] == '/'))
  {
    switch ( argv[1][1] )
    {
      case 'p':
        port_number = atoi(&argv[1][2]);
        break;
      case 'P':
        password = argv[1] + 2;
        break;
      case 'v':
        verbose = true;
        break;
      default:
        error("usage: ida_remote [switches]\n"
              "  -p...  port number\n"
              "  -P...  password\n"
              "  -v     verbose\n");
    }
    argv++;
    argc--;
  }
  signal(SIGINT, shutdown_gracefully);
  signal(SIGSEGV, shutdown_gracefully);
//  signal(SIGPIPE, SIG_IGN);

  if ( !init_irs_layer() )
    neterr("init_sockets");
  listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  if ( listen_socket == -1 )
    neterr("socket");

  setup_irs((idarpc_stream_t*)listen_socket);

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port   = qhtons(short(port_number));
  if ( bind(listen_socket, (sockaddr *)&sa, sizeof(sa)) == SOCKET_ERROR )
    neterr("bind");
  if ( listen(listen_socket, SOMAXCONN) == SOCKET_ERROR )
    neterr("listen");
  lprintf("Listening on port #%u...\n", port_number);

  while ( true )
  {
    sockaddr_in sa;
    socklen_t salen = sizeof(sa);
    SOCKET rpc_socket = accept(listen_socket, (sockaddr *)&sa, &salen);
    if ( rpc_socket == -1 )
      neterr("accept");
#if defined(__LINUX__) && defined(LIBWRAP)
    const char *p;
    if((p = check_connection(rpc_socket)) != NULL) {
      fprintf(stderr,
              "ida-server CONNECTION REFUSED from %s (tcp_wrappers)\n", p);
      shutdown(rpc_socket, 2);
      close(rpc_socket);
      continue;
    }
#endif // defined(__LINUX__) && defined(LIBWRAP)

    irs = (idarpc_stream_t *)rpc_socket;
    handle_session(password);

    term_server_irs(irs);
    irs = NULL;
  }
}

#endif


