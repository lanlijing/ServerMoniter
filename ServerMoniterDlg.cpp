// ServerMoniterDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ServerMoniter.h"
#include<WinSock2.h>
#include "ExeInfo.h"
#include "ServerMoniterDlg.h"

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


// CServerMoniterDlg �Ի���




CServerMoniterDlg::CServerMoniterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServerMoniterDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pExeInfoMgr = NULL;
}

void CServerMoniterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_List1);
	DDX_Control(pDX, IDC_EDTCONFIG, m_edtConfig);
	DDX_Control(pDX, IDC_EDTMSG, m_edtMsg);
}

BEGIN_MESSAGE_MAP(CServerMoniterDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_MYERR, OnErrMsg)
	ON_MESSAGE(WM_MONITERSTART, OnMoniterStart)
	ON_MESSAGE(WM_EXESTATUS, OnExeStatus)
	ON_BN_CLICKED(IDC_BTLOADCONFIG, &CServerMoniterDlg::OnBnClickedBtloadconfig)
	ON_BN_CLICKED(IDC_BTSTART, &CServerMoniterDlg::OnBnClickedBtstart)
	ON_BN_CLICKED(IDC_BTEND, &CServerMoniterDlg::OnBnClickedBtend)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTENDTIMER, &CServerMoniterDlg::OnBnClickedBtendtimer)
END_MESSAGE_MAP()


// CServerMoniterDlg ��Ϣ�������

BOOL CServerMoniterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

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
	WSADATA wsaData;
	WORD wVer    = MAKEWORD(2,2);    
	WSAStartup(wVer,&wsaData);
	//����LISTCTRL
	m_List1.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	lvc.fmt = LVCFMT_CENTER;
	lvc.pszText = "������";
	lvc.iSubItem = 0;	
	lvc.cx = 250;
	m_List1.InsertColumn(0,&lvc);
	lvc.pszText = "����";
	lvc.iSubItem = 1;	
	lvc.cx = 150;
	m_List1.InsertColumn(1,&lvc);
	lvc.pszText = "���ʱ��";
	lvc.iSubItem = 2;
	lvc.cx = 150;
	m_List1.InsertColumn(2,&lvc);
	lvc.pszText = "״̬";
	lvc.iSubItem = 3;
	lvc.cx = 100;
	m_List1.InsertColumn(3, &lvc);
	LVCOLUMN lvc1;
	lvc1.mask = LVCF_FMT;
	m_List1.GetColumn(0,&lvc1);	
	lvc1.fmt &= ~LVCFMT_JUSTIFYMASK;
	lvc1.fmt |= LVCFMT_CENTER;
	m_List1.SetColumn(0,&lvc1);
		
	//
	m_edtConfig.SetWindowText(_T("ServerMoniter.ini"));
	GetDlgItem(IDC_BTLOADCONFIG)->SetFocus();

	GetCurrExePath(g_strExePath);
	m_pExeInfoMgr = new CExeInfoMgr();
	
	return FALSE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CServerMoniterDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CServerMoniterDlg::OnPaint()
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
//
HCURSOR CServerMoniterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CServerMoniterDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if((pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_ESCAPE))	//����enter��escape
			return TRUE;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CServerMoniterDlg::OnBnClickedBtloadconfig()
{
	CString strFile;
	m_edtConfig.GetWindowText(strFile);
	BOOL bRet = m_pExeInfoMgr->LoadExeInfo(strFile);
	if(bRet)
	{
		GetDlgItem(IDC_BTLOADCONFIG)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTSTART)->SetFocus();

		m_pExeInfoMgr->FullListCtrl(&m_List1);
		//���ü�ر���
		CString strCaption = m_pExeInfoMgr->m_strServerName + _T("_ServerMoniter.ini") ;
		SetWindowText(strCaption);
	}
}

void CServerMoniterDlg::OnBnClickedBtstart()
{
	if(GetDlgItem(IDC_BTLOADCONFIG)->IsWindowEnabled() == TRUE)
		OnBnClickedBtloadconfig();

	if((m_pExeInfoMgr->GetStatus() != MoniterStatus::eLoadConfig)
		&& (m_pExeInfoMgr->GetStatus() != MoniterStatus::eExeRun))				//ֻ��״̬Ϊ1����2��ʱ����ܼ��
	{
		AfxMessageBox(_T("װ�ز��ɹ�����״̬����ȷ�����ܽ��м��"));
		return;
	}

	m_pExeInfoMgr->StartMoniter();
}

void CServerMoniterDlg::OnBnClickedBtend()
{
	m_pExeInfoMgr->CloseAll();

	GetDlgItem(IDC_BTLOADCONFIG)->EnableWindow(TRUE);
	GetDlgItem(IDC_BTSTART)->EnableWindow(TRUE);
}

LRESULT CServerMoniterDlg::OnErrMsg(WPARAM wParam,LPARAM lParam)
{
	char* pBuf = (char*)wParam;
	CString str;
	str.Format("%s\r\n",pBuf);

	m_edtMsg.SetSel(-1,0);
	m_edtMsg.ReplaceSel(str);

	return TRUE;
}

LRESULT CServerMoniterDlg::OnMoniterStart(WPARAM wParam,LPARAM lParam)
{
	if(wParam == 0)	//�������ɹ�
	{
		AfxMessageBox(_T("�������ʧ��"));
	}
	else
	{
		GetDlgItem(IDC_BTSTART)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTENDTIMER)->SetFocus();
	}

	return TRUE;
}

LRESULT CServerMoniterDlg::OnExeStatus(WPARAM wParam, LPARAM lParam)
{
	CString str;
	switch ((ExeStatus)lParam) {
	case ExeStatus::eIdle:
		str = _T("δ����");
		break;
	case ExeStatus::eRun:
		str = _T("������");
		break;
	case ExeStatus::eTerminate:
		str = _T("���ж�");
		break;
	default:
		str = "δ֪״̬";
	}
	int nIndex = (int)wParam;
	m_List1.SetItemText(nIndex, 3, str);

	return TRUE;
}

BOOL CServerMoniterDlg::DestroyWindow()
{
	if(m_pExeInfoMgr)
	{
		delete m_pExeInfoMgr;
		m_pExeInfoMgr = NULL;
	}

	WSACleanup();
	return CDialog::DestroyWindow();
}

void CServerMoniterDlg::OnBnClickedBtendtimer()
{
	m_pExeInfoMgr->EndMoniter();	
	::Sleep(1500);
	GetDlgItem(IDC_BTSTART)->EnableWindow(TRUE);
}
