/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_ENC.h
    Project name : Raspiberry pi 4b Camera Encode with h264
    Module name  : Encoder
    Date created : 2025年5月1日   11时36分50秒
    Author       : 
    Description  : 
*******************************************************************************/

#ifndef __RCE_ENC_H__
#define __RCE_ENC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "RCE_types.h"


CODE_SECTION("==========================") ;
CODE_SECTION("==  模块宏定义          ==") ;
CODE_SECTION("==========================") ;

/* 编码器性能 */
#define RCE_ENC_CAMERA_ENCODER_PRESET       (                               "veryfast") 
    /* "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo" */
#define RCE_ENC_CAMERA_ENCODER_PROFILE      (                               "baseline") 
    /* "baseline", "main", "high", "high10", "high422", "high444" */

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块枚举类型定义    ==") ;
CODE_SECTION("==========================") ;

/* 线程状态 */
typedef enum tagRCE_ENC_THREAD_STATUS
{
    ENC_THREAD_STATUS_RSV    = 0,   /* 保留 */
    ENC_THREAD_STATUS_RUN    = 1,   /* 线程正在运行 */
    ENC_THREAD_STATUS_END    = 2,   /* 线程结束（由main发来指令，回收资源正常结束） */
    ENC_THREAD_STATUS_EXIT   = 3    /* 线程退出（异常） */
} RCE_ENC_THREAD_STATUS_E ;


CODE_SECTION("==========================") ;
CODE_SECTION("==  模块结构类型定义    ==") ;
CODE_SECTION("==========================") ;

/* 统计信息 */
typedef struct tagRCE_ENC_WORKAREA_STATISTICS
{
    UINT64 uiRunTime ;                /* 线程累计运行时间，以us为单位 */
    UINT64 ui64DataCountEncoded ;     /* 累计被编码数据量，以字节为单位 */
    UINT   uiFrameCountEncoded ;      /* 累计被编码帧数量，以字节为单位 */
    UINT64 ui64DataCountGenerated ;   /* 累计输出数据量，以字节为单位 */
    FLOAT  fTotalEncodeFPS ;          /* 累计编码帧率，以fps为单位 */
    FLOAT  fCurrentEncodeFPS ;        /* 当前编码帧率，以fps为单位 */
    UINT   uiTotalEncodeBPS ;         /* 累计编码码率，以bps为单位 */
    UINT   uiCurrentEncodeBPS ;       /* 当前编码码率，以bps为单位 */
} RCE_ENC_WORKAREA_STATISTICS_S ;

/* 模块工作区 */
typedef struct tagRCE_ENC_WORKAREA
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
            INT                                 iStoreFile ;

            /* DW2 */
            RCE_ENC_THREAD_STATUS_E             enThreadStatus ;

            RCE_ENC_WORKAREA_STATISTICS_S       stStatistics ;
        } ;
    } ;
} RCE_ENC_WORKAREA_S ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块入口            ==") ;
CODE_SECTION("==========================") ;

VOID *RCE_ENC_Thread(VOID *pvArgs) ;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RCE_ENC_H__ */

/******* End of file RCE_ENC.h. *******/  
