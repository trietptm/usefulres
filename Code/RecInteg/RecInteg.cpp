// RecInteg.cpp : 定义应用程序的类行为。
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


// CRecIntegApp 构造

CRecIntegApp::CRecIntegApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CRecIntegApp 对象

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

	if(!GetOpenFileNameA( &ofn))//选择了取消按钮
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
// CRecIntegApp 初始化

BOOL CRecIntegApp::InitInstance()
{
	GetProcWorkDir(gszWorkDir);

	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControls()。否则，将无法创建窗口。
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	CRecIntegDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 在此放置处理何时用“确定”来关闭
		//对话框的代码
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用“取消”来关闭
		//对话框的代码
	}

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	// 而不是启动应用程序的消息泵。
	return FALSE;
}
