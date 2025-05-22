

#include "pch.h"
#include "USCP_defines.h"


//#pragma comment(lib, "gdiplus.lib")
#include <gdiplus.h>
using namespace Gdiplus;

#include "framework.h"
#include "USCP.h"
#include "USCP_DLG.h"
#include "afxdialogex.h"
#include "windows.h"
#include "GdiplusHeaders.h"

#include "fifo.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  头文件                   ==") ;
CODE_SECTION("===============================") ;


/* 项目内部 */
#include "USCP_Msg.h"
#include "USCP_config.h"
#include "USCP_VID.h"
#include "USCP_DEC.h"
#include "USCP_DTR.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
CODE_SECTION("===============================") ;
CODE_SECTION("==  全局变量                 ==") ;
CODE_SECTION("===============================") ;

/* V线程工作区 */
extern USCP_VID_WORKAREA_S g_stVidWorkarea ;
extern USCP_DTR_WORKAREA_S g_stDtrWorkarea ;

/* FIFO */
FIFO_DEVICE g_stDtr2DecFifo ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  构造函数                 ==") ;
CODE_SECTION("===============================") ;

CUSCP_DLG::CUSCP_DLG(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_USCP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


void CUSCP_DLG::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VIDEO, m_Video);
}



CODE_SECTION("===============================") ;
CODE_SECTION("==  消息处理函数             ==") ;
CODE_SECTION("===============================") ;
/*******************************************************************************
- Function    : OnInitDialog
- Description : 本函数初始化对话框。
- Input       : VOID
- Output      : NULL
- Return      : BOOL
- Others      :
*******************************************************************************/
BOOL CUSCP_DLG::OnInitDialog()
{
	ULONG_PTR m_gdiplusToken;
	CFont     objFont ;

	CDialogEx::OnInitDialog();

	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);


	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	/* 创建全局变量 */
	/* 创建DTR线程向DEC线程发送数据的FIFO */
	memset(&g_stDtr2DecFifo, 0, sizeof(g_stDtr2DecFifo)) ;
	FIFO_Create(1048576, &g_stDtr2DecFifo) ;

	/* 创建线程 */
	m_pobjDTRThread = AfxBeginThread((AFX_THREADPROC)USCP_DTR_Thread, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL) ;
	m_pobjDECThread = AfxBeginThread((AFX_THREADPROC)USCP_DEC_Thread, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL) ;
	m_pobjVIDThread = AfxBeginThread((AFX_THREADPROC)USCP_VID_Thread, this, THREAD_PRIORITY_NORMAL, 0, 0, NULL) ;

	/* 创建定时器 */ /* 每250ms触发一次 */
	m_TimerID = SetTimer(USCP_TIMER_ID, USCP_TIMER_ELAPSE, NULL) ;

	return TRUE ;
}

void CUSCP_DLG::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CUSCP_DLG::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}







