// RecIntegDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "RecInteg.h"
#include "RecIntegDlg.h"
#include ".\recintegdlg.h"

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


// CRecIntegDlg �Ի���



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


// CRecIntegDlg ��Ϣ�������

BOOL CRecIntegDlg::OnInitDialog()
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRecIntegDlg::OnPaint() 
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
			AfxMessageBox("���������ļ�ʧ��!");
			return FALSE;
		}
	}

	File f;
	if(!f.Open(lpFile,"wb"))
	{
		AfxMessageBox("�������ݿ��ļ�ʧ��!");
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
	if(!OpenFileDlg(szFile,NULL,"��Ӽ�¼","�ı��ļ�(*.txt)\0*.txt\0"
		"�����ļ�(*.*)\0*.*\0\0"))
		return;

	CString sDBFile;
	sDBFile.Format("%s\\record.txt",gszWorkDir);

	StrStrMap db;
	LoadData(sDBFile,db);

	if(!LoadData(szFile,db))
	{
		AfxMessageBox("��������ʧ��!");
		return;
	}

	if(SaveData(sDBFile,db))
	{
		s.Format("��Ӽ�¼�ɹ���Ŀǰ����%d����¼",db.size());
		AfxMessageBox(s);
	}
	else
	{
		AfxMessageBox("�����¼ʧ��!");
	}
}