// Spy360Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "Spy360Dlg.h"
#include ".\spy360dlg.h"

SysSweeper* gpSysSweeper;
HRESULT (__stdcall *IRubbishClean__GetItemScanResult)(void *pThis, int nID, LONG *pfileNum, ULONGLONG *fizeSizeTotal);
BOOL (__stdcall *IRubbishClean__GetObj)(INT nID, Category **ppCategory, DObj **ppDObj);

// CSpy360Dlg 对话框

IMPLEMENT_DYNAMIC(CSpy360Dlg, CDialog)
CSpy360Dlg::CSpy360Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpy360Dlg::IDD, pParent)
{
	m_h360clean = NULL;
	m_hSysSweeper = NULL;
}

CSpy360Dlg::~CSpy360Dlg()
{
}

void CSpy360Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSpy360Dlg, CDialog)
	ON_BN_CLICKED(IDC_CRACK, OnBnClickedCrack)
	ON_BN_CLICKED(IDC_GET_RUBBISH_COUNT, OnBnClickedGetRubbishCount)
	ON_BN_CLICKED(IDC_GET_RUBBISH, OnBnClickedGetRubbish)
END_MESSAGE_MAP()


// CSpy360Dlg 消息处理程序

void CSpy360Dlg::OnBnClickedCrack()
{
	if(!m_h360clean)
		m_h360clean = GetModuleHandle("360clean.dll");

	if(!m_h360clean)
	{
		AfxMessageBox("模块360clean.dll未加载");
		return;
	}

	if(!m_hSysSweeper)
		m_hSysSweeper = GetModuleHandle("SysSweeper.dll");

	if(!m_hSysSweeper)
	{
		AfxMessageBox("模块SysSweeper.dll未加载");
		return;
	}

	gpSysSweeper = (SysSweeper*)((ULONG)m_h360clean + 0x26828);
	*(ULONG*)&IRubbishClean__GetItemScanResult = (ULONG)m_hSysSweeper + 0x61B2;
	*(ULONG*)&IRubbishClean__GetObj = (ULONG)m_hSysSweeper + 0x5EEC;

	AfxMessageBox("获取成功");
}
void CSpy360Dlg::OnBnClickedGetRubbishCount()
{
	CString s;

	LONG nFile = 0;
	ULONGLONG fSizeTotal = 0;
	if(IRubbishClean__GetItemScanResult(gpSysSweeper->m8_pIRubbishClean,1,&nFile,&fSizeTotal)==S_OK)
	{
		s.Format("%d,%I64d",nFile,fSizeTotal);
		AfxMessageBox(s);
	}
}
void CSpy360Dlg::OnBnClickedGetRubbish()
{
	Category* pCategory = NULL;
	DObj* pDObj = NULL;

	if(!ThisCall(gpSysSweeper->m8_pIRubbishClean,IRubbishClean__GetObj,1,&pCategory, &pDObj))
		return;

	CString s,s1;
	if(pCategory)
	{
		File f;
		if(!f.Open("C:\\Rubbish1.txt","wb"))
			return;

		VecKObj* pVecKObj = pCategory->m40_vecKObj();
		for(ULONG i = 0;i<pVecKObj->m4_nItem;i++)
		{
			KObj* pKObj = pVecKObj->m0_ppItem[i];

			s1 = pKObj->m8_pathFile;
			s.Format("%s\r\n",s1);
			::fwrite(s,s.GetLength(),1,f);
		}
	}
}
