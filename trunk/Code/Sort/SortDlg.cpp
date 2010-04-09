// SortDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "Sort.h"
#include "SortDlg.h"
#include ".\sortdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CSortDlg �Ի���



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


// CSortDlg ��Ϣ�������

BOOL CSortDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��\������...\���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	CRect rcLs;
	m_lsData.GetClientRect(&rcLs);

	m_lsData.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_lsData.InsertColumn(0,"",LVCFMT_LEFT,rcLs.Width());
	
	return TRUE;  // ���������˿ؼ��Ľ��㣬���򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CSortDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
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
	if(!GetOpenFileNameA( &ofn))//ѡ����ȡ����ť
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
	if(!OpenFileDlg(szFile,NULL,"���˺�����","�ı��ļ�(*.txt)\0*.txt\0"
		"�����ļ�(*.*)\0*.*\0\0"))
		return;
	m_edtFile.SetWindowText(szFile);

	LoadFile(szFile);	
}