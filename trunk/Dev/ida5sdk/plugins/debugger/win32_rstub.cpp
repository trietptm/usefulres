#define REMOTE_DEBUGGER
#define DEBUGGER_CLIENT

#ifdef __AMD64__
char wanted_name[] = "Remote Win64 debugger";
#else
char wanted_name[] = "Remote Win32 debugger";
#endif
#define DEBUGGER_NAME  "win32"
#define PROCESSOR_NAME "metapc"
#define DEBUGGER_ID    DEBUGGER_ID_X86_IA32_WIN32_USER
#define DEBUGGER_FLAGS DBG_FLAG_REMOTE

#include "idarpc.cpp"
#include "pc_local.cpp"
#include "win32_local.cpp"
#include "common_local.cpp"

