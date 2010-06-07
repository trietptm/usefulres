// SpyOPRDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "SpyOPRDlg.h"


// CSpyOPRDlg 对话框

IMPLEMENT_DYNAMIC(CSpyOPRDlg, CDialog)
CSpyOPRDlg::CSpyOPRDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpyOPRDlg::IDD, pParent)
{
}

CSpyOPRDlg::~CSpyOPRDlg()
{
}

void CSpyOPRDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSpyOPRDlg, CDialog)
END_MESSAGE_MAP()


// CSpyOPRDlg 消息处理程序
