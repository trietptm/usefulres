// RecIntegDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RecInteg.h"
#include "RecIntegDlg.h"
#include ".\recintegdlg.h"

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


// CRecIntegDlg 对话框



CRecIntegDlg::CRecIntegDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecIntegDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRecIntegDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRecIntegDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_ADD_FILE, OnBnClickedAddFile)
END_MESSAGE_MAP()


// CRecIntegDlg 消息处理程序

BOOL CRecIntegDlg::OnInitDialog()
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
	
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CRecIntegDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CRecIntegDlg::OnPaint() 
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
HCURSOR CRecIntegDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
BOOL LoadData(LPCSTR lpFile,StrStrMap &map)
{
	Ptr data;
	ULONG fSize = 0;
	LPCSTR lpData = (LPCSTR)File::LoadFile(lpFile,data,fSize,TRUE);
	if(!lpData)
		return FALSE;

	CString sData = lpData;
	int iStart = 0;
	while(true)
	{
		CString sLine = sData.Tokenize("\r\n",iStart);
		if(iStart==-1)
			break;

		int iStart1 = 0;
		CString sKey = sLine.Tokenize("\t",iStart1);
		if(iStart1==-1)
			continue;

		CString sValue = sLine.Tokenize("\t",iStart1);
		if(iStart1==-1)
			continue;

		if(sValue.IsEmpty())
			continue;

		map[sKey] = sValue;
	}

	return TRUE;
}
BOOL SaveData(LPCSTR lpFile,/*const*/ StrStrMap &map)
{
	CString s;

	if(::PathFileExists(lpFile))
	{
		s.Format("%s.bak",lpFile);
		if(!::CopyFile(lpFile,s,FALSE))
		{
			AfxMessageBox("创建备份文件失败!");
			return FALSE;
		}
	}

	File f;
	if(!f.Open(lpFile,"wb"))
	{
		AfxMessageBox("创建数据库文件失败!");
		return FALSE;
	}

	StrStrMap::iterator iter;
	for(iter = map.begin();iter!=map.end();iter++)
	{
		s.Format("%s\t%s\r\n",iter->first,iter->second);
		::fwrite(s,s.GetLength(),1,f);
	}

	return TRUE;
}
void CRecIntegDlg::OnBnClickedAddFile()
{
	CString s;

	CHAR szFile[MAX_PATH] = "";
	if(!OpenFileDlg(szFile,NULL,"添加记录","文本文件(*.txt)\0*.txt\0"
		"所有文件(*.*)\0*.*\0\0"))
		return;

	CString sDBFile;
	sDBFile.Format("%s\\record.txt",gszWorkDir);

	StrStrMap db;
	LoadData(sDBFile,db);

	if(!LoadData(szFile,db))
	{
		AfxMessageBox("载入数据失败!");
		return;
	}

	if(SaveData(sDBFile,db))
	{
		s.Format("添加记录成功，目前共有%d条记录",db.size());
		AfxMessageBox(s);
	}
	else
	{
		AfxMessageBox("载入记录失败!");
	}
}