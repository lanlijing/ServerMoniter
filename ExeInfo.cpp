#include "stdafx.h"
#include "ServerMoniter.h"
#include <io.h>
#include <direct.h>
#include "ExeInfo.h"
#include "SmtpSendEmail.h"

#define SMTPSERVER "smtp.qq.com"
#define SMTPPORT 25
#define SMTPSENDFROM "xxxxxx@qq.com"			// �˴������Լ�������
#define SMTPUID	"xxxxxx@qq.com"					// �˴������Լ�������
#define SMTPPWD "xxxxxxxx"						// �˴������Լ���������֤��
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
	m_strShortName = strExeName.Right(strExeName.GetLength() - nPos - 1);	// nPos����-1Ҳ��Ӱ����
}

CExeInfo::~CExeInfo()
{

}

BOOL CExeInfo::StartExe()
{
	BOOL bRet = FALSE;
	ChangeCurDir();			// ���ûص�ǰĿ¼
	CString strExe = g_strExePath + m_strExeName;
	CString strParams;
	strParams.Format("%s %s", strExe, m_strParam);	//��֤��һ���������ļ���ȫ·��
	if(::_access(strExe,0) == -1)
	{
		SendExeErrorMsg(m_strExeName,"������");
		goto retHandle;
	}
		
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	ZeroMemory(&pi,sizeof(PROCESS_INFORMATION));

	ChangeExeDir(strExe);			// �л���exeĿ¼��,Ϊ�˿�Ŀ¼����
	if(!::CreateProcess(strExe,strParams.GetBuffer(strParams.GetLength() + 1),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
	{
		strParams.ReleaseBuffer();
		SendExeErrorMsg(m_strExeName,"CreateProcessʧ��");
		goto retHandle;
	}
	//���������Ϣ	
	m_ep.dwProcessID = pi.dwProcessId;
	m_ep.hProcess = pi.hProcess;
	m_ep.hThread = pi.hThread;
	::Sleep(400);	//sleepһ�ᣬ��֤�����Ѿ����������򣬻�ȡ������Ҫ�Ĵ��ھ��
	EnumWindows((WNDENUMPROC)EnumWinProc,(long)&m_ep);
	//Sleepһ��ʱ��
	::Sleep(m_dwInterTick);
	bRet = TRUE;

retHandle:
	m_Status = bRet ? ExeStatus::eRun : ExeStatus::eIdle;
	SendExeStatusMsg(m_nId, m_Status);
	ChangeCurDir();			// ���ûص�ǰĿ¼
	return bRet;
}

BOOL CExeInfo::CheckExeRun()
{
	ExeStatus oldStatus = m_Status;
	BOOL bResult = FALSE;
	if((m_ep.hMainWnd == NULL) || (m_ep.dwProcessID == 0) || (m_ep.hProcess == NULL) || (m_ep.hThread == NULL) )
	{
		SendExeErrorMsg(m_strExeName,"δ���� CheckExeRun");
		m_Status = ExeStatus::eIdle;
		goto retHandle;
	}
	else {
		DWORD dwWait = ::WaitForSingleObject(m_ep.hProcess, 2);
		bResult = (dwWait != WAIT_OBJECT_0);							//�������ΪWAIT_OBJECT_0�����Ѿ���������
		if (!bResult)
		{
			if (oldStatus != ExeStatus::eTerminate)		// ��֤�ڷǼ������£���Ϣֻ��һ��
			{
				char buf[512]; memset(buf, 0, sizeof(buf));
				_snprintf(buf, sizeof(buf) - 1, "%s����ر�", m_strParam);
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
	//�ȹر��̺߳ͽ���(��ֹ���̹ر��ˣ���������������н��̻�δ����)
	::SendMessage(m_ep.hMainWnd,WM_CLOSE,0,0);
	if (::WaitForSingleObject(m_ep.hProcess,CExeInfoMgr::eWaitObjectTime) != WAIT_OBJECT_0)
		::TerminateProcess(m_ep.hProcess, 0);
	CloseHandle(m_ep.hThread);
	CloseHandle(m_ep.hProcess);
	ZeroMemory(&m_ep,sizeof(EnumParam));
	::Sleep(1000);	//����һ��ʱ��
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
//	if(m_Status == CExeInfoMgr::eMoniterStart) �û� ��ʱ����ֻ�˳���س��򣬲�Ӱ�챻�����ĳ������Բ�CLOSEALL
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
		SendExeErrorMsg(strFile,"�ļ�������");
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
	//�ȶ�ȡ��һ�У���һ���Ƿ���������
	fl.ReadString(m_strServerName);
	//�ڶ��У��Ƿ��Զ��������з�����
	fl.ReadString(strRead);
	m_bAutoReStart = (atoi(strRead) != 0);
	//������ �Ƿ��Զ������ʼ�
	fl.ReadString(strRead);
	m_bSendEmail = (atoi(strRead) != 0);
	//�����У�Ҫ���͵ı��������б�
	fl.ReadString(strRead);
	ExtractString2(strRead);
	//
	int nId = 0;
	while(fl.ReadString(strRead))
	{
		if(!ExtractString(strRead,strExeName,strParam,dwTick))
			goto errHandler;
		CExeInfo* pNewInfo = new CExeInfo(nId, strExeName,strParam,dwTick);	//��Clear��ɾ��
		m_vecExeInfo.push_back(pNewInfo);
		nId++;
	}
	fl.Close();

	m_Status = MoniterStatus::eLoadConfig;
	return TRUE;

errHandler:
	Clear();
	fl.Close();
	SendExeErrorMsg(strFile,"LoadExeInfo���ɹ�");
	m_Status = MoniterStatus::eEmpty;
	return FALSE;
}
	
BOOL CExeInfoMgr::StartMoniter()
{
	//Ҫ�ж��Ƿ��Ѿ��������߳�
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
			//�����ʼ�
			if(m_bSendEmail && m_Status == MoniterStatus::eMoniterStart)
				SendAlertMail(pInfo->m_strExeName);

			if(m_bAutoReStart && m_Status == MoniterStatus::eMoniterStart)
			{
				//�汾һ��һ������ҵ���ȫ����������
				/*
				// �ر����н���
				for(int j = (m_vecExeInfo.size() - 1); j >= 0; j--)	//�Ӷ��еĺ�����ǰ��ر�
					m_vecExeInfo[j]->CloseExe();
				TaskKillAll();	//ȷ���رոɾ�
				// ���������н���
				for(int m = 0; m < m_vecExeInfo.size();m++)	//�Ӷ��е�ǰ��������������
					m_vecExeInfo[m]->StartExe();
					*/
				// �汾����һ������ҵ���ֻ�����������
				pInfo->CloseExe();
				pInfo->StartExe();
			}
		}	
	}	
}

void CExeInfoMgr::FullListCtrl(CListCtrl* pList)
{
	pList->DeleteAllItems();	//�����

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

	for(int i = (m_vecExeInfo.size() - 1); i >= 0; i--)	//�Ӷ��еĺ�����ǰ��ر�
		m_vecExeInfo[i]->CloseExe();

	//��ִ��һ��tasklist,ȷ���Ѿ�ȫ���ص�
	TaskKillAll();
}

BOOL CExeInfoMgr::RunExe()
{	
	for(int i = 0; i < m_vecExeInfo.size(); i++)
	{
		if( !(m_vecExeInfo[i]->StartExe()))
			goto errHandler;
	}
	
	m_Status = MoniterStatus::eExeRun;	//���ʱ���������˳���	
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
	char* szToken;	//�����и�
	CString strTick = _T("");
	char spes[] = " ";	//�и�����ַ���
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
	char* szToken;	//�����и�
	char spes[] = " ";	//�и�����ַ���
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
		_snprintf(szContent,(sizeof(szContent) - 1),"%s:����������%s�쳣�ر�,��������,�������Զ�����,���ʼ��ɼ�س����Զ�����",
		m_strServerName,szExeName);
	else
		_snprintf(szContent,(sizeof(szContent) - 1),"%s:����������%s�쳣�ر�,��������,û�н����Զ�����,���ʼ��ɼ�س����Զ�����",
		m_strServerName,szExeName);
	
	SYSTEMTIME systm;
	GetLocalTime(&systm);
	char szTime[256] = {0};
	_snprintf(szTime,sizeof(szTime) - 1,"%d-%d-%d %d:%d:%d",systm.wYear,systm.wMonth,systm.wDay,systm.wHour,systm.wMinute,systm.wSecond);
	
	for(int i = 0; i < m_vecRecvEmailAddr.size(); i++)
	{
		string strSendTo = m_vecRecvEmailAddr[i];
		//ȡ�����е��ʼ�������
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
		pSender->SetSubject("��������������رձ���");
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
		SendExeErrorMsg("CExeInfoMgr","RunExe ʧ�ܣ�δ���������");
		pThis->CleanThread();
		return 1;
	}
	theApp.m_pMainWnd->SendMessage(WM_MONITERSTART, bStart, 0);

	pThis->SetStatus(MoniterStatus::eMoniterStart);	//��ʼ���
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
	nLen -= 2;		//���һλ��\0
	while(true)
	{
		if(buf[nLen] == '\\')
			break;
		--nLen;
	}
	buf[nLen + 1] = '\0';	//ֻ����·������ִ���ļ�����Ҫ

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
