// InjectProcess.h : InjectProcess DLL ����ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// ������


// CInjectProcessApp
// �йش���ʵ�ֵ���Ϣ������� InjectProcess.cpp
//

class CInjectProcessApp : public CWinApp
{
public:
	CInjectProcessApp();

// ��д
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
