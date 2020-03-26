// ServerMoniterDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

class CExeInfoMgr;
// CServerMoniterDlg 对话框
class CServerMoniterDlg : public CDialog
{
// 构造
public:
	CServerMoniterDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_SERVERMONITER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	
	// 生成的消息映射函数
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

