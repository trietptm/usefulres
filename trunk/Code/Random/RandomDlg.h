// RandomDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CRandomDlg �Ի���
class CRandomDlg : public CDialog
{
// ����
public:
	CRandomDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_RANDOM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


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
	afx_msg void OnBnClickedGenHex();
	CEdit m_edtResult;
	afx_msg void OnBnClickedGenRegCode();
	afx_msg void OnBnClickedGenStr();
};
