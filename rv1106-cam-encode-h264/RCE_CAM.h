/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_CAM.h
    Project name : RV1106 Camera Encode with h264
    Module name  : Camera
    Date created : 2025年6月5日   19时17分46秒
    Author       : 
    Description  : 
*******************************************************************************/

#ifndef __RCE_CAM_H__
#define __RCE_CAM_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "RCE_types.h"
#include "RCE_defines.h"
#include "RCE_config.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块宏定义          ==") ;
CODE_SECTION("==========================") ;

/* 摄像头设备 */
#define RCE_CAM_CAMERA_ID                   (                                       0)
#define RCE_CAM_CAMERA_DEVICE               (                          "/dev/video11")
#define RCE_CAM_CAMERA_IQ_FILE_PATH         (                         "/etc/iqfiles/")

/* 图像缓存块数 */
#define RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM  (                                      4u)

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块枚举类型定义    ==") ;
CODE_SECTION("==========================") ;

/* 线程状态 */
typedef enum tagRCE_CAM_THREAD_STATUS
{
    CAM_THREAD_STATUS_RSV    = 0,   /* 保留 */
    CAM_THREAD_STATUS_RUN    = 1,   /* 线程正在运行 */
    CAM_THREAD_STATUS_END    = 2,   /* 线程结束（由main发来指令，回收资源正常结束） */
    CAM_THREAD_STATUS_EXIT   = 3    /* 线程退出（异常） */
} RCE_CAM_THREAD_STATUS_E ;

/* 启动或停止stream命令 */
typedef enum tagRCE_CAM_STREAM_CMD
{
    CAM_STREAM_COMMAND_RSV   = 0 ,  /* 保留 */
    CAM_STREAM_COMMAND_START = 1 ,  /* 启动stream */
    CAM_STREAM_COMMAND_STOP  = 2    /* 停止stream */
} RCE_CAM_STREAM_CMD_E ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块结构类型定义    ==") ;
CODE_SECTION("==========================") ;

/* 统计信息 */
typedef struct tagRCE_CAM_WORKAREA_STATISTICS
{
    UINT64 uiRunTime ;                /* 线程累计运行时间，以us为单位 */
    UINT   uiFrameCountFromCamera ;   /* 图像传感器输出的帧数（硬编码或软编码） */
    UINT   uiFrameCountToEncoder ;    /* 发送到编码器的帧数（仅软编码时） */
    FLOAT  fSoftEncodeRate ;          /* 软编码实时比率 */
    FLOAT  fTotalFPS ;                /* 累计帧率 */
    FLOAT  fCurrentFPS ;              /* 实时帧率 */
    UINT   uiTotalBPS ;               /* 累计码率，以bps为单位 */
    UINT   uiCurrentBPS ;             /* 实时码率，以bps为单位 */
    UINT64 ui64DataCountFromCamera ;  /* 图像传感器输出的数据量，以字节为单位 */
} RCE_CAM_WORKAREA_STATISTICS_S ;

/* 模块工作区 */
typedef struct tagRCE_CAM_WORKAREA
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
            INT                                 iCameraFile ;               /* 摄像头设备文件标识 */

            /* DW2 */
            RCE_CAM_THREAD_STATUS_E             enThreadStatus ;            /* 线程运行状态 */

            /* Others */
            RCE_CAM_WORKAREA_STATISTICS_S       stStatistics ;              /* 统计信息 */
        } ;
    } ;
} RCE_CAM_WORKAREA_S ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块入口            ==") ;
CODE_SECTION("==========================") ;

VOID *RCE_CAM_Thread(VOID *pvArgs) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RCE_CAM_H__ */

/******* End of file RCE_CAM.h. *******/  
