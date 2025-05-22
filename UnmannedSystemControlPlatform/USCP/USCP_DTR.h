/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : USCP_DTR.h
    Project name : Unmanned System Control Platform
    Module name  :
    Date created : 2025年05月21日 8时45分19秒
    Author       : Ning.JianLi
    Description  : Data transfer模块头文件。该模块从网络接收H264数据，然后置入
                   FIFO，等待DEC线程取走解码。
*******************************************************************************/

#ifndef __USCP_DTR_H__
#define __USCP_DTR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "USCP_defines.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  头文件                   ==") ;
CODE_SECTION("===============================") ;

#include "USCP_types.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  模块内宏定义             ==") ;
CODE_SECTION("===============================") ;

/* 通信配置 */
#define USCP_DTR_LOCAL_IP                             (              "192.168.3.100")    /* 本地IP地址 */
#define USCP_DTR_LOCAL_PORT                           (                       12000u)    /* 本地端口号 */

/* 接收数据是否写入到文件 */
#define USCP_DTR_WRITE_TO_FILE

#ifdef USCP_DTR_WRITE_TO_FILE
#define USCP_DTR_FILE_NAME                            (                   "recv.264")    /* socket接收数据写入到本地时，本地文件名 */
#endif

CODE_SECTION("===============================") ;
CODE_SECTION("==  模块枚举数据类型         ==") ;
CODE_SECTION("===============================") ;
/* 线程状态 */
typedef enum tagUSCP_DTR_THREAD_STATUS
{
    DTR_THREAD_STATUS_NOT_START = 0, /* 线程尚未运行 */
    DTR_THREAD_STATUS_RUN       = 1, /* 线程正在运行 */
    DTR_THREAD_STATUS_EXIT      = 2, /* 线程异常退出 */
    DTR_THREAD_STATUS_END       = 3  /* 线程正常退出 */
} USCP_DTR_THREAD_STATUS_E ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  模块结构数据类型         ==") ;
CODE_SECTION("===============================") ;

/* 统计信息 */
typedef struct tagUSCP_DTR_STATISTICS
{
    UINT64 ui64TotalRecv ; /* 累计接收字节数 */
    UINT   uiCurrentBps ;  /* 当前接收速率 */
    UINT   uiFifoDepth ;   /* FIFO内含数据深度 */
} USCP_DTR_STATISTICS_S ;

/* 模块工作区 */
typedef struct tagUSCP_DTR_WORKAREA
{
    union
    {
        UCHAR aucRawDATA[1024] ;

        struct
        {
            /* DW0 */ /* 仅用作标记 */
            UINT                     uiStopThread       :  1 ;
            UINT                     uiUpdateStatistics :  1 ;
            UINT                     uiDW0Rsv           : 30 ;

            /* DW1 */ /* 线程状态 */
            USCP_DTR_THREAD_STATUS_E enThreadStatus ;

            /* Socket描述符 */
            SOCKET                   skSocket ;

            /* 统计信息 */
            USCP_DTR_STATISTICS_S    stStatistics ;

#ifdef USCP_DTR_WRITE_TO_FILE
            /* 本地文件 */
            FILE                    *pf264File ;
#endif
        } ;
    } ;
} USCP_DTR_WORKAREA_S ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  函数声明                 ==") ;
CODE_SECTION("===============================") ;

/* 线程函数 */
UINT USCP_DTR_Thread(LPVOID pvParam) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __USCP_DTR_H__ */

/******* End of file USCP_DTR.h. *******/