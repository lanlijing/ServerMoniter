// ServerMoniterDlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

class CExeInfoMgr;
// CServerMoniterDlg �Ի���
class CServerMoniterDlg : public CDialog
{
// ����
public:
	CServerMoniterDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_SERVERMONITER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;
	
	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnErrMsg(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnMoniterStart(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnExeStatus(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_List1;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CEdit m_edtConfig;
	CEdit m_edtMsg;
	afx_msg void OnBnClickedBtloadconfig();
	afx_msg void OnBnClickedBtstart();
	afx_msg void OnBnClickedBtend();
	virtual BOOL DestroyWindow();
	afx_msg void OnBnClickedBtendtimer();

private:
	CExeInfoMgr* m_pExeInfoMgr;
};

