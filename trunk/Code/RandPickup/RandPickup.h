// RandPickup.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// CRandPickupApp:
// �йش����ʵ�֣������ RandPickup.cpp
//

class CRandPickupApp : public CWinApp
{
public:
	CRandPickupApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CRandPickupApp theApp;
