#pragma once
#include "afxwin.h"


// CSpyOPRDlg 对话框

class CSpyOPRDlg : public CDialog
{
	DECLARE_DYNAMIC(CSpyOPRDlg)

public:
	CSpyOPRDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSpyOPRDlg();

// 对话框数据
	enum { IDD = IDD_SPYOPRDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_wLog;
	afx_msg void OnBnClickedHook();
	void AddLog(LPCSTR lpText);
};