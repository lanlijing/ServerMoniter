// ServerMoniter.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CServerMoniterApp:
// �йش����ʵ�֣������ ServerMoniter.cpp
//

class CServerMoniterApp : public CWinApp
{
public:
	CServerMoniterApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CServerMoniterApp theApp;