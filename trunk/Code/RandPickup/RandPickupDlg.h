// RandPickupDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"


// CRandPickupDlg �Ի���
class CRandPickupDlg : public CDialog
{
// ����
public:
	CRandPickupDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_RANDPICKUP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

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
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
