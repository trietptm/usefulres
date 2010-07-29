// Console.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <list>
#include <io.h>
#include "Console.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Ψһ��Ӧ�ó������

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

//////////////////////////////////////////////////////////////////////////
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// ��ʼ�� MFC ����ʧ��ʱ��ʾ����
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: ���Ĵ�������Է���������Ҫ
		_tprintf(_T("��������: MFC ��ʼ��ʧ��\n"));
		nRetCode = 1;
	}
	else
	{
		if(argc!=3)
			return 0;

		Ptr data;
		ULONG fSize = 0;
		LPCSTR lpData = (LPCSTR)File::LoadFile(argv[1],data,fSize,TRUE);
		if(!lpData)
			return 0;

		CString sData = lpData;

		static CString markS;// = "<!--marks-->";
		static CString markE;// = "<!--marke-->";

		markS.Format("<!--%ss-->",argv[2]);
		markE.Format("<!--%se-->",argv[2]);

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
