#pragma once


// CSpy360Dlg �Ի���

class CSpy360Dlg : public CDialog
{
	DECLARE_DYNAMIC(CSpy360Dlg)

public:
	CSpy360Dlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSpy360Dlg();

// �Ի�������
	enum { IDD = IDD_SPY360DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
