#pragma once

#ifndef _SMTPSENDEMAIL_H
#define _SMTPSENDEMAIL_H

//·¢ËÍÏß³Ì
DWORD WINAPI SendMailThread(LPVOID lpParam);

class SmtpSendEmail 
{
public: 
	SmtpSendEmail(void); 
	virtual ~SmtpSendEmail(void);

public: 
	void SetPost( int nPost );
	void SetHost( string strHost );
	void SetAccount( string strAccount );
	void SetPassword( string strPassword );
	void SetMailFrom( string strMailFrom );
	void SetSendTo( string strSendTo );
	void SetSubject( string strSubject );
	void SetDateTime( string strDateTime );
	void SetData(string strData);
	bool AddDataFromString( string strData ); 
	bool AddDataFromBuffer( char* szData, int iLen ); 
	bool AddDataFromFile( string strFileName ); 
	bool AddAttachedFile( string strFilePath, string strFileName );
	bool SandThread(); 
	bool StarMailThread();
	bool IsIdle(){return m_bIsIdle;};

private: 
	bool CheckResponse(int nResCode);
	bool ConnectServ();
	bool SendHelo();
	bool SendEhlo();
	bool AutoLogin();
	bool EmailFrom();
	bool EmailTo();
	bool DataServ();
	bool SendData();
	bool SendAttachedFile();
	bool QuitServ();

private: 
	static const string MIMEMultipartMixedLogin; 
	static const string MIMETextPlainLogin; 
	static const string MyBoundary; 
	static const string CTCodeQP; 
	static const string CTCodeBase64; 
	static const string CTTextPlainCharCodeGB2312; 
	static const string CTAppOctetStreamName; 
	static const string CDAttachemntFileName; 

	struct SMTPSTRU 
	{ 
		int    nPost; 
		string strHost; 
		string strAccount; 
		string strPassword; 
		string strMailFrom; 
		string strSendTo; 
		string strSubject; 
		string strDateTime; 
		string strData; 
	}; 
	struct FILEINFOSTRU 
	{ 
		string strPath; 
		string strFile; 
	}; 

	SOCKET             m_hSocket;
	SMTPSTRU           m_smtpPro; 
	list<FILEINFOSTRU> m_listFileInfo; 

	bool  m_bIsIdle;
};

#endif
