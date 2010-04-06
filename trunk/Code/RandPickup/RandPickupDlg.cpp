// RandPickupDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "RandPickup.h"
#include "RandPickupDlg.h"
#include ".\randpickupdlg.h"

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


// CRandPickupDlg 对话框



CRandPickupDlg::CRandPickupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRandPickupDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRandPickupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FILE, m_lsFile);
}

BEGIN_MESSAGE_MAP(CRandPickupDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_ADD_DIR, OnBnClickedAddDir)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST_FILE, OnLvnDeleteitemListFile)
	ON_BN_CLICKED(IDC_RAND_PICKUP, OnBnClickedRandPickup)
	ON_BN_CLICKED(IDC_GET_CODE_LINE, OnBnClickedGetCodeLine)
END_MESSAGE_MAP()


// CRandPickupDlg 消息处理程序

BOOL CRandPickupDlg::OnInitDialog()
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
	m_lsFile.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	m_lsFile.InsertColumn(0,"文件",LVCFMT_LEFT,532);

	//LPCSTR lpDir = "D:\\SVN\\RegistryMechanic";
	//AddDir(lpDir,lpDir);
	
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CRandPickupDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CRandPickupDlg::OnPaint() 
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
HCURSOR CRandPickupDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void CRandPickupDlg::AddDir(LPCSTR lpOwner,LPCSTR lpDir)
{
	CString s;

	CFileFind ff;
	s.Format("%s\\*",lpDir);
	BOOL bContinue = ff.FindFile(s);

	while(bContinue)
	{
		bContinue = ff.FindNextFile();
		//---
		if(ff.IsDots())
			continue;

		CString pathFile = ff.GetFilePath();
		if(ff.IsDirectory())
		{
			AddDir(lpOwner,pathFile);
			continue;
		}

		LPCSTR lpExt = strrchr(pathFile,'.');
		if(!lpExt)
			continue;
		if(::stricmp(lpExt,".h")!=0 && ::stricmp(lpExt,".cpp")!=0)
			continue;

		int iItem = m_lsFile.InsertItem(m_lsFile.GetItemCount(),pathFile);
		
		Item* p = new Item();
		p->m_owner = lpDir;
		m_lsFile.SetItemData(iItem,(DWORD)p);
	}
}
void CRandPickupDlg::OnBnClickedAddDir()
{
	CString strPath;   

	BROWSEINFO   bi;
	char   name[MAX_PATH];
	ZeroMemory(&bi,sizeof(BROWSEINFO));
	bi.hwndOwner=GetSafeHwnd();
	bi.pszDisplayName=name;
	bi.lpszTitle="Select folder";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if(idl==NULL)
		return;   

	SHGetPathFromIDList(idl,strPath.GetBuffer(MAX_PATH));
	strPath.ReleaseBuffer();

	AddDir(strPath,strPath);
}
void CRandPickupDlg::OnLvnDeleteitemListFile(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	Item* p = (Item*)m_lsFile.GetItemData(pNMLV->iItem);
	if(p)
	{
		delete p;
		m_lsFile.SetItemData(pNMLV->iItem,NULL);
	}

	*pResult = 0;
}
void CRandPickupDlg::ResetUsed()
{
	ULONG nCount = m_lsFile.GetItemCount();
	for(ULONG i = 0;i<nCount;i++)
	{
		Item* p = (Item*)m_lsFile.GetItemData(i);
		if(!p)
			continue;

		p->m_bUsed = FALSE;
	}
}
BOOL LoadStrData(LPCSTR lpFile,CString &sData)
{
	sData.Empty();

	FILE *fp = ::fopen(lpFile,"rb");
	if(!fp)
		return FALSE;

	ULONG nSize = ::filelength(::fileno(fp));
	::fread(sData.GetBuffer(nSize+1),nSize,1,fp);
	sData.GetBuffer()[nSize] = '\0';
	sData.ReleaseBuffer();

	::fclose(fp);
	return TRUE;
}
void CRandPickupDlg::RandPickup(LPCSTR lpOutFile)
{
	File f;
	if(!f.Open(lpOutFile,"wb"))
		return;

	ULONG nCount = m_lsFile.GetItemCount();
	if(nCount <= 0)
		return;
	
	CString s;

	enum{NFile = 30,NLinePerFile = 50};
	ULONG iFile = 0;
	while(iFile < NFile)
	{
		ULONG iRand = rand()%nCount;
		Item* p = (Item*)m_lsFile.GetItemData(iRand);
		if(p->m_bUsed)
			continue;

		CString sFile = m_lsFile.GetItemText(iRand,0);

		if(sFile.GetLength() < p->m_owner.GetLength()+1)
		{
			p->m_bUsed = TRUE;
			continue;
		}

		CString sData;
		if(!LoadStrData(sFile,sData))
		{
			p->m_bUsed = TRUE;
			continue;
		}

		int iLine = 0;
		int iStart = 0;
		
		LPCSTR lpExt = strrchr(sFile,'.');

		CString sFData;
		while(true)
		{			
			CString sLine = sData.Tokenize("\r\n",iStart);
			if(iStart==-1)
				break;

			if(::stricmp(lpExt,".cpp")==0)
			{
				if(sLine.Find('(')!=-1)
				{
					sFData += sLine;
					break;
				}
			}
			else if(::stricmp(lpExt,".h")==0)
			{
				if(sLine.Find("class")!=-1)
				{
					sFData += sLine;
					break;
				}
			}
		}
		if(sFData.IsEmpty())
		{
			p->m_bUsed = TRUE;
			continue;
		}
		
		INT braceMatch = 0;
		while(true)
		{			
			CString sLine = sData.Tokenize("\r\n",iStart);
			if(iStart==-1)
				break;
			iLine++;

            if(iLine>=NLinePerFile)
			{
				if(braceMatch<=0)
					break;
			}

			LPCTSTR p1 = sLine;
			while(*p1)
			{
				if(*p1=='{')
				{
					braceMatch++;
				}
				else if(*p1=='}')
				{
					braceMatch--;
				}
				p1++;
			}

			sFData += sLine;
			sFData += "\r\n";
		}

		if(iLine < NLinePerFile)
		{
			p->m_bUsed = TRUE;
			continue;
		}

		s.Format("%s\r\n",sFile.Mid(p->m_owner.GetLength()+1));
		::fwrite(s,s.GetLength(),1,f);

		::fwrite(sFData,sFData.GetLength(),1,f);
		p->m_bUsed = TRUE;
		iFile++;

		s.Format("\r\n");
		::fwrite(s,s.GetLength(),1,f);
	}
}
void CRandPickupDlg::OnBnClickedRandPickup()
{
	ResetUsed();

	srand(time(0));

	CString s;
	
	s.Format("%s\\前面30页.h",gszWorkDir);
	RandPickup(s);

	s.Format("%s\\后面30页.h",gszWorkDir);
	RandPickup(s);
}
ULONG CRandPickupDlg::GetLineCount(LPCSTR lpFile)
{
	ULONG nLine = 0;

	CString sData;
	if(!LoadStrData(lpFile,sData))
		return 0;

	int iStart = 0;
	while(true)
	{			
		CString sLine = sData.Tokenize("\r\n",iStart);
		if(iStart==-1)
			break;
		nLine++;
	}
	return nLine;
}
void CRandPickupDlg::OnBnClickedGetCodeLine()
{
	CString s;

	ULONG nLine = 0;

	ULONG nCount = m_lsFile.GetItemCount();
	for(ULONG i = 0;i<nCount;i++)
	{
		s = m_lsFile.GetItemText(i,0);	
		nLine += GetLineCount(s);
	}

	s.Format("代码一共%d行",nLine);
	AfxMessageBox(s);
}
