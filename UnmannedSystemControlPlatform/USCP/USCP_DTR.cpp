/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : USCP_DTR.cpp
    Project name : Unmanned System Control Platform
    Module name  :
    Date created : 2025年05月21日 8时45分19秒
    Author       : Ning.JianLi
    Description  : Data transfer模块源文件。该模块从网络接收H264数据，然后置入
                   FIFO，等待DEC线程取走解码。
*******************************************************************************/

#include "pch.h"

//#ifdef __cplusplus
//extern "C" {
//#endif 

#include "USCP_defines.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  头文件                   ==") ;
CODE_SECTION("===============================") ;
/* 标准库 */
#include <stdio.h>

/* 第三方头文件 */
#include <afxwin.h>
#include <minwindef.h>
#include <WinSock2.h>

/* 项目内部头文件 */
#include "USCP_types.h"
#include "USCP_Msg.h"
#include "USCP_config.h"
#include "fifo.h"

#include "USCP_DTR.h"


CODE_SECTION("===============================") ;
CODE_SECTION("==  全局变量                 ==") ;
CODE_SECTION("===============================") ;

/* 模块工作区 */
USCP_DTR_WORKAREA_S   g_stDtrWorkarea ;

/* 连接DTR和DEC线程的FIFO */
extern FIFO_DEVICE    g_stDtr2DecFifo ;


CODE_SECTION("===============================") ;
CODE_SECTION("==  线程内部函数             ==") ;
CODE_SECTION("===============================") ;

/*******************************************************************************
- Function    : __USCP_DTR_StartupWinSocketAPI
- Description : 本函数初始化Windows socket API。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __USCP_DTR_StartupWinSocketAPI(VOID)
{
    WSADATA stWsaData ;
    INT     iRetVal ;

    /* 初始化WSA */
    iRetVal = WSAStartup(MAKEWORD(2, 2), &stWsaData) ;

    if (0 != iRetVal)
    {
        AfxMessageBox(_T("WSAStartup失败"), MB_OK) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DTR_StopWinSocketAPI
- Description : 本函数禁用Windows socket API。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DTR_StopWinSocketAPI(VOID)
{
    WSACleanup() ;
}


/*******************************************************************************
- Function    : __USCP_DTR_OpenSocket
- Description : 本函数打开一个socket。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __USCP_DTR_OpenSocket(VOID)
{
    g_stDtrWorkarea.skSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ;

    if (INVALID_SOCKET == g_stDtrWorkarea.skSocket)
    {
        AfxMessageBox(_T("创建socket失败"), MB_OK) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DTR_CloseSocket
- Description : 本函数关闭socket。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DTR_CloseSocket(VOID)
{
    closesocket(g_stDtrWorkarea.skSocket) ;

    return ;
}

/*******************************************************************************
- Function    : __USCP_DTR_BindLocal
- Description : 本函数绑定本地地址和端口到socekt。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __USCP_DTR_BindLocal(VOID)
{
    UINT                uiIPAddr ;
    struct sockaddr_in  stLocalSockAddr ;
    int                 iRetVal = 0 ;

    uiIPAddr                        = inet_addr(USCP_DTR_LOCAL_IP) ;
    stLocalSockAddr.sin_family      = AF_INET ;
    stLocalSockAddr.sin_addr.s_addr = uiIPAddr ;
    stLocalSockAddr.sin_port        = htons(USCP_DTR_LOCAL_PORT) ;

    iRetVal = bind(g_stDtrWorkarea.skSocket, (struct sockaddr*)&stLocalSockAddr, sizeof(stLocalSockAddr)) ;

    if (SOCKET_ERROR == iRetVal)
    {
        AfxMessageBox(_T("绑定当前端口失败"), MB_OK) ;

        return -1 ;
    }

    return 0 ;
}

#ifdef USCP_DTR_WRITE_TO_FILE
/*******************************************************************************
- Function    : __USCP_DTR_OpenFile
- Description : 本函数打开本地保存文件。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0 : 文件打开成功。
                   -1 : 文件打开失败。
- Others      :
*******************************************************************************/
INT __USCP_DTR_OpenFile(VOID)
{
    g_stDtrWorkarea.pf264File = fopen(USCP_DTR_FILE_NAME, "wb") ;

    if (NULL == g_stDtrWorkarea.pf264File)
    {
        AfxMessageBox(_T("DTR : 打开测试文件失败"), MB_OK) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DTR_CloseFile
- Description : 本函数关闭本地保存文件。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DTR_CloseFile(VOID)
{
    if (NULL != g_stDtrWorkarea.pf264File)
    {
        fflush(g_stDtrWorkarea.pf264File) ;
        fclose(g_stDtrWorkarea.pf264File) ;
    }

    return ;
}
#endif

/*******************************************************************************
- Function    : __USCP_DTR_UpdateStatistics
- Description : 本函数更新统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DTR_UpdateStatistics(VOID)
{
    STATIC UINT64 ui64PrevTotalRecv = 0 ;

    /* 计算从socket接收数据的当前BPS */
    g_stDtrWorkarea.stStatistics.uiCurrentBps = (UINT)((FLOAT)((g_stDtrWorkarea.stStatistics.ui64TotalRecv - ui64PrevTotalRecv) * 8) /
        ((FLOAT)USCP_TIMER_ELAPSE / 1000.0f)) ;

    /* 计算当前FIFO深度 */
    g_stDtrWorkarea.stStatistics.uiFifoDepth = FIFO_GetFifoDataCount(&g_stDtr2DecFifo) ;

    ui64PrevTotalRecv = g_stDtrWorkarea.stStatistics.ui64TotalRecv ;

    return ;
}

CODE_SECTION("===============================") ;
CODE_SECTION("==  线程入口函数             ==") ;
CODE_SECTION("===============================") ;
/*******************************************************************************
- Function    : USCP_DTR_Thread
- Description : VID线程入口函数。
- Input       : pvParam :
                    Point to father dlg.
- Output      : null
- Return      : UINT
- Others      :
*******************************************************************************/
UINT USCP_DTR_Thread(LPVOID pvParam)
{
    INT                iRetVal ;
    CHAR               acRecvBuf[4096] ;
    struct sockaddr_in stClientAddr ;
    INT                iClientAddrSize = sizeof(stClientAddr) ;

    /* 清空工作区 */
    memset(&g_stDtrWorkarea, 0, sizeof(g_stDtrWorkarea)) ;

    /* 设置线程运行状态 */
    g_stDtrWorkarea.enThreadStatus = DTR_THREAD_STATUS_RUN ;

    /* 使能Windows socket API */
    iRetVal = __USCP_DTR_StartupWinSocketAPI() ;

    if (0 != iRetVal)
    {
        g_stDtrWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_EXIT_WITH_NO_PROCESS ;
    }

    /* 创建socket */
    iRetVal = __USCP_DTR_OpenSocket() ;

    if (0 != iRetVal)
    {
        g_stDtrWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_EXIT_WITH_STOP_WIN_SOCKET_API ;
    }

    /* 绑定本地端口 */
    iRetVal = __USCP_DTR_BindLocal() ;

    if (0 != iRetVal)
    {
        g_stDtrWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_EXIT_WITH_CLOSE_SOCKET ;
    }

#ifdef USCP_DTR_WRITE_TO_FILE
    iRetVal = __USCP_DTR_OpenFile() ;

    if (0 != iRetVal)
    {
        g_stDtrWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_EXIT_WITH_CLOSE_SOCKET ;
    }
#endif

    while (1)
    {
        /* 从socket获取数据 */
        iRetVal = recvfrom(g_stDtrWorkarea.skSocket, acRecvBuf, sizeof(acRecvBuf), 0, (sockaddr*)&stClientAddr, &iClientAddrSize) ;

        if (SOCKET_ERROR == iRetVal)
        {
            g_stDtrWorkarea.enThreadStatus = DTR_THREAD_STATUS_EXIT ;
#ifdef USCP_DTR_WRITE_TO_FILE
            goto LABEL_THREAD_EXIT_WITH_CLOSE_FILE ;
#else
            goto LABEL_THREAD_EXIT_WITH_CLOSE_SOCKET ;
#endif
        }
        else if(0 < iRetVal)
        {
            FIFO_Put((UCHAR *)acRecvBuf, iRetVal, &g_stDtr2DecFifo) ;

            /* 统计接收的字节数 */
            g_stDtrWorkarea.stStatistics.ui64TotalRecv += iRetVal ;

#ifdef USCP_DTR_WRITE_TO_FILE
            /* 接收到的数据写入到本地文件 */
            fwrite(acRecvBuf, 1, iRetVal, g_stDtrWorkarea.pf264File) ;
            fflush(g_stDtrWorkarea.pf264File) ;
#endif
        }

        if (1 == g_stDtrWorkarea.uiUpdateStatistics)
        {
            /* 更新统计信息 */
            g_stDtrWorkarea.uiUpdateStatistics = 0 ;

            __USCP_DTR_UpdateStatistics() ;
        }

        if (1 == g_stDtrWorkarea.uiStopThread)
        {
            /* 正常退出线程 */
            g_stDtrWorkarea.enThreadStatus = DTR_THREAD_STATUS_END ;
#ifdef USCP_DTR_WRITE_TO_FILE
            goto LABEL_THREAD_EXIT_WITH_CLOSE_FILE ;
#else
            goto LABEL_THREAD_EXIT_WITH_CLOSE_SOCKET ;
#endif
        }
    }

#ifdef USCP_DTR_WRITE_TO_FILE
LABEL_THREAD_EXIT_WITH_CLOSE_FILE :
    __USCP_DTR_CloseFile() ;
#endif
LABEL_THREAD_EXIT_WITH_CLOSE_SOCKET :
    __USCP_DTR_CloseSocket() ;
LABEL_THREAD_EXIT_WITH_STOP_WIN_SOCKET_API :
    __USCP_DTR_StopWinSocketAPI() ;
LABEL_THREAD_EXIT_WITH_NO_PROCESS :
    return 0 ;
}

//#ifdef __cplusplus
//}
//#endif /* __cplusplus */

/******* End of file USCP_DTR.cpp. *******/