#ifndef __PluginEx_h__
#define __PluginEx_h__

#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/IGNORE:4078 /IGNORE:4089")
#pragma comment(linker,"/RELEASE")
#pragma comment(linker,"/SECTION:.text,EWR")
#pragma comment(linker,"/merge:.rdata=.data")
#pragma comment(linker,"/merge:.text=.data")
#if _MSC_VER >= 1000
#pragma comment(linker,"/FILEALIGN:0x200")
#endif
#endif

#ifdef PLUGINEX_EXPORTS
#define PLUGINEX_API __stdcall
#else
#define PLUGINEX_API __stdcall
#endif

#endif // __PluginEx_h__