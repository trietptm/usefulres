// RandPickup.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "RandPickup.h"
#include "RandPickupDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRandPickupApp

BEGIN_MESSAGE_MAP(CRandPickupApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CRandPickupApp ����

CRandPickupApp::CRandPickupApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CRandPickupApp ����

CRandPickupApp theApp;
CHAR gszWorkDir[MAX_PATH*2];

// CRandPickupApp ��ʼ��
BOOL GetProcWorkDir(LPSTR pWorkDir,HMODULE hModule = NULL)
{
	CHAR   szModFileName[MAX_PATH*2];
	DWORD   dwRetVal;
	CHAR   *pFind;
	dwRetVal = GetModuleFileNameA(hModule, szModFileName, sizeof(szModFileName));
	if(dwRetVal == 0 || dwRetVal >= sizeof(szModFileName))
		return FALSE;

	pFind = strrchr(szModFileName, '\\');
	if(pFind == NULL)
		return FALSE;

	pFind[0] = 0;
	if(pWorkDir)
		strcpy(pWorkDir, szModFileName);
	return TRUE;
}

BOOL CRandPickupApp::InitInstance()
{
	GetProcWorkDir(gszWorkDir);

	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControls()�����򣬽��޷��������ڡ�
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	CRandPickupDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˷��ô����ʱ�á�ȷ�������ر�
		//�Ի���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ�á�ȡ�������ر�
		//�Ի���Ĵ���
	}

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	// ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}
