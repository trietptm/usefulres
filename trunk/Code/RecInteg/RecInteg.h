// RecInteg.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// CRecIntegApp:
// �йش����ʵ�֣������ RecInteg.cpp
//

class CRecIntegApp : public CWinApp
{
public:
	CRecIntegApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CRecIntegApp theApp;
