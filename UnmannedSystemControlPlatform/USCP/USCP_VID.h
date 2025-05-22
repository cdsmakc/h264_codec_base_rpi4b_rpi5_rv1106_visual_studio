/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : USCP_VID.h
    Project name : Unmanned System Control Platform
    Module name  :
    Date created : 2025年05月19日 13时08分26秒
    Author       : Ning.JianLi
    Description  : video模块头文件。该模块显示视频图像到界面中。
*******************************************************************************/

#ifndef __USCP_VID_H__
#define __USCP_VID_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "USCP_defines.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  头文件                   ==") ;
CODE_SECTION("===============================") ;


CODE_SECTION("===============================") ;
CODE_SECTION("==  模块内宏定义             ==") ;
CODE_SECTION("===============================") ;

/* 图像大小信息 */
#define USCP_VID_VIDEO_WIDTH                         (                             1280U)   /* 图像宽度 */
#define USCP_VID_VIDEO_HEIGHT                        (                              720U)   /* 图像高度 */
#define USCP_VID_VIDEO_PIXELS                        (USCP_VID_VIDEO_WIDTH * \
                                                                   USCP_VID_VIDEO_HEIGHT)   /* 像素总数 */
#define USCP_VID_VIDEO_BYTES_BGR                     (         USCP_VID_VIDEO_PIXELS * 3)   /* BGR24格式图像字节数 */
#define USCP_VID_VIDEO_BYTES_YUV420                  (     USCP_VID_VIDEO_PIXELS * 3 / 2)   /* YUV420格式图像字节数 */

/* 定义测试：彩条测试，或者使用YUV文件测试 */
//#define USCP_VID_TEST

#ifdef USCP_VID_TEST
/* 如果定义彩条测试，则使用彩条测试；否则使用YUV文件测试 */
//#define USCP_VID_TEST_WITH_COLOR_BAR
#ifdef USCP_VID_TEST_WITH_COLOR_BAR
#undef USCP_VID_TEST_WITH_YUV_FILE
#undef USCP_VID_TEST_INPUT_YUV_FILE_NAME
#else
#define USCP_VID_TEST_WITH_YUV_FILE
#define USCP_VID_TEST_INPUT_YUV_FILE_NAME           (             "./x64/Debug/720p.yuv")   /* 测试用文件 */
#endif
#endif

CODE_SECTION("===============================") ;
CODE_SECTION("==  模块枚举数据类型         ==") ;
CODE_SECTION("===============================") ;
/* 线程状态 */
typedef enum tagUSCP_VID_THREAD_STATUS
{
    VID_THREAD_STATUS_NOT_START = 0, /* 线程尚未运行 */
    VID_THREAD_STATUS_RUN       = 1, /* 线程正在运行 */
    VID_THREAD_STATUS_EXIT      = 2, /* 线程异常退出 */
    VID_THREAD_STATUS_END       = 3  /* 线程正常退出 */
} USCP_VID_THREAD_STATUS_E ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  模块结构数据类型         ==") ;
CODE_SECTION("===============================") ;

/* 统计信息 */
typedef struct tagUSCP_VID_STATISTICS
{
    UINT  uiTotalFrames ; /* 累计显示帧数 */
    FLOAT fCurrentFps ;  /* 当前帧率 */
} USCP_VID_STATISTICS_S ;

/* 模块工作区 */
typedef struct tagUSCP_VID_WORKAREA
{
    union
    {
        UCHAR aucRawDATA[1024] ;

        struct
        {
            /* DW0 */ /* 仅用作标记 */
            UINT                     uiStopThread       :  1 ;
            UINT                     uiNewYuvFrame      :  1 ;
            UINT                     uiUpdateStatistics :  1 ;
            UINT                     uiDW0Rsv           : 29 ;

            /* DW1 */ /* 线程状态 */
            USCP_VID_THREAD_STATUS_E enThreadStatus ;

            /* YUV图像 */
            UCHAR*pucYuvFrame ;

            /* 位图信息 */
            UCHAR                   *pucBitmapFile ;
            UCHAR                   *pucBitmapData ;
#ifdef USCP_VID_TEST_WITH_YUV_FILE
            /* 测试文件 */
            FILE                    *pfYuvFile ;
            UINT                     uiTotalFrame ;
            UINT                     uiCurrentFrame ;
#endif
            USCP_VID_STATISTICS_S    stStatistics ;
        } ;
    } ;
} USCP_VID_WORKAREA_S ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  函数声明                 ==") ;
CODE_SECTION("===============================") ;

/* 线程函数 */
UINT USCP_VID_Thread(LPVOID pvParam) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __USCP_VID_H__ */

/******* End of file USCP_VID.h. *******/