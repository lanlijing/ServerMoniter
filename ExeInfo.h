#ifndef _EXEINFO_H_
#define _EXEINFO_H_

#pragma once

#define WM_MYERR WM_USER + 111		//�������Ϣ
#define WM_MONITERSTART	WM_USER + 112	//��ɼ������
#define WM_EXESTATUS WM_USER + 113		// exe״̬�ı�

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
	eEmpty = 0,	//��
	eLoadConfig,	//�Ѿ�װ��������
	eExeRun,		//exe�Ѿ�����
	eMoniterStart,	//����Ѿ�����
};

enum ExeStatus
{
	eIdle = 0,			// δ����
	eRun,				// ������
	eTerminate,			// ������ֹ
};

class CExeInfo
{
public:
	CExeInfo();
	CExeInfo(int nId, CString strExeName,CString strParam,DWORD dwTick);
	~CExeInfo();

	BOOL StartExe();		//��������
	BOOL CheckExeRun();		//�������Ƿ�������
	void CloseExe();		//����Ӧ�ó���

	BOOL ValidId() { return m_nId >= 0; };

public:
	int m_nId;				// id,��0��ʼ
	ExeStatus m_Status;		// ״̬,0ֹͣ,1����
	CString m_strShortName;		// �����֣�����Kill

	CString m_strExeName;	//������ (������,���������·��)
	CString m_strParam;		//��������
	DWORD	m_dwInterTick;	//���������

	EnumParam	m_ep;	//�����Ľ�����Ϣ
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
	BOOL ExtractString(CString strIn,CString& strExeName,CString& strParam,DWORD& dwTick);	//���յ����ַ����ָ�
	BOOL ExtractString2(CString strIn);
	void CleanThread()
	{
		m_bThreadProc = FALSE;
		::WaitForSingleObject(m_hThread,eWaitObjectTime);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	void SendAlertMail(const char* szExeName);
	void TaskKillAll();	//����ȷ���رոɾ�

	static DWORD WINAPI MoniterThreadProc(void* pParam);	
private:
	MoniterStatus		m_Status;		//0����ͨ״̬��1���Ѿ�װ�����ļ���2�������� 3�������в���ʹ���˼��
	vector<CExeInfo*> m_vecExeInfo;

	HANDLE		m_hThread;
	DLockerBuff m_lockStatus;

	BOOL m_bThreadProc;
	SmtpSendEmail* m_pSendEmails;
	//���������ñ���
	BOOL m_bAutoReStart;						//�Ƿ��Զ�����
	BOOL m_bSendEmail;							//���������򵱻����Ƿ�ᷢ�����ʼ�
	vector<string> m_vecRecvEmailAddr;			//�Զ������󣬷����ʼ���Ŀ�������б�
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