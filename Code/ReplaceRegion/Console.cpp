// Console.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <list>
#include <io.h>
#include "Console.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象

CWinApp theApp;

BOOL GetClipboardText(CString &sText)
{
	sText = "";

	LPSTR buffer = NULL; 
	if ( ::OpenClipboard(NULL) ) 
	{ 
		HANDLE hData = ::GetClipboardData(CF_TEXT);
		if(hData)
		{
			LPSTR buffer = (LPSTR)::GlobalLock(hData); 
			sText = buffer; 
			::GlobalUnlock(hData); 
			::CloseClipboard();

			return TRUE;
		}
	}
    return FALSE;
}
void ReadAllStdIn(CString &sIn)
{
	CHAR buf[MAX_PATH+1] = {0};
	while(fgets(buf,MAX_PATH,stdin))
		sIn += buf;
}
//////////////////////////////////////////////////////////////////////////
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// 初始化 MFC 并在失败时显示错误
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: 更改错误代码以符合您的需要
		_tprintf(_T("致命错误: MFC 初始化失败\n"));
		nRetCode = 1;
	}
	else
	{
		if(argc!=2)
			return 0;

		CString sIn;
		ReadAllStdIn(sIn);

	#if 0
		Ptr data;
		ULONG fSize = 0;
		LPCSTR lpData = (LPCSTR)File::LoadFile(argv[1],data,fSize,TRUE);
		if(!lpData)
			return 0;

		CString sData = lpData;
	#else
		CString sData = sIn;
	#endif

		static CString markS;// = "<!--marks-->";
		static CString markE;// = "<!--marke-->";

		markS.Format("<!--%ss-->",argv[1]);
		markE.Format("<!--%se-->",argv[1]);

		int sOfs = sData.Find(markS);
		if(sOfs==-1)
			return 0;

		int eOfs = sData.Find(markE,sOfs+markS.GetLength());
		if(eOfs==-1)
			return 0;

		CString sText;
		if(!GetClipboardText(sText))
			return 0;

		int start = sOfs+markS.GetLength();
		sData.Delete(start,eOfs-start);
		sData.Insert(start,sText);

		printf("%s",sData);
	}

	return nRetCode;
}
