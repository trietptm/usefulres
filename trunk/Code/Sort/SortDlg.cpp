// SortDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Sort.h"
#include "SortDlg.h"
#include ".\sortdlg.h"

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


// CSortDlg 对话框



CSortDlg::CSortDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSortDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSortDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DATA, m_lsData);
	DDX_Control(pDX, IDC_EDIT_FILE, m_edtFile);
}

BEGIN_MESSAGE_MAP(CSortDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_SEL_FILE, OnBnClickedSelFile)
END_MESSAGE_MAP()


// CSortDlg 消息处理程序

BOOL CSortDlg::OnInitDialog()
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
	CRect rcLs;
	m_lsData.GetClientRect(&rcLs);

	m_lsData.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_lsData.InsertColumn(0,"",LVCFMT_LEFT,rcLs.Width());
	
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CSortDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSortDlg::OnPaint() 
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
HCURSOR CSortDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL   OpenFileDlg(LPSTR pStrFile, HWND hParWnd, LPCSTR pTitle, LPCSTR pFilter)
{
	CHAR          szFileName[1024] = "";
	OPENFILENAMEA ofn ;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hParWnd;
	ofn.lpstrFilter = pFilter;
	ofn.nFilterIndex = 0;
	ofn.nMaxFile = MAX_PATH ;
	ofn.lpstrTitle = pTitle;
	ofn.lpstrFile = szFileName;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if(!GetOpenFileNameA( &ofn))//选择了取消按钮
		return FALSE;

	strcpy(pStrFile, szFileName);
	return TRUE;
}
BOOL CSortDlg::LoadFile(LPCSTR lpFile)
{
	m_lsData.SetRedraw(FALSE);
	m_lsData.DeleteAllItems();

	CString s;

	BOOL bRet = FALSE;
	do{
		File f;
		if(!f.Open(lpFile,"rb"))
			break;

		ULONG fSize = ::filelength(::fileno(f));
		if(fSize==0)
		{
			bRet = TRUE;
			break;
		}
		UCHAR* pBuf = (UCHAR*)::malloc(fSize+sizeof(WCHAR));
		if(!pBuf)
			break;
		Ptr data = pBuf;
		::fread(pBuf,fSize,1,f);
		pBuf[fSize] = '\0';
		pBuf[fSize+1] = '\0';

		UCHAR* p = pBuf;
		//unicode
		if(*(USHORT*)p==0xFEFF)
		{
			p += sizeof(USHORT);

			CStringW sData = (LPCWSTR)p;
			int iStart = 0;
			while(true)
			{
				CStringW sLine = sData.Tokenize(L"\r\n",iStart);
				if(iStart==-1)
					break;

				s = sLine;
				m_lsData.InsertItem(m_lsData.GetItemCount(),s);
			}
		}
		else
		{

		}
	}while(0);
	m_lsData.SetRedraw(TRUE);
	return bRet;
}
void CSortDlg::OnBnClickedSelFile()
{
	CHAR szFile[MAX_PATH] = "";
	if(!OpenFileDlg(szFile,NULL,"导人黑名单","文本文件(*.txt)\0*.txt\0"
		"所有文件(*.*)\0*.*\0\0"))
		return;
	m_edtFile.SetWindowText(szFile);

	LoadFile(szFile);	
}