// EncryptFileDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"


// CEncryptFileDlg �Ի���
class CEncryptFileDlg : public CDialog
{
// ����
public:
	CEncryptFileDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_ENCRYPTFILE_DIALOG };

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
	afx_msg void OnBnClickedSelFile();
	CEdit m_edtFile;
	afx_msg void OnBnClickedEncryptXor();
	CEdit m_edtXorKey;
};
