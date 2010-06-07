// SpyOPRDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "SpyOPRDlg.h"
#include ".\spyoprdlg.h"


CSpyOPRDlg *gpSpyOPRDlg;

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
		call On_45AE91

		mov eax,0x46098F

		mov esp,oldEsp
		push 0x0045AE96
		ret
	}
}
void CSpyOPRDlg::OnBnClickedHook()
{
	HookAddr(0x0045AE91,Hook_45AE91);
}