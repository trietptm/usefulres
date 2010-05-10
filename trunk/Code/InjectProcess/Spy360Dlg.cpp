// Spy360Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "Spy360Dlg.h"


// CSpy360Dlg 对话框

IMPLEMENT_DYNAMIC(CSpy360Dlg, CDialog)
CSpy360Dlg::CSpy360Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpy360Dlg::IDD, pParent)
{
}

CSpy360Dlg::~CSpy360Dlg()
{
}

void CSpy360Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSpy360Dlg, CDialog)
END_MESSAGE_MAP()


// CSpy360Dlg 消息处理程序
