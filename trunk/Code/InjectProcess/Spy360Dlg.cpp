// Spy360Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "Spy360Dlg.h"
#include ".\spy360dlg.h"


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

	AfxMessageBox("获取成功");
}
void CSpy360Dlg::OnBnClickedGetRubbishCount()
{

}