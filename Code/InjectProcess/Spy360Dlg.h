#pragma once

class SysSweeper
{
public:
	int m0_pvt;
	int m4_hSysSweeper;
	void* m8_pIRubbishClean;
};

class DObj
{
public:
};

class KObj
{
public:
	void* m0_pvt;
	int field_4;
	CStringW m8_pathFile;
	int field_C;
	ULONGLONG m10_fileSize;
};

class VecKObj
{
public:
	KObj **m0_ppItem;
	ULONG m4_nItem;
	ULONG m8_size;
	ULONG mC;
};

class Category
{
public:
	VecKObj* m40_vecKObj(){return (VecKObj*)((ULONG)this + 0x40);}
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
	afx_msg void OnBnClickedGetRubbish();
	afx_msg void OnBnClickedTest();
	afx_msg void OnBnClickedHook();
};
