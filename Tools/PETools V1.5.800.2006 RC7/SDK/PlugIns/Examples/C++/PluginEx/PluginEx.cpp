// PluginEx.cpp : Defines the entry point for the DLL application.
// Coded by NEOx <NEOx@Pisem.net>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include "PluginEx.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
    {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HINSTANCE)hModule);
            break;
        case DLL_THREAD_ATTACH:  break;
        case DLL_THREAD_DETACH:  break;
        case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}

VOID PLUGINEX_API StartPTPlugin(HWND hDlg)
{
	MessageBox(hDlg, "Plugin", "Hello, I'm plugin !", MB_OK | MB_ICONINFORMATION);
}

CHAR *PLUGINEX_API GetPTPluginName()
{
	return "Plugin Example";
}