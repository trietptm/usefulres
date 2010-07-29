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
		if(argc!=2)
			return 0;

		Ptr data;
		ULONG fSize = 0;
		LPCSTR lpData = (LPCSTR)File::LoadFile(argv[1],data,fSize,TRUE);
		if(!lpData)
			return 0;

		CString sData = lpData;

		static CString markS = "<!--marks-->";
		static CString markE = "<!--marke-->";

		int sOfs = sData.Find(markS);
		if(sOfs==-1)
			return 0;

		int eOfs = sData.Find(markE,sOfs+markS.GetLength());
		if(eOfs==-1)
			return 0;

		int start = sOfs+markS.GetLength();
		sData.Delete(start,eOfs-start);
		sData.Insert(start,"\r\nDone!\r\n");

		printf("%s",sData);
	}

	return nRetCode;
}
