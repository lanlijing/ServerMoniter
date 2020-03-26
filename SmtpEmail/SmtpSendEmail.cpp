#include "StdAfx.h"
#include "SmtpSendEmail.h"
#include "ZBase64.h"

//发送线程
DWORD WINAPI SendMailThread(LPVOID lpParam)
{
	((SmtpSendEmail*)lpParam)->SandThread();
	return 0;
}

const string SmtpSendEmail::MIMEMultipartMixedLogin   = "X-Mailer: Gddsky mailer\r\n MIME-Version: 1.0\r\nContent-type: multipart/mixed;boundary=\"===_000_Gddsky_000_===\"\r\n\r\n";
const string SmtpSendEmail::MIMETextPlainLogin        = "X-Mailer: Gddsky mailer\r\nMIME-Version: 1.0\r\nContent-type: text/plain;charset=\"GB2312\"\r\n Content-Transfer-Encoding: base64\r\n\r\n"; 
const string SmtpSendEmail::MyBoundary                = "\r\n--===_000_Gddsky_000_===\r\n"; 
const string SmtpSendEmail::CTCodeQP                  = "Content-Transfer-Encoding: quoted-printable\r\n\r\n"; 
const string SmtpSendEmail::CTCodeBase64              = "Content-Transfer-Encoding: base64\r\n\r\n"; 
const string SmtpSendEmail::CTTextPlainCharCodeGB2312 = "Content-Type: text/plain;charset=\"GB2312\"\r\n"; 
const string SmtpSendEmail::CTAppOctetStreamName      = "Content-Type: application/octet-stream;name="; 
const string SmtpSendEmail::CDAttachemntFileName      = "Content-Disposition: attachment;filename=";

SmtpSendEmail::SmtpSendEmail(void) 
{ 	
	m_smtpPro.nPost = 25;
	m_hSocket = 0;
	m_bIsIdle = TRUE;
} 

SmtpSendEmail::~SmtpSendEmail(void) 
{ 
	m_listFileInfo.clear(); 
	//关闭SOCK
	closesocket(m_hSocket);
	m_hSocket = 0;	
} 

void SmtpSendEmail::SetPost( int nPost )
{ 
	m_smtpPro.nPost = nPost; 
}

void SmtpSendEmail::SetHost( string strHost )
{
	m_smtpPro.strHost = strHost; 
}

void SmtpSendEmail::SetAccount( string strAccount )
{
	m_smtpPro.strAccount = strAccount; 
}

void SmtpSendEmail::SetPassword( string strPassword )
{ 
	m_smtpPro.strPassword = strPassword;
}

void SmtpSendEmail::SetMailFrom( string strMailFrom )
{ 
	m_smtpPro.strMailFrom = strMailFrom; 
}

void SmtpSendEmail::SetSendTo( string strSendTo )
{ 
	m_smtpPro.strSendTo = strSendTo;    
}

void SmtpSendEmail::SetSubject( string strSubject )
{ 
	m_smtpPro.strSubject = strSubject;   
}

void SmtpSendEmail::SetDateTime( string strDateTime )
{ 
	m_smtpPro.strDateTime = strDateTime;
}

void SmtpSendEmail::SetData(string strData)
{
	m_smtpPro.strData = strData;
}

bool SmtpSendEmail::AddDataFromString( std::string strData ) 
{ 
	m_smtpPro.strData.append( strData.c_str() ); 
	return true; 
} 

bool SmtpSendEmail::AddDataFromBuffer( char* szData, int iLen ) 
{ 
	if( NULL != szData && iLen > 0) 
	{ 
		m_smtpPro.strData.append( szData, iLen ); 
		return true; 
	} 

	return false; 
} 

bool SmtpSendEmail::AddDataFromFile( string strFileName ) 
{ 
	ifstream InputFile; 
	InputFile.open( strFileName.c_str(), ios_base::binary | ios_base::in ); 
	if( InputFile.fail() ) 
	{ 
		return false; 
	} 
	InputFile.seekg( 0, ios_base::end ); 
	int iLen = InputFile.tellg(); 
	InputFile.seekg( 0, ios_base::beg ); 

	char* pszFileData = new char[iLen + 1]; 
	memset(pszFileData, 0, iLen + 1);
	InputFile.read( pszFileData, iLen ); 
	m_smtpPro.strData.append( pszFileData ); 
	delete pszFileData;

	return true; 
} 

bool SmtpSendEmail::AddAttachedFile(string strFilePath, string strFileName) 
{ 
	FILEINFOSTRU fileInfo; 
	fileInfo.strPath = strFilePath; 
	fileInfo.strFile = strFileName; 
	m_listFileInfo.push_back(fileInfo); 
	return true;
}

bool SmtpSendEmail::StarMailThread()
{
	m_bIsIdle = FALSE;
	//创建SOCK，连接服务器
	if (!ConnectServ())
	{
		m_bIsIdle = true;
		return false;
	}

	DWORD threadID;
	HANDLE threadHandle = CreateThread(NULL, 0, SendMailThread, this, 0, &threadID);
	return true;
}

// 发送邮件 
bool SmtpSendEmail::SandThread() 
{	
	//HELO CMD
	if(!SendHelo())
	{
		return false;
	}
	//EHLO CMD
	if (SendEhlo())
	{
		if (!AutoLogin())
		{
			return false;
		}
	}
	//MAIL FORM CMD
	if (!EmailFrom())
	{
		return false;
	}
	//RCPT TO CMD
	if (!EmailTo())
	{
		return false;
	}
	//DATA CMD
	if (!DataServ())
	{
		return false;
	}
	// 邮件内容
	if (m_listFileInfo.size() > 0)
	{
		if (!SendAttachedFile())
		{
			return false;
		}
	}
	else
	{
		if (!SendData())
		{
			return false;
		}
	}

	// 发送结束 
	if (!QuitServ())
	{
		return false;
	}

	//关闭SOCK
	closesocket(m_hSocket);
	m_hSocket = 0;

	m_bIsIdle = TRUE;

	return true; 
}

bool SmtpSendEmail::ConnectServ()
{
	//创建SOCK
	m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int Err = ::WSAGetLastError();

	//超时
	struct timeval timeoutRecv = {0};
	timeoutRecv.tv_sec = 30000;
	setsockopt(m_hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeoutRecv, sizeof(timeoutRecv));

	//连接
	SOCKADDR_IN saServer;
	memset(&saServer,0,sizeof(SOCKADDR_IN));
	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = ::inet_addr(m_smtpPro.strHost.c_str());
	if (saServer.sin_addr.s_addr == INADDR_NONE) // 域名转IP代码
	{
		LPHOSTENT lphost;
		lphost = ::gethostbyname(m_smtpPro.strHost.c_str());
		if (lphost != NULL)
			saServer.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			return false;
		}
	}
	saServer.sin_port = ::htons(m_smtpPro.nPost);
	if(connect(m_hSocket,(const sockaddr*)&saServer, sizeof(saServer)) == SOCKET_ERROR)
	{
		DWORD dwErr = ::WSAGetLastError();
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}
	
	return CheckResponse(220);
}

bool SmtpSendEmail::CheckResponse(int nResCode)
{
	char szRecvBuf[1024] = {0};
	char szTemp[20] = {0};
	_itoa_s(nResCode, szTemp, 20, 10);
	//接收回复
	int nRecvLen = recv(m_hSocket, szRecvBuf, 1024, 0);
	if (nRecvLen <= 0)
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	if (nRecvLen >= (int)strlen(szTemp))
	{
		if (strstr(szRecvBuf, szTemp) != NULL)
		{
			return true;
		}
	}
	return false;
}

bool SmtpSendEmail::SendHelo()
{
	string strSendBuf;
	strSendBuf.append( "HELO " );
	strSendBuf.append(m_smtpPro.strHost.c_str());
	strSendBuf.append("\r\n");
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(250);
}

bool SmtpSendEmail::SendEhlo()
{
	string strSendBuf;
	strSendBuf.append( "EHLO 126.com \r\n" );
	//strSendBuf.append(m_smtpPro.Host.c_str());
	//strSendBuf.append("\r\n");
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(250);
}

bool SmtpSendEmail::AutoLogin()
{
	int nTemp = 0;
	string strSendBuf;
	strSendBuf.append( "AUTH LOGIN\r\n" ); 
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}
	//回复
	if (!CheckResponse(334))
	{
		return false;
	}

	//帐户
	ZBase64 zBase;
	strSendBuf.clear();
	string strBase64 = zBase.Encode((const BYTE*)(m_smtpPro.strAccount.c_str()),(int)m_smtpPro.strAccount.size());
	strSendBuf.append( strBase64.c_str() );
	strSendBuf.append("\r\n");
	strBase64 = zBase.Decode(strBase64.c_str(),(int)strBase64.size(),nTemp);
	//发送
	nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}
	//回复
	if (!CheckResponse(334))
	{
		return false;
	}


	//密码
	strSendBuf.clear();
	strBase64 = zBase.Encode((const BYTE*)(m_smtpPro.strPassword.c_str()),(int)m_smtpPro.strPassword.size());
	strSendBuf.append( strBase64.c_str() );
	strSendBuf.append("\r\n");
	//发送
	nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(235);
}

bool SmtpSendEmail::EmailFrom()
{
	string strSendBuf;
	strSendBuf.append( "MAIL FROM: <" ); 
	strSendBuf.append( m_smtpPro.strMailFrom.c_str() ); 
	strSendBuf.append( ">\r\n" ); 
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(250);
}

bool SmtpSendEmail::EmailTo()
{
	string strSendBuf;
	strSendBuf.append( "RCPT TO: <" ); 
	strSendBuf.append( m_smtpPro.strSendTo.c_str() ); 
	strSendBuf.append( ">\r\n" ); 
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(250);
}

bool SmtpSendEmail::DataServ()
{
	string strSendBuf;
	strSendBuf.append( "DATA\r\n" ); 
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(354);
}
bool SmtpSendEmail::SendData()
{
	string strSendBuf;
	strSendBuf.append( "From: <" ); 
	strSendBuf.append( m_smtpPro.strMailFrom.c_str() ); 
	strSendBuf.append( ">\r\n" );

	strSendBuf.append( "To: <" ); 
	strSendBuf.append( m_smtpPro.strSendTo.c_str() ); 
	strSendBuf.append( ">\r\n" );

	strSendBuf.append( "Date: " ); 
	strSendBuf.append( m_smtpPro.strDateTime.c_str() ); 
	strSendBuf.append( "\r\n" );

	strSendBuf.append( "Subject: " ); 
	strSendBuf.append( m_smtpPro.strSubject.c_str() ); 
	strSendBuf.append( "\r\n" );

	strSendBuf.append( MIMETextPlainLogin.c_str() );

	//内容
	strSendBuf.append(m_smtpPro.strData.c_str());
	//结束符
	strSendBuf.append("\r\n.\r\n");
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(250);
}

bool SmtpSendEmail::SendAttachedFile()
{
	string strSendBuf;
	strSendBuf.append( "From: <" ); 
	strSendBuf.append( m_smtpPro.strMailFrom.c_str() ); 
	strSendBuf.append( ">\r\n" );

	strSendBuf.append( "To: <" ); 
	strSendBuf.append( m_smtpPro.strSendTo.c_str() ); 
	strSendBuf.append( ">\r\n" );

	strSendBuf.append( "Date: " ); 
	strSendBuf.append( m_smtpPro.strDateTime.c_str() ); 
	strSendBuf.append( "\r\n" );

	strSendBuf.append( "Subject: " ); 
	strSendBuf.append( m_smtpPro.strSubject.c_str() ); 
	strSendBuf.append( "\r\n" );

	strSendBuf.append( MIMEMultipartMixedLogin.c_str() );

	strSendBuf.append( MyBoundary.c_str() ); 
	strSendBuf.append( CTTextPlainCharCodeGB2312.c_str() ); 
	strSendBuf.append( CTCodeBase64.c_str() ); 
	//内容加密
	ZBase64 zBase;
	string strBase = zBase.Encode((const BYTE*)m_smtpPro.strData.c_str(), (int)m_smtpPro.strData.size());
	strSendBuf.append(strBase);
	strSendBuf.append( "\r\n" );
	//发送
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}
	//附件
	for( list<FILEINFOSTRU>::iterator it = m_listFileInfo.begin(); it != m_listFileInfo.end(); it++ ) 
	{ 
		string strFile = ( *it ).strPath + ( *it ).strFile; 
		string strFileData; 
		// 读文件 
		ifstream InputFile; 
		InputFile.open( strFile.c_str(), ios_base::binary | ios_base::in ); 
		if( InputFile.fail() ) 
		{ 
			continue; 
		} 
		InputFile.seekg( 0, ios_base::end ); 
		int iLen = InputFile.tellg(); 
		InputFile.seekg( 0, ios_base::beg );

		strFileData.resize( iLen ); 
		InputFile.read( (char*)strFileData.c_str(), iLen );

		// 加入一个附件头 
		strSendBuf.clear(); 
		strSendBuf.append( MyBoundary.c_str() ); 
		strSendBuf.append( CTAppOctetStreamName.c_str() ); 
		strSendBuf.append( ( *it ).strFile.c_str() ); 
		strSendBuf.append( "\r\n" ); 
		strSendBuf.append( CDAttachemntFileName.c_str() ); 
		strSendBuf.append( ( *it ).strFile.c_str() ); 
		strSendBuf.append( "\r\n" ); 
		strSendBuf.append( CTCodeBase64.c_str() ); 
		//内容加密
		ZBase64 zBase;
		string strBase = zBase.Encode((const BYTE*)strFileData.c_str(), (int)strFileData.size());
		strSendBuf.append(strBase);
		strSendBuf.append( "\r\n" );
		//发送
		nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
		if (nSendLen != (int)strSendBuf.size())
		{
			closesocket(m_hSocket);
			m_hSocket = 0;
			return false;
		}
	} 
	//结束符
	strSendBuf.clear();
	strSendBuf.append("\r\n.\r\n");
	//发送
	nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(250);
}

bool SmtpSendEmail::QuitServ()
{
	string strSendBuf = "QUIT\r\n"; 
	int nSendLen = send(m_hSocket, strSendBuf.c_str(), (int)strSendBuf.size(), 0);
	if (nSendLen != (int)strSendBuf.size())
	{
		closesocket(m_hSocket);
		m_hSocket = 0;
		return false;
	}

	//回复
	return CheckResponse(221);
}
