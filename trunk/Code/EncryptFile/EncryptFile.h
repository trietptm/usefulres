// EncryptFile.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// CEncryptFileApp:
// �йش����ʵ�֣������ EncryptFile.cpp
//

class CEncryptFileApp : public CWinApp
{
public:
	CEncryptFileApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CEncryptFileApp theApp;
