#ifndef _EXEINFO_H_
#define _EXEINFO_H_

#pragma once

#define WM_MYERR WM_USER + 111		//错误的消息
#define WM_MONITERSTART	WM_USER + 112	//完成监控启动
#define WM_EXESTATUS WM_USER + 113		// exe状态改变

#include "DLockerBuff.h"

class SmtpSendEmail;
struct	EnumParam   
{   
	HWND	hMainWnd;   
	HANDLE  hThread;
	DWORD	dwProcessID;
	HANDLE	hProcess;
};   

enum MoniterStatus
{
	eEmpty = 0,	//空
	eLoadConfig,	//已经装载了配置
	eExeRun,		//exe已经启动
	eMoniterStart,	//监控已经启动
};

enum ExeStatus
{
	eIdle = 0,			// 未启动
	eRun,				// 运行中
	eTerminate,			// 意外中止
};

class CExeInfo
{
public:
	CExeInfo();
	CExeInfo(int nId, CString strExeName,CString strParam,DWORD dwTick);
	~CExeInfo();

	BOOL StartExe();		//启动程序
	BOOL CheckExeRun();		//检查程序是否在运行
	void CloseExe();		//闭门应用程序

	BOOL ValidId() { return m_nId >= 0; };

public:
	int m_nId;				// id,从0开始
	ExeStatus m_Status;		// 状态,0停止,1运行
	CString m_strShortName;		// 短名字，用于Kill

	CString m_strExeName;	//程序名 (长名字,可能有相对路径)
	CString m_strParam;		//启动参数
	DWORD	m_dwInterTick;	//间隔毫秒数

	EnumParam	m_ep;	//启动的进程消息
};

class CExeInfoMgr
{
public:
	CExeInfoMgr();
	~CExeInfoMgr();

public:
	enum{eWaitObjectTime = 2500,};

	CString m_strServerName;
public:
	BOOL LoadExeInfo(CString strFile);
	BOOL StartMoniter();
	void EndMoniter();	
	void CloseAll();

	void FullListCtrl(CListCtrl* pList);
	void SetStatus(MoniterStatus status)
	{
		DAutoLock autoLock(m_lockStatus);
		m_Status = status;
	};

	MoniterStatus GetStatus(){return m_Status;};
private:
	void CheckRun();
	BOOL RunExe();
	void Clear();
	BOOL ExtractString(CString strIn,CString& strExeName,CString& strParam,DWORD& dwTick);	//把收到的字符串分割
	BOOL ExtractString2(CString strIn);
	void CleanThread()
	{
		m_bThreadProc = FALSE;
		::WaitForSingleObject(m_hThread,eWaitObjectTime);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	void SendAlertMail(const char* szExeName);
	void TaskKillAll();	//用来确保关闭干净

	static DWORD WINAPI MoniterThreadProc(void* pParam);	
private:
	MoniterStatus		m_Status;		//0，普通状态，1，已经装载了文件，2程序运行 3程序运行并且使用了监控
	vector<CExeInfo*> m_vecExeInfo;

	HANDLE		m_hThread;
	DLockerBuff m_lockStatus;

	BOOL m_bThreadProc;
	SmtpSendEmail* m_pSendEmails;
	//以下是配置变量
	BOOL m_bAutoReStart;						//是否自动重启
	BOOL m_bSendEmail;							//服务器程序当机后是否会发报警邮件
	vector<string> m_vecRecvEmailAddr;			//自动重启后，发送邮件的目标邮箱列表
};

//
extern CString g_strExePath;
void SendExeErrorMsg(CString strExeName,const char* szError);
void SendExeStatusMsg(int nId, ExeStatus status);
BOOL WINAPI EnumWinProc(HWND hWnd,LPARAM lParam);
void GetCurrExePath(CString& strPath);
void ChangeCurDir();
void ChangeExeDir(CString exePath);

#endif