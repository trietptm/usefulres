// Procs_Ex3.cpp : Defines the entry point for the application.
// Coded by NEOx <NEOx@Pisem.net>

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "resource.h"
#include "..\..\Include\Procs32.h"

#pragma comment(lib,"..\\..\\Procs32.lib")


#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/IGNORE:4078 /IGNORE:4089")
#pragma comment(linker,"/RELEASE")
#pragma comment(linker,"/merge:.rdata=.data")
#pragma comment(linker,"/merge:.text=.data")
#if _MSC_VER >= 1000
#pragma comment(linker,"/FILEALIGN:0x200")
#endif
#endif

BOOL EnableDebugPrivilege(BOOL fEnable);
void TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
BOOL CALLBACK DialogProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);

#define IsNT (BOOL)(GetVersion() < 0x80000000 ? TRUE : FALSE)

CHAR cBuffer[MAX_PATH];
CHAR lpFileName[MAX_PATH];

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	if(IsNT)
		EnableDebugPrivilege(TRUE);

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc   = DefDlgProc;
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.hInstance     = hInstance;
	wc.hCursor       = LoadCursor(hInstance, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "Procs_Ex3";
	RegisterClass(&wc);
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, (DLGPROC)DialogProc);

	return 0;
}

BOOL CALLBACK DialogProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	switch(uiMessage)
	{
		case WM_INITDIALOG:

			GetModuleFileName(NULL, lpFileName, MAX_PATH);

			wsprintf(cBuffer, "0x%0.8X", GetProcessPathID(lpFileName));
			SetDlgItemText(hWnd, IDC_PATH_ID, cBuffer);

			SetTimer(hWnd, 111, 50, (TIMERPROC)TimerProc);
			return TRUE;

		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_CLOSE:
					KillTimer(hWnd, 111);
					SendMessage(hWnd, WM_CLOSE, NULL, NULL);
					ExitProcess(0);
					break;
			}
	}
	return FALSE;
}

void TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	wsprintf(cBuffer, "%d", GetNumberOfProcesses());
	SetDlgItemText(hWnd, IDC_PROC_NUM, cBuffer);

	wsprintf(cBuffer, "%d", GetNumberOfModules(GetCurrentProcessId()));
	SetDlgItemText(hWnd, IDC_MODUL_NUM, cBuffer);
}

BOOL EnableDebugPrivilege(BOOL fEnable)
{
	HANDLE hToken;
	BOOL   fOk = FALSE;
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return fOk;
}