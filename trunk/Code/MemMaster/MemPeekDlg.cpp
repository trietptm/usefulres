// MemPeekDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "MemPeek.h"
#include "MemPeekDlg.h"
#include ".\mempeekdlg.h"

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


// CMemPeekDlg �Ի���



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
END_MESSAGE_MAP()


// CMemPeekDlg ��Ϣ�������

BOOL CMemPeekDlg::OnInitDialog()
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
	m_btModify.EnableWindow(FALSE);

	m_edtProcess.SetWindowText("OPRemove.exe");
	m_edtModifyAddr.SetWindowText("0041FB70");
	m_edtModifyValue.SetWindowText("B801000000C3");

	return TRUE;  // ���������˿ؼ��Ľ��㣬���򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CMemPeekDlg::OnPaint() 
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