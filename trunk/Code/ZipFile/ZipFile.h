// ZipFile.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// CZipFileApp:
// �йش����ʵ�֣������ ZipFile.cpp
//

class CZipFileApp : public CWinApp
{
public:
	CZipFileApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CZipFileApp theApp;
