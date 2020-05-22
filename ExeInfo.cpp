#include "stdafx.h"
#include "ServerMoniter.h"
#include <io.h>
#include <direct.h>
#include "ExeInfo.h"
#include "SmtpSendEmail.h"

#define SMTPSERVER "smtp.qq.com"
#define SMTPPORT 25
#define SMTPSENDFROM "xxxxxx@qq.com"			// 此处换上自己的邮箱
#define SMTPUID	"xxxxxx@qq.com"					// 此处换上自己的邮箱
#define SMTPPWD "xxxxxxxx"						// 此处换上自己的邮箱验证码
#define SMTPSENDSNUM 100

CString g_strExePath = _T("");
CExeInfo::CExeInfo()
{
	m_nId = -1;
	m_Status = ExeStatus::eIdle;
	m_strShortName = _T("");

	m_strExeName = _T("");
	m_strParam = _T("");
	m_dwInterTick = 2000;

	ZeroMemory(&m_ep,sizeof(EnumParam));
}

CExeInfo::CExeInfo(int nId, CString strExeName,CString strParam,DWORD dwTick)
{
	m_nId = nId;
	m_Status = ExeStatus::eIdle;

	m_strExeName = strExeName;
	//m_strExeName.MakeUpper();
	m_strParam = strParam;
	m_dwInterTick = dwTick;
	ZeroMemory(&m_ep,sizeof(EnumParam));

	int nPos = strExeName.ReverseFind('\\');
	m_strShortName = strExeName.Right(strExeName.GetLength() - nPos - 1);	// nPos等于-1也不影响结果
}

CExeInfo::~CExeInfo()
{

}

BOOL CExeInfo::StartExe()
{
	BOOL bRet = FALSE;
	ChangeCurDir();			// 设置回当前目录
	CString strExe = g_strExePath + m_strExeName;
	CString strParams;
	strParams.Format("%s %s", strExe, m_strParam);	//保证第一个参数是文件的全路径
	if(::_access(strExe,0) == -1)
	{
		SendExeErrorMsg(m_strExeName,"不存在");
		goto retHandle;
	}
		
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));

	ChangeExeDir(strExe);			// 切换到exe目录下,为了跨目录调起
	if(!::CreateProcess(strExe,strParams.GetBuffer(strParams.GetLength() + 1),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
	{
		strParams.ReleaseBuffer();
		SendExeErrorMsg(m_strExeName,"CreateProcess失败");
		goto retHandle;
	}
	//保存相关信息	
	m_ep.dwProcessID = pi.dwProcessId;
	m_ep.hProcess = pi.hProcess;
	m_ep.hThread = pi.hThread;
	::Sleep(400);	//sleep一会，保证程序已经启动，否则，会取不到必要的窗口句柄
	EnumWindows((WNDENUMPROC)EnumWinProc,(long)&m_ep);
	//Sleep一段时间
	::Sleep(m_dwInterTick);
	bRet = TRUE;

retHandle:
	m_Status = bRet ? ExeStatus::eRun : ExeStatus::eIdle;
	SendExeStatusMsg(m_nId, m_Status);
	ChangeCurDir();			// 设置回当前目录
	return bRet;
}

BOOL CExeInfo::CheckExeRun()
{
	ExeStatus oldStatus = m_Status;
	BOOL bResult = FALSE;
	if((m_ep.hMainWnd == NULL) || (m_ep.dwProcessID == 0) || (m_ep.hProcess == NULL) || (m_ep.hThread == NULL) )
	{
		SendExeErrorMsg(m_strExeName,"未启动 CheckExeRun");
		m_Status = ExeStatus::eIdle;
		goto retHandle;
	}
	else {
		DWORD dwWait = ::WaitForSingleObject(m_ep.hProcess, 2);
		bResult = (dwWait != WAIT_OBJECT_0);							//如果返回为WAIT_OBJECT_0，则已经不在运行
		if (!bResult)
		{
			if (oldStatus != ExeStatus::eTerminate)		// 保证在非监控情况下，消息只发一次
			{
				char buf[512]; memset(buf, 0, sizeof(buf));
				_snprintf(buf, sizeof(buf) - 1, "%s意外关闭", m_strParam);
				SendExeErrorMsg(m_strExeName, buf);
			}
			m_Status = ExeStatus::eTerminate;
		}
	}

retHandle:
	if (oldStatus != m_Status)
	{
		SendExeStatusMsg(m_nId, m_Status);
	}
	return bResult;
}

void CExeInfo::CloseExe()
{
	//先关闭线程和进程(防止进程关闭了，但是任务管理器中进程还未消亡)
	::SendMessage(m_ep.hMainWnd,WM_CLOSE,0,0);
	if (::WaitForSingleObject(m_ep.hProcess,CExeInfoMgr::eWaitObjectTime) != WAIT_OBJECT_0)
		::TerminateProcess(m_ep.hProcess, 0);
	CloseHandle(m_ep.hThread);
	CloseHandle(m_ep.hProcess);
	ZeroMemory(&m_ep,sizeof(EnumParam));
	::Sleep(1000);	//休眠一段时间
}

//==============================================================================================
CExeInfoMgr::CExeInfoMgr()
{
	m_Status = MoniterStatus::eEmpty;
	m_hThread = NULL;
	m_strServerName = _T("");
	m_bThreadProc = FALSE;
	m_pSendEmails = new SmtpSendEmail[SMTPSENDSNUM];

	m_bAutoReStart = FALSE;
	m_bSendEmail = FALSE;
}

CExeInfoMgr::~CExeInfoMgr()
{
//	if(m_Status == CExeInfoMgr::eMoniterStart) 用户 有时就想只退出监控程序，不影响被启动的程序所以不CLOSEALL
//		CloseAll();
	Clear();
	if(m_pSendEmails)
	{
		delete[] m_pSendEmails;
		m_pSendEmails = NULL;
	}
}

BOOL CExeInfoMgr::LoadExeInfo(CString strFile)
{	
	CString strPath = g_strExePath + strFile;
	if(::access(strPath,0) == -1)
	{
		SendExeErrorMsg(strFile,"文件不存在");
		return FALSE;
	}

	Clear();

	CString strRead = _T(""),strExeName = _T(""),strParam = _T("");
	DWORD dwTick = 0;
	CStdioFile fl;
	if(!(fl.Open((LPCTSTR)strPath,CFile::modeRead|CFile::typeText)))
	{
		SendExeErrorMsg(strFile,"cstdiofile open error");
		return FALSE;
	}
	//先读取第一行，第一行是服务器名字
	fl.ReadString(m_strServerName);
	//第二行，是否自动重启所有服务器
	fl.ReadString(strRead);
	m_bAutoReStart = (atoi(strRead) != 0);
	//第三行 是否自动发送邮件
	fl.ReadString(strRead);
	m_bSendEmail = (atoi(strRead) != 0);
	//第四行，要发送的报警邮箱列表
	fl.ReadString(strRead);
	ExtractString2(strRead);
	//
	int nId = 0;
	while(fl.ReadString(strRead))
	{
		if(!ExtractString(strRead,strExeName,strParam,dwTick))
			goto errHandler;
		CExeInfo* pNewInfo = new CExeInfo(nId, strExeName,strParam,dwTick);	//在Clear中删除
		m_vecExeInfo.push_back(pNewInfo);
		nId++;
	}
	fl.Close();

	m_Status = MoniterStatus::eLoadConfig;
	return TRUE;

errHandler:
	Clear();
	fl.Close();
	SendExeErrorMsg(strFile,"LoadExeInfo不成功");
	m_Status = MoniterStatus::eEmpty;
	return FALSE;
}
	
BOOL CExeInfoMgr::StartMoniter()
{
	//要判断是否已经启动了线程
	if(!m_hThread)
	{
		DWORD dwThreadID = 0;
		m_bThreadProc = TRUE;
		m_hThread = ::CreateThread(NULL,NULL,CExeInfoMgr::MoniterThreadProc,this,NULL,&dwThreadID);
		return TRUE;
	}

	if (m_Status != MoniterStatus::eMoniterStart) {
		m_Status = MoniterStatus::eMoniterStart;
		theApp.m_pMainWnd->SendMessage(WM_MONITERSTART, TRUE, 0);
	}

	return TRUE;
}

void CExeInfoMgr::EndMoniter()
{
	if(m_Status == MoniterStatus::eMoniterStart)
		m_Status = MoniterStatus::eExeRun;
}

void CExeInfoMgr::CheckRun()
{
	for(int i = 0; i < m_vecExeInfo.size(); i++)
	{
		CExeInfo* pInfo = m_vecExeInfo[i];
		if(!(pInfo->CheckExeRun()))
		{
			//发送邮件
			if(m_bSendEmail && m_Status == MoniterStatus::eMoniterStart)
				SendAlertMail(pInfo->m_strExeName);

			if(m_bAutoReStart && m_Status == MoniterStatus::eMoniterStart)
			{
				//版本一，一个程序挂掉，全部程序重启
				/*
				// 关闭所有进程
				for(int j = (m_vecExeInfo.size() - 1); j >= 0; j--)	//从队列的后面往前面关闭
					m_vecExeInfo[j]->CloseExe();
				TaskKillAll();	//确保关闭干净
				// 重启动所有进程
				for(int m = 0; m < m_vecExeInfo.size();m++)	//从队列的前面往后面面启动
					m_vecExeInfo[m]->StartExe();
					*/
				// 版本二，一个程序挂掉，只重启这个程序
				pInfo->CloseExe();
				pInfo->StartExe();
			}
		}	
	}	
}

void CExeInfoMgr::FullListCtrl(CListCtrl* pList)
{
	pList->DeleteAllItems();	//先清除

	CString strInter = _T("");
	for(int i = 0; i < m_vecExeInfo.size(); i++)
	{
		pList->InsertItem(i,m_vecExeInfo[i]->m_strExeName);
		pList->SetItemText(i,1,m_vecExeInfo[i]->m_strParam);
		strInter.Format("%d",m_vecExeInfo[i]->m_dwInterTick);
		pList->SetItemText(i,2,strInter);
	}
}

void CExeInfoMgr::CloseAll()
{
	if (m_hThread)
		CleanThread();

	m_Status = MoniterStatus::eLoadConfig;	

	for(int i = (m_vecExeInfo.size() - 1); i >= 0; i--)	//从队列的后面往前面关闭
		m_vecExeInfo[i]->CloseExe();

	//再执行一次tasklist,确保已经全部关掉
	TaskKillAll();
}

BOOL CExeInfoMgr::RunExe()
{	
	for(int i = 0; i < m_vecExeInfo.size(); i++)
	{
		if( !(m_vecExeInfo[i]->StartExe()))
			goto errHandler;
	}
	
	m_Status = MoniterStatus::eExeRun;	//这个时候是启动了程序	
	return TRUE;

errHandler:
	CloseAll();
	return FALSE;
}

void CExeInfoMgr::Clear()
{
	for(int i = 0; i < m_vecExeInfo.size(); i++)
	{
		delete m_vecExeInfo[i];
		m_vecExeInfo[i] = NULL;
	}

	m_vecExeInfo.clear();
	m_bAutoReStart = FALSE;
	m_bSendEmail = FALSE;
	m_vecRecvEmailAddr.clear();

	m_Status = MoniterStatus::eEmpty;
}

BOOL CExeInfoMgr::ExtractString(CString strIn,CString& strExeName,CString& strParam,DWORD& dwTick)
{
	char* szToken;	//用于切割
	CString strTick = _T("");
	char spes[] = " ";	//切割的子字符串
	int nInLen = strIn.GetLength();

	strExeName.Empty();
	strParam.Empty();
	dwTick = 0;

	szToken = ::strtok(strIn.GetBuffer(nInLen + 1),spes);
	if(!szToken)
		goto errHandle;
	strExeName.Format("%s",szToken);
	szToken = ::strtok(NULL,spes);
	if(!szToken)
		goto errHandle;
	strParam.Format("%s",szToken);
	szToken = ::strtok(NULL,spes);
	if(!szToken)
		goto errHandle;
	strTick.Format("%s",szToken);

	dwTick = atoi(strTick);
	if(dwTick <= 0)
		goto errHandle;

	strIn.ReleaseBuffer();
	return TRUE;

errHandle:
	strIn.ReleaseBuffer();
	return FALSE;
}

BOOL CExeInfoMgr::ExtractString2(CString strIn)
{
	char* szToken;	//用于切割
	char spes[] = " ";	//切割的子字符串
	int nInLen = strIn.GetLength();

	szToken = ::strtok(strIn.GetBuffer(nInLen + 1),spes);
	while(szToken)
	{
		string strAddr = szToken;
		m_vecRecvEmailAddr.push_back(strAddr);
		szToken = ::strtok(NULL,spes);
	}

	return TRUE;
}

void CExeInfoMgr::SendAlertMail(const char* szExeName)
{
	char szContent[1024] = {0};
	if(m_bAutoReStart)
		_snprintf(szContent,(sizeof(szContent) - 1),"%s:服务器程序%s异常关闭,根据配置,进行了自动重启,本邮件由监控程序自动发送",
		m_strServerName,szExeName);
	else
		_snprintf(szContent,(sizeof(szContent) - 1),"%s:服务器程序%s异常关闭,根据配置,没有进行自动重启,本邮件由监控程序自动发送",
		m_strServerName,szExeName);
	
	SYSTEMTIME systm;
	GetLocalTime(&systm);
	char szTime[256] = {0};
	_snprintf(szTime,sizeof(szTime) - 1,"%d-%d-%d %d:%d:%d",systm.wYear,systm.wMonth,systm.wDay,systm.wHour,systm.wMinute,systm.wSecond);
	
	for(int i = 0; i < m_vecRecvEmailAddr.size(); i++)
	{
		string strSendTo = m_vecRecvEmailAddr[i];
		//取出空闲的邮件发送器
		SmtpSendEmail* pSender = NULL;
		for(int k = 0; k < SMTPSENDSNUM; k++)
		{
			if(m_pSendEmails[k].IsIdle())
			{
				pSender = &(m_pSendEmails[k]);
				break;
			}
		}
		if(!pSender)
			return;
		//
		pSender->SetHost(SMTPSERVER);
		pSender->SetPost(SMTPPORT);
		pSender->SetAccount(SMTPUID);
		pSender->SetPassword(SMTPPWD);
		pSender->SetMailFrom(SMTPSENDFROM);
		pSender->SetSendTo(strSendTo);
		pSender->SetSubject("服务器程序意外关闭报警");
		pSender->SetDateTime(szTime);
		pSender->SetData(szContent);
		pSender->StarMailThread();
	}
}

void CExeInfoMgr::TaskKillAll()
{
	char szCmd[256]; memset(szCmd, 0, sizeof(szCmd));
	for(int i = (m_vecExeInfo.size() - 1); i >= 0; i--)	
	{
		_snprintf(szCmd,sizeof(szCmd) - 1,"taskkill /im %s /t",m_vecExeInfo[i]->m_strShortName);
		system(szCmd);
	}
}

DWORD WINAPI CExeInfoMgr::MoniterThreadProc(void* pParam)
{
	CExeInfoMgr* pThis = (CExeInfoMgr*)pParam;
	if(pThis->GetStatus() != MoniterStatus::eLoadConfig)
	{
		pThis->CleanThread();
		return 1;
	}

	BOOL bStart = pThis->RunExe();
	if(!bStart)
	{
		SendExeErrorMsg("CExeInfoMgr","RunExe 失败，未能启动监控");
		pThis->CleanThread();
		return 1;
	}
	theApp.m_pMainWnd->SendMessage(WM_MONITERSTART, bStart, 0);

	pThis->SetStatus(MoniterStatus::eMoniterStart);	//开始监控
	while(pThis->m_bThreadProc)
	{
		pThis->CheckRun();

		::Sleep(1000);
	}

	return 0;
}

//===============================================================================
void SendExeErrorMsg(CString strExeName,const char* szError)
{
	SYSTEMTIME curTime;
	GetLocalTime(&curTime);

	char buf[1024] = {0};
	_snprintf(buf,sizeof(buf) - 1,"%04d-%02d-%02d %02d:%02d:%02d %s:%s",curTime.wYear, curTime.wMonth, curTime.wDay,
		curTime.wHour, curTime.wMinute, curTime.wSecond,strExeName,szError);

	theApp.m_pMainWnd->SendMessage(WM_MYERR,(WPARAM)buf,0);
}

void SendExeStatusMsg(int nId, ExeStatus status)
{
	if (nId < 0)
	{
		char szMsg[200]; memset(szMsg, 0, sizeof(szMsg));
		_snprintf(szMsg, (sizeof(szMsg) - 1), "ExeStatusMsg error.Id:%d", nId);
		SendExeErrorMsg("ExeStatusMsg", szMsg);
		return;
	}

	WPARAM wParam = (WPARAM)nId;
	LPARAM lParam = (LPARAM)status;
	theApp.m_pMainWnd->SendMessage(WM_EXESTATUS, wParam, lParam);
}

BOOL WINAPI EnumWinProc(HWND hWnd,LPARAM lParam)
{
	DWORD dwID = 0; 
	EnumParam* pep = (EnumParam*)lParam;   
	::GetWindowThreadProcessId(hWnd,&dwID);   
	if(dwID == pep->dwProcessID)  
	{   
		pep->hMainWnd = hWnd;
		return FALSE;
	}   

	return TRUE;
}

void GetCurrExePath(CString& strPath)
{
	strPath.Empty();

	char buf[512] = {0};
	::GetModuleFileName(NULL,buf,511);

	size_t nLen = strlen(buf);
	nLen -= 2;		//最后一位是\0
	while(true)
	{
		if(buf[nLen] == '\\')
			break;
		--nLen;
	}
	buf[nLen + 1] = '\0';	//只保留路径，可执行文件名不要

	strPath = buf;
}

void ChangeCurDir()
{
	char szCurrentDir[512] = { 0 };
	GetModuleFileNameA(GetModuleHandleA(NULL), szCurrentDir, sizeof(szCurrentDir) - 1);
	char* szPos = strrchr(szCurrentDir, '\\');
	*(szPos + 1) = 0;
	_chdir(szCurrentDir);
}

void ChangeExeDir(CString exePath)
{
	int nLen = exePath.GetLength();
	char* szPath = exePath.GetBuffer(nLen + 1);
	char* szPos = strrchr(szPath, '\\');
	*(szPos + 1) = 0;
	_chdir(szPath);
}
