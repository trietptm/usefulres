// SpyOPRDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "SpyOPRDlg.h"
#include ".\spyoprdlg.h"


CSpyOPRDlg *gpSpyOPRDlg;

ULONG (*COleStreamFile__GetPosition)();

IMPLEMENT_DYNAMIC(CSpyOPRDlg, CDialog)
CSpyOPRDlg::CSpyOPRDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpyOPRDlg::IDD, pParent)
{
	gpSpyOPRDlg = this;
}

CSpyOPRDlg::~CSpyOPRDlg()
{
}

void CSpyOPRDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_LOG, m_wLog);
}


BEGIN_MESSAGE_MAP(CSpyOPRDlg, CDialog)
	ON_BN_CLICKED(IDC_HOOK, OnBnClickedHook)
END_MESSAGE_MAP()

void CSpyOPRDlg::AddLog(LPCSTR lpText)
{
	int len = gpSpyOPRDlg->m_wLog.GetWindowTextLength();
	gpSpyOPRDlg->m_wLog.SetSel(len,len,TRUE);
	gpSpyOPRDlg->m_wLog.ReplaceSel(lpText);
}

void __stdcall On_45AE91(LPVOID lpOleFile)
{
	CString s;
	s.Format("new COleStreamFile:0x%X\r\n",lpOleFile);
	gpSpyOPRDlg->AddLog(s);
}
__declspec(naked) void Hook_45AE91()
{
	static ULONG oldEsp;
	__asm{
		mov oldEsp,esp
		
		push ecx
		push edx
		push ebx

		push ecx
		call On_45AE91

		pop ebx
		pop edx
		pop ecx

		mov eax,0x46098F

		mov esp,oldEsp
		push 0x0045AE96
		ret
	}
}

void __stdcall On_45AF5F(LPVOID lpOleFile)
{
	CString s;
	s.Format("delete COleStreamFile:0x%X\r\n",lpOleFile);
	gpSpyOPRDlg->AddLog(s);
}
__declspec(naked) void Hook_45AF5F()
{
	static ULONG oldEsp;
	__asm{
		mov oldEsp,esp

		push ecx
		push edx
		push ebx

		push ecx
		call On_45AF5F

		pop ebx
		pop edx
		pop ecx
		
		mov esp,oldEsp

		push esi
		mov esi,ecx
		mov eax,0x0045AF7B
		call eax

		push 0x0045AF67
		ret
	}
}

void __stdcall On_45B29C(LPVOID lpOleFile)
{
	CString s;

	ULONG pos = ThisCall(lpOleFile,COleStreamFile__GetPosition);
	s.Format("COleStreamFile::Read 0x%X,0x%X\r\n",lpOleFile,pos);
	gpSpyOPRDlg->AddLog(s);
}
__declspec(naked) void Hook_45B29C()
{
	static ULONG oldEsp;
	__asm{
		mov oldEsp,esp

		push ecx
		push edx
		push ebx

		push ecx
		call On_45B29C

		pop ebx
		pop edx
		pop ecx

		mov esp,oldEsp

		push ebp
		mov ebp,esp
		lea eax,[ebp+0xC]

		push 0x0045B2A2
		ret
	}
}
void CSpyOPRDlg::OnBnClickedHook()
{
	*(ULONG*)&COleStreamFile__GetPosition = 0x0045B101;

	HookAddr(0x0045AE91,Hook_45AE91);
	HookAddr(0x0045AF5F,Hook_45AF5F);
	HookAddr(0x0045B29C,Hook_45B29C);
}