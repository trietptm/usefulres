// Spy360Dlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "Spy360Dlg.h"
#include ".\spy360dlg.h"


// CSpy360Dlg �Ի���

IMPLEMENT_DYNAMIC(CSpy360Dlg, CDialog)
CSpy360Dlg::CSpy360Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpy360Dlg::IDD, pParent)
{
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
END_MESSAGE_MAP()


// CSpy360Dlg ��Ϣ�������

void CSpy360Dlg::OnBnClickedCrack()
{
	if(!m_hSysSweeper)
		m_hSysSweeper = GetModuleHandle("SysSweeper.dll");

	if(!m_hSysSweeper)
	{
		AfxMessageBox("ģ��SysSweeper.dllδ����");
		return;
	}
}