// MemPeek.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// CMemPeekApp:
// �йش����ʵ�֣������ MemPeek.cpp
//

class CMemPeekApp : public CWinApp
{
public:
	CMemPeekApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CMemPeekApp theApp;
