// MemPeekDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CMemPeekDlg 对话框
class CMemPeekDlg : public CDialog
{
// 构造
public:
	CMemPeekDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MEMPEEK_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	Process m_proc;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOpenProcess();
	CEdit m_edtProcess;
	CButton m_btOpenProc;
	afx_msg void OnBnClickedModify();
	CButton m_btModify;
	CEdit m_edtModifyAddr;
	CEdit m_edtModifyValue;
};
