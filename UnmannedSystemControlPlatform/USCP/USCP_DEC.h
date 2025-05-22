/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : USCP_DEC.h
    Project name : Unmanned System Control Platform
    Module name  :
    Date created : 2025年05月21日 8时45分19秒
    Author       : Ning.JianLi
    Description  : decode模块源文件。该模块从接收到的视频流中获取编码的H264 NAL，
                   然后解码为YUV420数据。
*******************************************************************************/

#ifndef __USCP_DEC_H__
#define __USCP_DEC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <libavcodec/avcodec.h>

#include "USCP_defines.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  头文件                   ==") ;
CODE_SECTION("===============================") ;


CODE_SECTION("===============================") ;
CODE_SECTION("==  模块内宏定义             ==") ;
CODE_SECTION("===============================") ;

#define USCP_DEC_CODEC_INPUT_BUF                     (                                4096u)

CODE_SECTION("===============================") ;
CODE_SECTION("==  模块枚举数据类型         ==") ;
CODE_SECTION("===============================") ;
/* 线程状态 */
typedef enum tagUSCP_DEC_THREAD_STATUS
{
    DEC_THREAD_STATUS_NOT_START = 0, /* 线程尚未运行 */
    DEC_THREAD_STATUS_RUN       = 1, /* 线程正在运行 */
    DEC_THREAD_STATUS_EXIT      = 2, /* 线程异常退出 */
    DEC_THREAD_STATUS_END       = 3  /* 线程正常退出 */
} USCP_DEC_THREAD_STATUS_E ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  模块结构数据类型         ==") ;
CODE_SECTION("===============================") ;
/* 统计信息 */
typedef struct tagUSCP_DEC_STATISTICS
{
    UINT  uiTotalDecodedFrames ; /* 累计解出帧数 */
    FLOAT fCurrentFps ;          /* 当前帧率 */
} USCP_DEC_STATISTICS_S ;

/* 模块工作区 */
typedef struct tagUSCP_DEC_WORKAREA
{
    union
    {
        UCHAR aucRawDATA[1024] ;

        struct
        {
            /* DW0 */ /* 仅用作标记 */
            UINT                      uiStopThread       :  1 ;
            UINT                      uiUpdateStatistics :  1 ;
            UINT                      uiDW0Rsv           : 30 ;

            /* 线程运行状态 */
            USCP_DEC_THREAD_STATUS_E  enThreadStatus ;
            /* 编码图像包 */
            AVPacket                 *pstPackage ;

            /* 未编码图像包 */
            AVFrame                  *pstFrame ;

            /* 解码器 */
            const AVCodec            *pstCodec ;

            /* 编码数据解析器 */
            AVCodecParserContext     *pstParser ;

            /* 解码器上下文 */
            AVCodecContext           *pstCodecContext ;

            /* 统计信息 */
            USCP_DEC_STATISTICS_S     stStatistics ;
        } ;
    } ;
} USCP_DEC_WORKAREA_S ;

CODE_SECTION("===============================") ;
CODE_SECTION("==  函数声明                 ==") ;
CODE_SECTION("===============================") ;

/* 线程函数 */
UINT USCP_DEC_Thread(LPVOID pvParam) ;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __USCP_DEC_H__ */

/******* End of file USCP_DEC.h. *******/
