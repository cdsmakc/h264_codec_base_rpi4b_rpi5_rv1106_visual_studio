﻿/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : USCP_VID.cpp
    Project name : Unmanned System Control Platform
    Module name  :
    Date created : 2025年05月19日 13时08分26秒
    Author       : Ning.JianLi
    Description  : video模块源文件。该模块显示视频图像到界面中。
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
#include <opencv2/gapi.hpp>
#include <opencv2/gapi/core.hpp>
#include <opencv2/gapi/imgproc.hpp>
#include <opencv2/opencv.hpp>

/* 项目内部头文件 */
#include "USCP_types.h"
#include "USCP_Msg.h"
#include "USCP_config.h"
#include "USCP_DLG.h"
#include "USCP_VID.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  全局变量                 ==") ;
CODE_SECTION("===============================") ;

/* 模块工作区 */
USCP_VID_WORKAREA_S     g_stVidWorkarea ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  线程内部函数             ==") ;
CODE_SECTION("===============================") ;

#ifdef USCP_VID_TEST_WITH_COLOR_BAR
/*******************************************************************************
- Function    : __USCP_VID_TEST_DrawColorBarInMemory
- Description : 本函数在内存中创建一个彩条图形。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_VID_TEST_DrawColorBarInMemory(VOID)
{
    UINT        uiLoopRow, uiLoopCol, uiBarCount = 0 ;
    STATIC UINT uiSwap = 0 ;

    if(0 == uiSwap)
    {
        /* 制作Frame1 */ /* R-G-B */
        for (uiLoopRow = 0 ; uiLoopRow < USCP_VID_VIDEO_HEIGHT ; uiLoopRow++)
        {
            for (uiLoopCol = 0, uiBarCount = 0 ; uiLoopCol < USCP_VID_VIDEO_WIDTH ; uiLoopCol++)
            {
                if (0 == (uiBarCount % 3))
                {
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 0] = 0xc0 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 1] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 2] = 0x00 ;
                }
                else if (1 == (uiBarCount % 3))
                {
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 0] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 1] = 0xc0 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 2] = 0x00 ;
                }
                else if (2 == (uiBarCount % 3))
                {
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 0] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 1] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 2] = 0xc0 ;
                }

                if ((uiLoopCol + 1) == (uiBarCount + 1) * (USCP_VID_VIDEO_WIDTH / 10))
                {
                    /* 到达当前彩条边界，切换到下一个彩条 */
                    uiBarCount++ ;
                }
            }
        }

        uiSwap = 1 ;
    }
    else
    {
        /* 制作Frame2 */ /* B-G-R */
        for (uiLoopRow = 0 ; uiLoopRow < USCP_VID_VIDEO_HEIGHT ; uiLoopRow++)
        {
            for (uiLoopCol = 0, uiBarCount = 0 ; uiLoopCol < USCP_VID_VIDEO_WIDTH ; uiLoopCol++)
            {
                if (0 == (uiBarCount % 3))
                {
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 0] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 1] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 2] = 0xc0 ;
                }
                else if (1 == (uiBarCount % 3))
                {
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 0] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 1] = 0xc0 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 2] = 0x00 ;
                }
                else if (2 == (uiBarCount % 3))
                {
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 0] = 0xc0 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 1] = 0x00 ;
                    g_stVidWorkarea.pucBitmapData[(uiLoopRow * 1280 + uiLoopCol) * 3 + 2] = 0x00 ;
                }

                if ((uiLoopCol + 1) == (uiBarCount + 1) * (USCP_VID_VIDEO_WIDTH / 10))
                {
                    /* 到达当前彩条边界，切换到下一个彩条 */
                    uiBarCount++ ;
                }
            }
        }

        uiSwap = 0 ;
    }

    return ;
}
#endif

#ifdef USCP_VID_TEST_WITH_YUV_FILE
/*******************************************************************************
- Function    : __USCP_VID_TEST_OpenTestFile
- Description : 本函数打开测试文件。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0 : 正常。
                   -1 : 错误。
- Others      :
*******************************************************************************/
INT __USCP_VID_TEST_OpenTestFile(VOID)
{
    INT iRetVal ;
    /* 打开YUV文件 */
    g_stVidWorkarea.pfYuvFile = fopen(USCP_VID_TEST_INPUT_YUV_FILE_NAME, "rb") ;

    if (NULL == g_stVidWorkarea.pfYuvFile)
    {
        AfxMessageBox(_T("VID : 打开测试文件失败"), MB_OK) ;
        return -1 ;
    }

    /* 计算文件大小，从而得出文件中的YUV帧数 */
    if (0 != fseek(g_stVidWorkarea.pfYuvFile, 0, SEEK_END))
    {
        AfxMessageBox(_T("VID : 定位至文件尾失败"), MB_OK) ;
        fclose(g_stVidWorkarea.pfYuvFile) ;
        return -1 ;
    }

    iRetVal = ftell(g_stVidWorkarea.pfYuvFile) ;

    if (iRetVal < 0)
    {
        AfxMessageBox(_T("VID : 获取文件大小失败"), MB_OK) ;
        fclose(g_stVidWorkarea.pfYuvFile) ;
        return -1 ;
    }
    else
    {
        g_stVidWorkarea.uiTotalFrame = iRetVal / USCP_VID_VIDEO_BYTES_YUV420 ;
    }

    /* 重新定位到文件头 */
    fseek(g_stVidWorkarea.pfYuvFile, 0, SEEK_SET) ;

    g_stVidWorkarea.uiCurrentFrame = 0 ;

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_VID_TEST_ReadYuvData
- Description : 本函数从测试文件中读取一帧数据到内存。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_VID_TEST_ReadYuvData(VOID)
{
    if (g_stVidWorkarea.uiCurrentFrame >= (g_stVidWorkarea.uiTotalFrame - 1))
    {
        /* 从头开始读取 */
        fseek(g_stVidWorkarea.pfYuvFile, 0, SEEK_SET) ;
        g_stVidWorkarea.uiCurrentFrame = 0 ;
    }

    /* 读取图像数据 */
    fread(g_stVidWorkarea.pucYuvFrame, USCP_VID_VIDEO_BYTES_YUV420, 1, g_stVidWorkarea.pfYuvFile) ;
    g_stVidWorkarea.uiCurrentFrame++ ;

    return ;
}


/*******************************************************************************
- Function    : __USCP_VID_TEST_CloseTestFile
- Description : 本函数关闭测试文件。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_VID_TEST_CloseTestFile(VOID)
{
    if (NULL != g_stVidWorkarea.pfYuvFile)
    {
        fclose(g_stVidWorkarea.pfYuvFile) ;
    }

    return ;
}
#endif

/*******************************************************************************
- Function    : __USCP_VID_InitWorkarea
- Description : 本函数初始化工作区。
- Input       : VOID
- Output      : NULL
- Return      : INT :
                    0 : 创建成功。
                   -1 : 申请YUV图像缓存失败。
- Others      :
*******************************************************************************/
INT __USCP_VID_InitWorkarea(VOID)
{
    /* 工作区初始化为0 */
    memset(&g_stVidWorkarea, 0, sizeof(g_stVidWorkarea)) ;

    /* 设置线程状态 */
    g_stVidWorkarea.enThreadStatus = VID_THREAD_STATUS_RUN ;

    /* 根据图像分辨率申请图片缓存 */
    g_stVidWorkarea.pucYuvFrame = (UCHAR *)malloc(USCP_VID_VIDEO_BYTES_YUV420) ;

    if (NULL == g_stVidWorkarea.pucYuvFrame)
    {
        return -1 ;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __USCP_VID_DistroyWorkarea
- Description : 本函数释放工作区。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_VID_DistroyWorkarea(VOID)
{
    /* 释放YUV图像缓存 */
    if (NULL != g_stVidWorkarea.pucYuvFrame)
    {
        free(g_stVidWorkarea.pucYuvFrame) ;
    }

    return ;
}


/*******************************************************************************
- Function    : __USCP_VID_CreateBitmapFile
- Description : 本函数在内存中创建Bitmap文件。
- Input       : VOID
- Output      : NULL
- Return      : INT :
                    0 : 创建成功。
                   -1 : 申请缓存失败。
- Others      :
*******************************************************************************/
INT __USCP_VID_CreateBitmapFile(VOID)
{
    UINT              uiFileSize ;
    BITMAPFILEHEADER *pstBitmapFileHeader ;
    BITMAPINFO       *pstBitmapInfo ;


    /* 根据图像分辨率申请图片缓存 */
    uiFileSize = USCP_VID_VIDEO_BYTES_BGR + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO) ;

    g_stVidWorkarea.pucBitmapFile = (UCHAR *)malloc(uiFileSize) ;

    if (NULL == g_stVidWorkarea.pucBitmapFile)
    {
        AfxMessageBox(_T("VID : 创建Bitmap文件失败"), MB_OK) ;

        return -1 ;
    }

    /* 设定Bitmap文件头 */
    pstBitmapFileHeader = (BITMAPFILEHEADER *)g_stVidWorkarea.pucBitmapFile ;

    pstBitmapFileHeader->bfType      = 'MB' ;
    pstBitmapFileHeader->bfSize      = uiFileSize ; /* 整个文件的大小 */
    pstBitmapFileHeader->bfReserved1 = 0 ;
    pstBitmapFileHeader->bfReserved2 = 0 ;
    pstBitmapFileHeader->bfOffBits   = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) ; /* 头部大小 */

    /* 设定Bitmap信息头 */
    pstBitmapInfo = (BITMAPINFO *)(g_stVidWorkarea.pucBitmapFile + sizeof(BITMAPFILEHEADER)) ;

    pstBitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER) ;
    pstBitmapInfo->bmiHeader.biWidth         = USCP_VID_VIDEO_WIDTH ;
    pstBitmapInfo->bmiHeader.biHeight        = INT(0) - (INT)USCP_VID_VIDEO_HEIGHT ;
    pstBitmapInfo->bmiHeader.biPlanes        = 1 ;
    pstBitmapInfo->bmiHeader.biBitCount      = 24 ;
    pstBitmapInfo->bmiHeader.biCompression   = BI_RGB ;
    pstBitmapInfo->bmiHeader.biSizeImage     = USCP_VID_VIDEO_BYTES_BGR ;
    pstBitmapInfo->bmiHeader.biXPelsPerMeter = 0 ;
    pstBitmapInfo->bmiHeader.biYPelsPerMeter = 0 ;
    pstBitmapInfo->bmiHeader.biClrUsed       = 0 ;
    pstBitmapInfo->bmiHeader.biClrImportant  = 0 ;

    g_stVidWorkarea.pucBitmapData = g_stVidWorkarea.pucBitmapFile + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) ;

    return 0 ;
}


/*******************************************************************************
- Function    : __USCP_VID_DeleteBitmapFile
- Description : 本函数删除内存中Bitmap文件。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_VID_DeleteBitmapFile(VOID)
{
    if (NULL != g_stVidWorkarea.pucBitmapFile)
    {
        free(g_stVidWorkarea.pucBitmapFile) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __USCP_VID_ConvertYuv420ToBgr24
- Description : 本函数将YUV420图像转换为BGR24图像。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_VID_ConvertYuv420ToBgr24(VOID)
{
    STATIC cv::Mat objYuvImage(USCP_VID_VIDEO_HEIGHT + USCP_VID_VIDEO_HEIGHT / 2, USCP_VID_VIDEO_WIDTH, CV_8UC1, g_stVidWorkarea.pucYuvFrame) ;
    STATIC cv::Mat objBgrImage(USCP_VID_VIDEO_HEIGHT, USCP_VID_VIDEO_WIDTH, CV_8UC3, g_stVidWorkarea.pucBitmapFile + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) ;

    cv::cvtColor(objYuvImage, objBgrImage, cv::COLOR_YUV2BGR_I420) ;

    return ;
}

/*******************************************************************************
- Function    : __USCP_VID_UpdateStatistics
- Description : 本函数更新统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_VID_UpdateStatistics(VOID)
{
    STATIC UINT uiPrevTotalFrames = 0 ;

    /* 计算帧率 */
    g_stVidWorkarea.stStatistics.fCurrentFps = ((FLOAT)(g_stVidWorkarea.stStatistics.uiTotalFrames - uiPrevTotalFrames) /
        ((FLOAT)USCP_TIMER_ELAPSE / 1000.0f)) ;

    uiPrevTotalFrames = g_stVidWorkarea.stStatistics.uiTotalFrames ;

    return ;
}


CODE_SECTION("===============================") ;
CODE_SECTION("==  线程入口函数             ==") ;
CODE_SECTION("===============================") ;
/*******************************************************************************
- Function    : USCP_VID_Thread
- Description : VID线程入口函数。
- Input       : pvParam :
                    Point to father dlg.
- Output      : null
- Return      : UINT
- Others      :
*******************************************************************************/
UINT USCP_VID_Thread(LPVOID pvParam)
{
    CUSCP_DLG  *pDlg = (CUSCP_DLG*)pvParam ;
    INT         iRetVal ;

    /* 初始化工作区 */
    iRetVal = __USCP_VID_InitWorkarea() ;

    if (0 != iRetVal)
    {
        g_stVidWorkarea.enThreadStatus = VID_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_EXIT_WITH_NO_PROCESS ;
    }

    /* 创建Bitmap文件 */
    iRetVal = __USCP_VID_CreateBitmapFile() ;

    if (0 != iRetVal)
    {
        g_stVidWorkarea.enThreadStatus = VID_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_EXIT_WITH_DISTROY_WORKAREA ;
    }

#ifdef USCP_VID_TEST_WITH_YUV_FILE
    iRetVal = __USCP_VID_TEST_OpenTestFile() ;

    if (0 != iRetVal)
    {
        g_stVidWorkarea.enThreadStatus = VID_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_EXIT_WITH_DELETE_BITMAP ;
    }
#endif

    while (1)
    {
#ifdef USCP_VID_TEST_WITH_COLOR_BAR
        /* 绘制color-bar */
        __USCP_VID_TEST_DrawColorBarInMemory() ;

        /* 给主线程发送消息，更新显示 */
        g_stVidWorkarea.stStatistics.uiTotalFrames++ ;
        SendMessage(pDlg->m_hWnd, USCP_WIN_MSG_USER_VID_UPDATE_VIDEO, FALSE, NULL) ;

        Sleep(500) ;
#elif defined USCP_VID_TEST_WITH_YUV_FILE
        /* 读取一帧YUV420图像到内存 */
        __USCP_VID_TEST_ReadYuvData() ;

        /* 将YUV图像转换为BGR24图像数据 */
        __USCP_VID_ConvertYuv420ToBgr24() ;

        /* 给主线程发送消息，更新显示 */
        g_stVidWorkarea.stStatistics.uiTotalFrames++ ;
        SendMessage(pDlg->m_hWnd, USCP_WIN_MSG_USER_VID_UPDATE_VIDEO, FALSE, NULL) ;
#else
        if (1 == g_stVidWorkarea.uiNewYuvFrame)
        {
            /* 如果有YUV图像就绪，将YUV图像转换为BGR24图像数据 */
            __USCP_VID_ConvertYuv420ToBgr24() ;

            /* 给主线程发送消息，更新显示 */
            g_stVidWorkarea.stStatistics.uiTotalFrames++ ;
            SendMessage(pDlg->m_hWnd, USCP_WIN_MSG_USER_VID_UPDATE_VIDEO, FALSE, NULL) ;

            /* 清除标记 */
            g_stVidWorkarea.uiNewYuvFrame = 0 ;
        }
#endif
        if (1 == g_stVidWorkarea.uiUpdateStatistics)
        {
            /* 更新统计信息 */
            g_stVidWorkarea.uiUpdateStatistics = 0 ;

            __USCP_VID_UpdateStatistics() ;
        }

        if (1 == g_stVidWorkarea.uiStopThread)
        {
            g_stVidWorkarea.enThreadStatus = VID_THREAD_STATUS_END ;
#ifdef USCP_VID_TEST_WITH_YUV_FILE
            goto LABEL_THREAD_EXIT_WITH_CLOSE_FILE ;
#else
            goto LABEL_THREAD_EXIT_WITH_DELETE_BITMAP ;
#endif
        }
    }

#ifdef USCP_VID_TEST_WITH_YUV_FILE
LABEL_THREAD_EXIT_WITH_CLOSE_FILE :
    __USCP_VID_TEST_CloseTestFile() ;
#endif
LABEL_THREAD_EXIT_WITH_DELETE_BITMAP :
    __USCP_VID_DeleteBitmapFile() ;
LABEL_THREAD_EXIT_WITH_DISTROY_WORKAREA :
    __USCP_VID_DistroyWorkarea() ;
LABEL_THREAD_EXIT_WITH_NO_PROCESS :
    return 0 ;
}

//#ifdef __cplusplus
//}
//#endif /* __cplusplus */
/******* End of file USCP_VID.cpp. *******/
