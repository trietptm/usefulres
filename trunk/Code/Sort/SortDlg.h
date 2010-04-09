// SortDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CSortDlg 对话框
class CSortDlg : public CDialog
{
// 构造
public:
	CSortDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SORT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


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
	afx_msg void OnBnClickedSelFile();
	CListCtrl m_lsData;
	CEdit m_edtFile;
	BOOL LoadFile(LPCSTR lpFile);
};
