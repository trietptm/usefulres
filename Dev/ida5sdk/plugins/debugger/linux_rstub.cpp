


#define REMOTE_DEBUGGER
#define DEBUGGER_CLIENT

char wanted_name[] = "Remote Linux debugger";
#define DEBUGGER_NAME  "linux"
#define PROCESSOR_NAME "metapc"
#define DEBUGGER_ID    DEBUGGER_ID_X86_IA32_LINUX_USER
#define DEBUGGER_FLAGS DBG_FLAG_REMOTE

#include "idarpc.cpp"
#include "pc_local.cpp"
#include "linux_local.cpp"
#include "common_local.cpp"

