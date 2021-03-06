// MemPeekDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MemPeek.h"
#include "MemPeekDlg.h"
#include ".\mempeekdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CMemPeekDlg 对话框



CMemPeekDlg::CMemPeekDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMemPeekDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMemPeekDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PROCESS, m_edtProcess);
	DDX_Control(pDX, IDC_OPEN_PROCESS, m_btOpenProc);
	DDX_Control(pDX, IDC_MODIFY, m_btModify);
	DDX_Control(pDX, IDC_EDIT_MODIFY_ADDR, m_edtModifyAddr);
	DDX_Control(pDX, IDC_EDIT_MODIFY_VALUE, m_edtModifyValue);
}

BEGIN_MESSAGE_MAP(CMemPeekDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_OPEN_PROCESS, OnBnClickedOpenProcess)
	ON_BN_CLICKED(IDC_MODIFY, OnBnClickedModify)
	ON_BN_CLICKED(IDC_EXPORT_DATA, OnBnClickedExportData)
END_MESSAGE_MAP()


// CMemPeekDlg 消息处理程序

BOOL CMemPeekDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将\“关于...\”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_btModify.EnableWindow(FALSE);

	m_edtProcess.SetWindowText("AWC.exe");
	m_edtModifyAddr.SetWindowText("03648000");
	m_edtModifyValue.SetWindowText("B801000000C3");

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CMemPeekDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMemPeekDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CMemPeekDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMemPeekDlg::OnBnClickedOpenProcess()
{
	CString sProcess;
	m_edtProcess.GetWindowText(sProcess);
	if(sProcess.IsEmpty())
		return;

	if(!m_proc.AttachProcess(sProcess))
	{
		AfxMessageBox("Attach process failed!");
		return;
	}

	m_edtProcess.SetReadOnly(TRUE);
	m_btOpenProc.EnableWindow(FALSE);
	m_btModify.EnableWindow(TRUE);
}
void CMemPeekDlg::OnBnClickedModify()
{
	CString s;
	m_edtModifyAddr.GetWindowText(s);
	if(s.IsEmpty())
		return;

	ULONG addr;
	int r = ::sscanf(s,"%X",&addr);
	if(r!=1)
		return;

	CString sValue;
	m_edtModifyValue.GetWindowText(sValue);
	if(sValue.IsEmpty() || (sValue.GetLength()%2)!=0)
		return;
	
	Bytes bytes;
	for(int ofs = 0;ofs < sValue.GetLength();ofs += 2)
	{
		s = sValue.Mid(ofs,2);

		ULONG value;
		r = ::sscanf(s,"%X",&value);
		if(r!=1)
			return;

		bytes.push_back(value);
	}

	if(m_proc.WriteMem(addr,&bytes[0],bytes.size()))
	{
		AfxMessageBox("Modify success!");
	}
}

class St
{
public:
	LPCSTR m0_lpStr;
	ULONG m4;
};
void CMemPeekDlg::OnBnClickedExportData()
{
	ULONG nItem = 0x8FA;
	
	ULONG srcAddr = 0x03648000;

	St* pData = (St*)::malloc(nItem*sizeof(St));
	Ptr data = pData;
	if(!m_proc.ReadMem(srcAddr,(UCHAR*)pData,nItem*sizeof(St)))
		return;

	{
		File f;
		if(!f.Open("_export_data.txt","wb"))
			return;

		for(ULONG i = 0;i<nItem;i++)
		{
			CString str;
			if(!m_proc.ReadStr((ULONG)pData[i].m0_lpStr,str))
				return;

			::fwrite(str,str.GetLength(),1,f);
			::fwrite("\r\n",2,1,f);
		}
	}

	AfxMessageBox("导出成功");
}