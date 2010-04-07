// ZipFileDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ZipFile.h"
#include "ZipFileDlg.h"
#include ".\zipfiledlg.h"

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


// CZipFileDlg �Ի���



CZipFileDlg::CZipFileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CZipFileDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CZipFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_FILE, m_edtFile);
}

BEGIN_MESSAGE_MAP(CZipFileDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SEL_FILE, OnBnClickedButtonSelFile)
	ON_BN_CLICKED(IDC_BUTTON_ZIP, OnBnClickedButtonZip)
	ON_BN_CLICKED(IDC_UN_ZIP, OnBnClickedUnZip)
END_MESSAGE_MAP()


// CZipFileDlg ��Ϣ�������

BOOL CZipFileDlg::OnInitDialog()
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
	
	return TRUE;  // ���������˿ؼ��Ľ��㣬���򷵻� TRUE
}

void CZipFileDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CZipFileDlg::OnPaint() 
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
HCURSOR CZipFileDlg::OnQueryDragIcon()
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
void CZipFileDlg::OnBnClickedButtonSelFile()
{
	CHAR szFile[MAX_PATH] = "";
	if(!OpenFileDlg(szFile,NULL,"Open file","All files(*.*)\0*.*\0\0"))
		return;

	m_edtFile.SetWindowText(szFile);
}
void CZipFileDlg::OnBnClickedButtonZip()
{
	CString s;

	CString sFile;
	m_edtFile.GetWindowText(sFile);
	if(sFile.IsEmpty())
		return;

	Ptr data;
	ULONG fSize = 0;
	UCHAR* pData = File::LoadFile(sFile,data,fSize);
	if(!pData)
		return;

	ULONG zipLen = fSize;
	UCHAR* pZipBuf = (UCHAR*)::malloc(zipLen);
	while(true)
	{
		if(compress(pZipBuf,&zipLen,pData,fSize)==Z_OK)
			break;

		zipLen += 0x1000;
		pZipBuf = (UCHAR*)::realloc(pZipBuf,zipLen);
	}
	Ptr data1(pZipBuf);

	s.Format("%s.z",sFile);
	if(File::WriteToFile(s,pZipBuf,zipLen))
	{
		AfxMessageBox("Zip success!");
	}	
}

void CZipFileDlg::OnBnClickedUnZip()
{
	CString s;

	CString sFile;
	m_edtFile.GetWindowText(sFile);
	if(sFile.IsEmpty())
		return;

	Ptr data;
	ULONG fSize = 0;
	UCHAR* pData = File::LoadFile(sFile,data,fSize);
	if(!pData)
		return;

	ULONG bufLen = fSize;
	UCHAR* pBuf = (UCHAR*)::malloc(bufLen);
	Ptr data1(pBuf);	
	if(uncompress(pBuf,&bufLen,pData,fSize)!=Z_OK)
	{
		AfxMessageBox("UnZip failed!");
		return;
	}

	s.Format("%s.raw",sFile);
	if(File::WriteToFile(s,pBuf,bufLen))
	{
		AfxMessageBox("UnZip success!");
	}
}
