﻿
#pragma once


class CUSCP_DLG : public CDialogEx
{
// 构造
public:
	CUSCP_DLG(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_USCP_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	afx_msg LRESULT OnProcessVIDMsg(WPARAM w, LPARAM l) ;
private :
	CStatic     m_Video ;
	UINT_PTR    m_TimerID ;

private :
	CWinThread* m_pobjDTRThread ; /* DTR线程 */
	CWinThread* m_pobjDECThread ; /* DEC线程 */
	CWinThread* m_pobjVIDThread ; /* VID线程 */
	
public:
	afx_msg VOID OnTimer(UINT_PTR nIDEvent);
	afx_msg VOID OnBnClickedButtonExit();

};
