// RandPickupDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"


// CRandPickupDlg 对话框
class CRandPickupDlg : public CDialog
{
// 构造
public:
	CRandPickupDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_RANDPICKUP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	class Item
	{
	public:
		CString m_owner;
		BOOL m_bUsed;
	public:
		Item()
		{
			m_bUsed = FALSE;
		}
	};
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
	CListCtrl m_lsFile;
	afx_msg void OnBnClickedAddDir();
	void AddDir(LPCSTR lpOwner,LPCSTR lpDir);
	afx_msg void OnLvnDeleteitemListFile(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedRandPickup();
	void ResetUsed();
	void RandPickup(LPCSTR lpOutFile);
	afx_msg void OnBnClickedGetCodeLine();
	ULONG GetLineCount(LPCSTR lpFile);
};
