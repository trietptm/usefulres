// InjectProcess.cpp : ���� DLL �ĳ�ʼ�����̡�
//

#include "stdafx.h"
#include "InjectProcess.h"
#include "Spy360Dlg.h"
#include "SpyOPRDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//	ע�⣡
//
//		����� DLL ��̬���ӵ� MFC
//		DLL���Ӵ� DLL ������
//		���� MFC ���κκ����ں�������ǰ��
//		��������� AFX_MANAGE_STATE �ꡣ
//
//		����:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// �˴�Ϊ��ͨ������
//		}
//
//		�˺������κ� MFC ����
//		������ÿ��������ʮ����Ҫ������ζ��
//		��������Ϊ�����еĵ�һ�����
//		���֣������������ж������������
//		������Ϊ���ǵĹ��캯���������� MFC
//		DLL ���á�
//
//		�й�������ϸ��Ϣ��
//		����� MFC ����˵�� 33 �� 58��
//

// CInjectProcessApp

BEGIN_MESSAGE_MAP(CInjectProcessApp, CWinApp)
END_MESSAGE_MAP()


// CInjectProcessApp ����

CInjectProcessApp::CInjectProcessApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CInjectProcessApp ����

CInjectProcessApp theApp;


// CInjectProcessApp ��ʼ��

BOOL CInjectProcessApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

void InjectInit()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
#if 0
	CSpy360Dlg dlg;
#else
	CSpyOPRDlg dlg;
#endif
	dlg.DoModal();
}