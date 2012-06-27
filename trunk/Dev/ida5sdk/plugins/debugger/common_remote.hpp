#define msg     smsg
#define warning swarning
#define error   serror

extern bool debug_debugger;

inline void debdeb(const char *format, ...)
{
  if ( debug_debugger )
  {
    va_list va;
    va_start(va, format);
    rpc_svmsg(0, format, va);
    va_end(va);
  }
}

bool deberr(const char *format, ...);
