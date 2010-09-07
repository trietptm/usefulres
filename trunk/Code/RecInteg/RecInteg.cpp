// RecInteg.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "RecInteg.h"
#include "RecIntegDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRecIntegApp

BEGIN_MESSAGE_MAP(CRecIntegApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CRecIntegApp ����

CRecIntegApp::CRecIntegApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CRecIntegApp ����

CRecIntegApp theApp;
CHAR gszWorkDir[MAX_PATH*2];

BOOL   OpenFileDlg(LPSTR pStrFile, HWND hParWnd, LPCSTR pTitle, LPCSTR pFilter)
{
	CHAR          szFileName[1024] = "";
	OPENFILENAMEA ofn ;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hParWnd;
	ofn.lpstrFilter = pFilter;
	ofn.nFilterIndex = 0;
	ofn.nMaxFile = MAX_PATH ;
	ofn.lpstrTitle = pTitle;
	ofn.lpstrFile = szFileName;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;

	if(!GetOpenFileNameA( &ofn))//ѡ����ȡ����ť
		return FALSE;

	strcpy(pStrFile, szFileName);
	return TRUE;
}
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
// CRecIntegApp ��ʼ��

BOOL CRecIntegApp::InitInstance()
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

	CRecIntegDlg dlg;
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
