#pragma once
#include "afxwin.h"


// CSpyOPRDlg �Ի���

class CSpyOPRDlg : public CDialog
{
	DECLARE_DYNAMIC(CSpyOPRDlg)

public:
	CSpyOPRDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSpyOPRDlg();

// �Ի�������
	enum { IDD = IDD_SPYOPRDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_wLog;
	afx_msg void OnBnClickedHook();
	void AddLog(LPCSTR lpText);
};