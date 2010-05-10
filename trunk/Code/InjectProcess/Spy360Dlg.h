#pragma once

class SysSweeper
{
public:
	int m0_pvt;
	int m4_hSysSweeper;
	void* m8_pIRubbishClean;
};
// CSpy360Dlg 对话框

class CSpy360Dlg : public CDialog
{
	DECLARE_DYNAMIC(CSpy360Dlg)

public:
	CSpy360Dlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSpy360Dlg();

// 对话框数据
	enum { IDD = IDD_SPY360DLG };

	HMODULE m_h360clean;
	HMODULE m_hSysSweeper;	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCrack();
	afx_msg void OnBnClickedGetRubbishCount();
};
