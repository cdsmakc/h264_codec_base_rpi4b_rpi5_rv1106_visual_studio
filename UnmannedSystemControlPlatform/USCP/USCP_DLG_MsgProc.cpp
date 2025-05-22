/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
	File name    : USCP_DLG_MsgProc.cpp
	Project name : Unmanned System Control Platform
	Module name  :
	Date created : 2025年05月21日 8时45分19秒
	Author       : Ning.JianLi
	Description  : 对话框的自定义消息处理函数。
*******************************************************************************/

#include "pch.h"
#include "USCP_defines.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  头文件                   ==") ;
CODE_SECTION("===============================") ;
/* 标准库 */

/* 第三方 */
#include <gdiplus.h>

/* 项目内部 */
#include "USCP.h"
#include "USCP_Msg.h"
#include "USCP_config.h"

#include "fifo.h"

#include "USCP_DLG.h"
#include "USCP_VID.h"
#include "USCP_DTR.h"
#include "USCP_DEC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CODE_SECTION("===============================") ;
CODE_SECTION("==  全局变量                 ==") ;
CODE_SECTION("===============================") ;

/* 线程工作区 */
extern USCP_VID_WORKAREA_S g_stVidWorkarea ;
extern USCP_DEC_WORKAREA_S g_stDecWorkarea ;
extern USCP_DTR_WORKAREA_S g_stDtrWorkarea ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  消息映射                 ==") ;
CODE_SECTION("===============================") ;

BEGIN_MESSAGE_MAP(CUSCP_DLG, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	/* 自定义消息映射 */
	ON_WM_TIMER()                                                      /* 定时器消息        */
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CUSCP_DLG::OnBnClickedButtonExit)  /* “退出按钮”消息    */
	ON_MESSAGE(USCP_WIN_MSG_USER_VID_UPDATE_VIDEO, OnProcessVIDMsg)    /* VID线程发来的消息 */
END_MESSAGE_MAP()

CODE_SECTION("===============================") ;
CODE_SECTION("==  消息处理函数             ==") ;
CODE_SECTION("===============================") ;
/*******************************************************************************
- Function    : OnTimer
- Description : 本函数响应定时器消息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
/*
 *  在窗口显示Dtr接收数据总量信息
 */
VOID __ONTIMER_DisplayDtrRecvCount(wchar_t *pwcBuf, UINT uiBufSize)
{
	/* 显示DTR接收字节数 */
	if (1024 * 1024 * 1024 <= g_stDtrWorkarea.stStatistics.ui64TotalRecv)
	{
		/* 以GB为单位显示 */
		swprintf(pwcBuf, uiBufSize, L"DTR 接收总量 ：%3.3f GB", (DOUBLE)g_stDtrWorkarea.stStatistics.ui64TotalRecv / (DOUBLE)(1024 * 1024 * 1024)) ;
	}
	else if (1024 * 1024 <= g_stDtrWorkarea.stStatistics.ui64TotalRecv)
	{
		/* 以MB为单位显示 */
		swprintf(pwcBuf, uiBufSize, L"DTR 接收总量 ：%3.3f MB", (DOUBLE)g_stDtrWorkarea.stStatistics.ui64TotalRecv / (DOUBLE)(1024 * 1024)) ;
	}
	else if (1024 <= g_stDtrWorkarea.stStatistics.ui64TotalRecv)
	{
		/* 以KB为单位显示 */
		swprintf(pwcBuf, uiBufSize, L"DTR 接收总量 ：%3.3f KB", (DOUBLE)g_stDtrWorkarea.stStatistics.ui64TotalRecv / (DOUBLE)(1024)) ;
	}
	else
	{
		/* 以B为单位显示 */
		swprintf(pwcBuf, uiBufSize, L"DTR 接收总量 ：%llu B", g_stDtrWorkarea.stStatistics.ui64TotalRecv) ;
	}

	return ;
}

/*
 *  在窗口显示Dtr接收数据速率信息
 */
VOID __ONTIMER_DisplayDtrRecvBps(wchar_t* pwcBuf, UINT uiBufSize)
{
	/* 显示DTR接收速率 */
	if (1000 * 1000 <= g_stDtrWorkarea.stStatistics.uiCurrentBps)
	{
		/* 以Mbps为单位显示 */
		swprintf(pwcBuf, uiBufSize, L"DTR 接收速率 ：%3.3f Mbps", (DOUBLE)g_stDtrWorkarea.stStatistics.uiCurrentBps / (DOUBLE)(1000 * 1000)) ;
	}
	else if (1000 <= g_stDtrWorkarea.stStatistics.uiCurrentBps)
	{
		/* 以Kbps为单位显示 */
		swprintf(pwcBuf, uiBufSize, L"DTR 接收速率 ：%3.3f Kbps", (DOUBLE)g_stDtrWorkarea.stStatistics.uiCurrentBps / (DOUBLE)(1000)) ;
	}
	else
	{
		/* 以bps为单位显示 */
		swprintf(pwcBuf, uiBufSize, L"DTR 接收速率 ：%u bps", g_stDtrWorkarea.stStatistics.uiCurrentBps) ;
	}

	return ;
}

/*
 *  在窗口显示Dtr至DEC的FIFO深度
 */
VOID __ONTIMER_DisplayDtr2DecFifoDepth(wchar_t* pwcBuf, UINT uiBufSize)
{
	/* 显示FIFO深度 */
	swprintf(pwcBuf, uiBufSize, L"DTR FIFO深度 ：%u B", g_stDtrWorkarea.stStatistics.uiFifoDepth) ;

	return ;
}

/*
 *  在窗口显示DEC解码帧率
 */
VOID __ONTIMER_DisplayDecFps(wchar_t* pwcBuf, UINT uiBufSize)
{
	/* 显示FIFO深度 */
	swprintf(pwcBuf, uiBufSize, L"DEC 显示帧率 ：%2.3f fps", g_stDecWorkarea.stStatistics.fCurrentFps) ;

	return ;
}

/*
 *  在窗口显示VID显示帧率
 */
VOID __ONTIMER_DisplayVidFps(wchar_t* pwcBuf, UINT uiBufSize)
{
	/* 显示FIFO深度 */
	swprintf(pwcBuf, uiBufSize, L"VID 显示帧率 ：%2.3f fps", g_stVidWorkarea.stStatistics.fCurrentFps) ;

	return ;
}

VOID CUSCP_DLG::OnTimer(UINT_PTR nIDEvent)
{
	wchar_t awcPrintBuf[100] ;

	/* 在窗口显示Dtr接收数据总量信息 */
	__ONTIMER_DisplayDtrRecvCount(awcPrintBuf, sizeof(awcPrintBuf) / sizeof(awcPrintBuf[0])) ;

	this->GetDlgItem(IDC_STATIC_DTR_RECV_COUNT)->SetWindowText(awcPrintBuf) ;

	/* 在窗口显示Dtr接收数据速率信息 */
	__ONTIMER_DisplayDtrRecvBps(awcPrintBuf, sizeof(awcPrintBuf) / sizeof(awcPrintBuf[0])) ;

	this->GetDlgItem(IDC_STATIC_DTR_RECV_BPS)->SetWindowText(awcPrintBuf) ;

	/* 在窗口显示Dtr接收数据速率信息 */
	__ONTIMER_DisplayDtr2DecFifoDepth(awcPrintBuf, sizeof(awcPrintBuf) / sizeof(awcPrintBuf[0])) ;

	this->GetDlgItem(IDC_STATIC_DTR_FIFO_USAGE)->SetWindowText(awcPrintBuf) ;

	/* 在窗口显示Dec显示帧率 */
	__ONTIMER_DisplayDecFps(awcPrintBuf, sizeof(awcPrintBuf) / sizeof(awcPrintBuf[0])) ;

	this->GetDlgItem(IDC_STATIC_DEC_FPS)->SetWindowText(awcPrintBuf) ;

	/* 在窗口显示Vid显示帧率 */
	__ONTIMER_DisplayVidFps(awcPrintBuf, sizeof(awcPrintBuf) / sizeof(awcPrintBuf[0])) ;

	this->GetDlgItem(IDC_STATIC_VID_FPS)->SetWindowText(awcPrintBuf) ;

	/* 通知其他模块更新统计信息 */
	g_stDtrWorkarea.uiUpdateStatistics = 1 ;
	g_stDecWorkarea.uiUpdateStatistics = 1 ;
	g_stVidWorkarea.uiUpdateStatistics = 1 ;

	return ;
}

/*******************************************************************************
- Function    : OnProcessVIDMsg
- Description : 本函数处理来自VID线程的消息。
- Input       : VOID
- Output      : NULL
- Return      : BOOL
- Others      :
*******************************************************************************/
inline VOID RenderBitmap(CWnd* pWnd, Gdiplus::Bitmap* pbmp)
{
	RECT rect;
	Gdiplus::Status status ;
	pWnd->GetClientRect(&rect);

	Gdiplus::Graphics grf(pWnd->m_hWnd);
	status = grf.GetLastStatus();
	if (status == Gdiplus::Ok)
	{
		Gdiplus::Rect rc(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);

		grf.DrawImage(pbmp, rc);
	}
}

afx_msg LRESULT CUSCP_DLG::OnProcessVIDMsg(WPARAM w, LPARAM l)
{
	CWnd          *pWnd     = GetDlgItem(IDC_VIDEO) ;
	IStream       *pPicture = NULL ;
	LARGE_INTEGER  liTemp   = { 0 } ;

	CreateStreamOnHGlobal(NULL, TRUE, &pPicture) ;

	if (pPicture != NULL)
	{
		pPicture->Write(g_stVidWorkarea.pucBitmapFile, USCP_VID_VIDEO_BYTES_BGR + 54, NULL) ;
		pPicture->Seek(liTemp, STREAM_SEEK_SET, NULL) ;
		Gdiplus::Bitmap TempBmp(pPicture) ;
		RenderBitmap(pWnd, &TempBmp) ;
	}
	if (pPicture != NULL)
	{
		pPicture->Release();
		pPicture = NULL;
	}

	return 0;
}

/*******************************************************************************
- Function    : OnBnClickedButtonExit
- Description : 本函数处理退出按钮按下时的消息。
- Input       : VOID
- Output      : NULL
- Return      : BOOL
- Others      :
*******************************************************************************/
VOID CUSCP_DLG::OnBnClickedButtonExit()
{
	/* 退出三个线程 */
	g_stDtrWorkarea.uiStopThread = 1 ;
	g_stDecWorkarea.uiStopThread = 1 ;
	g_stVidWorkarea.uiStopThread = 1 ;

	/* 等待Dtr线程结束 */
	while(DTR_THREAD_STATUS_END != g_stDtrWorkarea.enThreadStatus && DTR_THREAD_STATUS_EXIT != g_stDtrWorkarea.enThreadStatus)

	/* 等待Dec线程结束 */
	while (DEC_THREAD_STATUS_END != g_stDecWorkarea.enThreadStatus && DEC_THREAD_STATUS_EXIT != g_stDecWorkarea.enThreadStatus)

	/* 等待Vid线程结束 */
	while (VID_THREAD_STATUS_END != g_stVidWorkarea.enThreadStatus && VID_THREAD_STATUS_EXIT != g_stVidWorkarea.enThreadStatus)


	CDialogEx::OnOK() ;

	return ;
}
