/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_DTR.h
    Project name : Raspiberry pi 4b Camera Encode with h264
    Module name  : Data transfer
    Date created : 2025年5月5日   11时17分46秒
    Author       : 
    Description  : 
*******************************************************************************/

#ifndef __RCE_DTR_H__
#define __RCE_DTR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "RCE_types.h"
#include "RCE_defines.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块宏定义          ==") ;
CODE_SECTION("==========================") ;

/* 发送数据缓冲区大小 */
#define RCE_DTR_SEND_BUFFER_SIZE            (                           512u * 1024u)

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块枚举类型定义    ==") ;
CODE_SECTION("==========================") ;

/* 线程状态 */
typedef enum tagRCE_DTR_THREAD_STATUS
{
    DTR_THREAD_STATUS_RSV    = 0,   /* 保留 */
    DTR_THREAD_STATUS_RUN    = 1,   /* 线程正在运行 */
    DTR_THREAD_STATUS_END    = 2,   /* 线程结束（由main发来指令，回收资源正常结束） */
    DTR_THREAD_STATUS_EXIT   = 3    /* 线程退出（异常） */
} RCE_DTR_THREAD_STATUS_E ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块结构类型定义    ==") ;
CODE_SECTION("==========================") ;

/* FIFO */
typedef struct tagRCE_DTR_FIFO
{
    /* 数据部分 */
    UCHAR aucData[RCE_DTR_SEND_BUFFER_SIZE] ;

    /* 指针部分 */
    UINT  uiWrPtr ;   /* 指向下一个待写的空位置 */
    UINT  uiRdPtr ;   /* 指向下一个待读的数据位置 */
} RCE_DTR_FIFO_S ;

/* 统计信息 */
typedef struct tagRCE_DTR_WORKAREA_STATISTICS
{
    UINT64 uiRunTime ;                /* 线程累计运行时间，以us为单位 */
    UINT64 ui64DataCountFromFIFO ;    /* 从FIFO中提取的数据，以字节为单位 */
    UINT64 ui64DataCountToSocket ;    /* 发送到Socket的数据量，以字节为单位 */
    UINT   uiCurrentBPSFromFIFO ;     /* 从FIFO中提取的数据速率，以bps为单位 */
    UINT   uiCurrentBPSToSocket ;     /* 发送到Socket的数据速率，以bps为单位 */
} RCE_DTR_WORKAREA_STATISTICS_S ;

/* 模块工作区 */
typedef struct tagRCE_DTR_WORKAREA
{
    union
    {
        UCHAR       aucRawData[1024] ;

        struct
        {
            /* DW0 */ /* flags only */
            UINT                                uiStopThread       :  1 ;   /* 指示本线程结束时置位 */
            UINT                                uiUpdateStatistics :  1 ;   /* 更新统计信息 */
            UINT                                uiFlags            : 30 ;

            /* DW1 */
            INT                                 iSocket ;

            /* DW2 */
            RCE_DTR_THREAD_STATUS_E             enThreadStatus ;

            RCE_DTR_WORKAREA_STATISTICS_S       stStatistics ;
        } ;
    } ;
} RCE_DTR_WORKAREA_S ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  函数声明            ==") ;
CODE_SECTION("==========================") ;
UINT  RCE_DTR_PushFIFO(UCHAR *pucSrc, UINT uiSize) ;
VOID *RCE_DTR_Thread(VOID *pvArgs) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RCE_DTR_H__ */

/******* End of file RCE_DTR.h. *******/  
