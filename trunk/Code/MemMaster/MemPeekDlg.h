// MemPeekDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CMemPeekDlg �Ի���
class CMemPeekDlg : public CDialog
{
// ����
public:
	CMemPeekDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MEMPEEK_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

	Process m_proc;
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
	afx_msg void OnBnClickedOpenProcess();
	CEdit m_edtProcess;
	CButton m_btOpenProc;
	afx_msg void OnBnClickedModify();
	CButton m_btModify;
	CEdit m_edtModifyAddr;
	CEdit m_edtModifyValue;
};
