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


// CSpy360Dlg ��Ϣ�������

void CSpy360Dlg::OnBnClickedCrack()
{
	if(!m_h360clean)
		m_h360clean = GetModuleHandle("360clean.dll");

	if(!m_h360clean)
	{
		AfxMessageBox("ģ��360clean.dllδ����");
		return;
	}

	if(!m_hSysSweeper)
		m_hSysSweeper = GetModuleHandle("SysSweeper.dll");

	if(!m_hSysSweeper)
	{
		AfxMessageBox("ģ��SysSweeper.dllδ����");
		return;
	}

	AfxMessageBox("��ȡ�ɹ�");
}
void CSpy360Dlg::OnBnClickedGetRubbishCount()
{

}