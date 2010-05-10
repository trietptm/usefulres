#pragma once


// CSpy360Dlg 对话框

class CSpy360Dlg : public CDialog
{
	DECLARE_DYNAMIC(CSpy360Dlg)

public:
	CSpy360Dlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSpy360Dlg();

// 对话框数据
	enum { IDD = IDD_SPY360DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
