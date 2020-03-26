// ServerMoniterDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ServerMoniter.h"
#include<WinSock2.h>
#include "ExeInfo.h"
#include "ServerMoniterDlg.h"

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


// CServerMoniterDlg 对话框




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


// CServerMoniterDlg 消息处理程序

BOOL CServerMoniterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

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
	WSADATA wsaData;
	WORD wVer    = MAKEWORD(2,2);    
	WSAStartup(wVer,&wsaData);
	//设置LISTCTRL
	m_List1.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	lvc.fmt = LVCFMT_CENTER;
	lvc.pszText = "程序名";
	lvc.iSubItem = 0;	
	lvc.cx = 250;
	m_List1.InsertColumn(0,&lvc);
	lvc.pszText = "参数";
	lvc.iSubItem = 1;	
	lvc.cx = 150;
	m_List1.InsertColumn(1,&lvc);
	lvc.pszText = "间隔时间";
	lvc.iSubItem = 2;
	lvc.cx = 150;
	m_List1.InsertColumn(2,&lvc);
	lvc.pszText = "状态";
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
	
	return FALSE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CServerMoniterDlg::OnPaint()
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
//
HCURSOR CServerMoniterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CServerMoniterDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if((pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_ESCAPE))	//屏蔽enter及escape
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
		//设置监控标题
		CString strCaption = m_pExeInfoMgr->m_strServerName + _T("_ServerMoniter.ini") ;
		SetWindowText(strCaption);
	}
}

void CServerMoniterDlg::OnBnClickedBtstart()
{
	if(GetDlgItem(IDC_BTLOADCONFIG)->IsWindowEnabled() == TRUE)
		OnBnClickedBtloadconfig();

	if((m_pExeInfoMgr->GetStatus() != MoniterStatus::eLoadConfig)
		&& (m_pExeInfoMgr->GetStatus() != MoniterStatus::eExeRun))				//只有状态为1或者2的时候才能监控
	{
		AfxMessageBox(_T("装载不成功或者状态不正确，不能进行监控"));
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
	if(wParam == 0)	//启动不成功
	{
		AfxMessageBox(_T("监控启动失败"));
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
		str = _T("未启动");
		break;
	case ExeStatus::eRun:
		str = _T("运行中");
		break;
	case ExeStatus::eTerminate:
		str = _T("被中断");
		break;
	default:
		str = "未知状态";
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
