/*******************************************************************************
 Copyright SAKC Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : USCP_DEC.cpp
    Project name : Unmanned System Control Platform
    Module name  :
    Date created : 2025年05月21日 8时45分19秒
    Author       : Ning.JianLi
    Description  : decode模块源文件。该模块从接收到的视频流中获取编码的H264 NAL，
                   然后解码为YUV420数据。
*******************************************************************************/

#include "pch.h"

#include "USCP_defines.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  头文件                   ==") ;
CODE_SECTION("===============================") ;
/* 标准库 */
#include <stdio.h>

/* 第三方头文件 */
#include <afxwin.h>
#include <minwindef.h>
extern "C" {
#include <libavcodec/avcodec.h>
}

/* 项目内部头文件 */
#include "USCP_types.h"
#include "USCP_Msg.h"
#include "USCP_config.h"
#include "fifo.h"

#include "USCP_VID.h"
#include "USCP_DEC.h"

CODE_SECTION("===============================") ;
CODE_SECTION("==  全局变量                 ==") ;
CODE_SECTION("===============================") ;

/* 模块工作区 */
USCP_DEC_WORKAREA_S         g_stDecWorkarea ;

/* 连接DTR和DEC线程的FIFO */
extern FIFO_DEVICE          g_stDtr2DecFifo ;

/* VID工作区声明 */
extern USCP_VID_WORKAREA_S  g_stVidWorkarea ;


extern "C" {

CODE_SECTION("===============================") ;
CODE_SECTION("==  线程内部函数             ==") ;
CODE_SECTION("===============================") ;

/*******************************************************************************
- Function    : __USCP_DEC_AllocPackage
- Description : 本函数申请存储编码图像数据的包。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT __USCP_DEC_AllocPackage(VOID)
{
    g_stDecWorkarea.pstPackage = av_packet_alloc() ;

    if (NULL == g_stDecWorkarea.pstPackage)
    {
        return -1 ;
    }
    
    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DEC_FreePackage
- Description : 本函数释放存储编码图像数据的包。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DEC_FreePackage(VOID)
{
    if (NULL != g_stDecWorkarea.pstPackage)
    {
        av_packet_free(&g_stDecWorkarea.pstPackage) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __USCP_DEC_AllocFrame
- Description : 本函数申请存储未编码图像数据的包。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT __USCP_DEC_AllocFrame(VOID)
{
    g_stDecWorkarea.pstFrame = av_frame_alloc() ;

    if (NULL == g_stDecWorkarea.pstFrame)
    {
        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DEC_FreeFrame
- Description : 本函数释放存储未编码图像数据的包。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DEC_FreeFrame(VOID)
{
    if (NULL != g_stDecWorkarea.pstFrame)
    {
        av_frame_free(&g_stDecWorkarea.pstFrame) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __USCP_DEC_FindDecoder
- Description : 本函数查找解码器。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT __USCP_DEC_FindDecoder(VOID)
{
    g_stDecWorkarea.pstCodec = avcodec_find_decoder(AV_CODEC_ID_H264) ;

    if (NULL == g_stDecWorkarea.pstCodec)
    {
        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DEC_InitParser
- Description : 本函数初始化H264视频流解析器。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT __USCP_DEC_InitParser(VOID)
{
    g_stDecWorkarea.pstParser = av_parser_init(g_stDecWorkarea.pstCodec->id) ;

    if (NULL == g_stDecWorkarea.pstParser)
    {
        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DEC_CloseParser
- Description : 本函数关闭H264视频流解析器。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DEC_CloseParser(VOID)
{
    if (NULL != g_stDecWorkarea.pstParser)
    {
        av_parser_close(g_stDecWorkarea.pstParser) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __USCP_DEC_AllocContext
- Description : 本函数申请解码器上下文。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT __USCP_DEC_AllocContext(VOID)
{
    g_stDecWorkarea.pstCodecContext = avcodec_alloc_context3(g_stDecWorkarea.pstCodec) ;

    if (NULL == g_stDecWorkarea.pstCodecContext)
    {
        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DEC_FreeContext
- Description : 本函数释放解码器上下文。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DEC_FreeContext(VOID)
{
    if (NULL != g_stDecWorkarea.pstCodecContext)
    {
        avcodec_free_context(&g_stDecWorkarea.pstCodecContext);
    }

    return ;
}

/*******************************************************************************
- Function    : __USCP_DEC_OpenCodec
- Description : 本函数激活解码器。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT __USCP_DEC_OpenCodec(VOID)
{
    INT iRetVal ;

    iRetVal = avcodec_open2(g_stDecWorkarea.pstCodecContext, g_stDecWorkarea.pstCodec, NULL) ;

    if (iRetVal < 0)
    {
        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __USCP_DEC_Decode
- Description : 本函数解码一H264数据，并将结果（YUV）保存到VID的缓存中。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
                    -2 : 失败。
- Others      :
*******************************************************************************/
INT __USCP_DEC_Decode(AVCodecContext *pstCodecContext, AVFrame *pstFrame, AVPacket *pstPackage)
{
    INT    iRetVal ;
    INT    iLoop ;
    UCHAR *pucWrPtr ;

    iRetVal = avcodec_send_packet(pstCodecContext, pstPackage) ;

    if (0 > iRetVal)
    {
        return -1 ;
    }

    while (0 <= iRetVal)
    {
        iRetVal = avcodec_receive_frame(pstCodecContext, pstFrame);

        if (iRetVal == AVERROR(EAGAIN) || iRetVal == AVERROR_EOF)
        {
            return 0 ;
        }
        else if (0 > iRetVal) 
        {
            return -2 ;
        }

        /* 更新统计信息 */
        g_stDecWorkarea.stStatistics.uiTotalDecodedFrames++ ;

        if (0 == g_stVidWorkarea.uiNewYuvFrame)
        {
            pucWrPtr = g_stVidWorkarea.pucYuvFrame ;

            /* 将数据存入缓存 */
            for (iLoop = 0; iLoop < pstFrame->height; iLoop++)
            {
                /* 拷贝Y分量 */
                memcpy(pucWrPtr,
                    pstFrame->data[0] + iLoop * pstFrame->linesize[0], 
                    pstFrame->width) ;

                pucWrPtr += pstFrame->width ;
            }

            for (iLoop = 0; iLoop < pstFrame->height / 2; iLoop++)
            {
                /* 拷贝U分量 */
                memcpy(pucWrPtr,
                    pstFrame->data[1] + iLoop * pstFrame->linesize[1], 
                    pstFrame->width / 2) ;

                pucWrPtr += (pstFrame->width / 2) ;
            }

            for (iLoop = 0; iLoop < pstFrame->height / 2; iLoop++)
            {
                /* 拷贝V分量 */
                memcpy(pucWrPtr,
                    pstFrame->data[2] + iLoop * pstFrame->linesize[2], 
                    pstFrame->width / 2) ;

                pucWrPtr += (pstFrame->width / 2) ;
            }

            g_stVidWorkarea.uiNewYuvFrame = 1 ;
        }
    }

    return 0 ;
}
#if 0
static FILE* fout;
UINT uiWriteTimes = 0 ;
static void pgm_save(AVFrame* frame, int xsize, int ysize, const char* filename)
{
    uiWriteTimes++ ;

    int i;
    if (!fout)
    {
        fout = fopen(filename, "wb");
    }


    //linesize中存储的是宽度，不是总长度
    //写入顺序，Y  V  U =YV12 ;解码后的数据是YUV顺序存储的
    for (i = 0; i < ysize; i++)
        fwrite(frame->data[0] + i * frame->linesize[0], 1, xsize, fout);

    for (i = 0; i < ysize / 2; i++)
        fwrite(frame->data[2] + i * frame->linesize[2], 1, xsize / 2, fout);

    for (i = 0; i < ysize / 2; i++)
        fwrite(frame->data[1] + i * frame->linesize[1], 1, xsize / 2, fout);

}

static void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const char* filename)
{
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3ld\n", dec_ctx->frame_num);
        printf("YUV pts:%ld \n", frame->pts);
        fflush(stdout);

        /* the picture is allocated by the decoder. no need to
           free it */

        pgm_save(frame, frame->width, frame->height, filename);
    }
}
#endif

/*******************************************************************************
- Function    : __USCP_DEC_UpdateStatistics
- Description : 本函数更新统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __USCP_DEC_UpdateStatistics(VOID)
{
    STATIC UINT uiPrevTotalDecodedFrames = 0 ;

    /* 计算帧率 */
    g_stDecWorkarea.stStatistics.fCurrentFps = ((FLOAT)(g_stDecWorkarea.stStatistics.uiTotalDecodedFrames - uiPrevTotalDecodedFrames) /
        ((FLOAT)USCP_TIMER_ELAPSE / 1000.0f)) ;

    uiPrevTotalDecodedFrames = g_stDecWorkarea.stStatistics.uiTotalDecodedFrames ;

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
UINT USCP_DEC_Thread(LPVOID pvParam)
{
    INT     iRetVal ;
    UCHAR   aucInputBuf[USCP_DEC_CODEC_INPUT_BUF + AV_INPUT_BUFFER_PADDING_SIZE] ;
    UCHAR  *pucInputBuf ;
    UINT    uiFifoReadSize ;

    /* 更新线程状态 */
    g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_RUN ;

    /* 申请存储压缩数据的包 */
    iRetVal = __USCP_DEC_AllocPackage() ;

    if (0 != iRetVal)
    {
        g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_WHIT_NO_PROCESS ;
    }

    /* 申请存储未压缩数据的包 */
    iRetVal = __USCP_DEC_AllocFrame() ;

    if (0 != iRetVal)
    {
        g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_WHIT_FREE_PACKAGE ;
    }

    /* 查找解码器 */
    iRetVal = __USCP_DEC_FindDecoder() ;

    if (0 != iRetVal)
    {
        g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_WHIT_FREE_FRAME ;
    }

    /* 初始化解析器 */
    iRetVal = __USCP_DEC_InitParser() ;

    if (0 != iRetVal)
    {
        g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_WHIT_FREE_FRAME ;
    }

    /* 申请解码器上下文 */
    iRetVal = __USCP_DEC_AllocContext() ;

    if (0 != iRetVal)
    {
        g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_WHIT_CLOSE_PARSER ;
    }

    /* 打开解码器 */
    iRetVal = __USCP_DEC_OpenCodec() ;

    if (0 != iRetVal)
    {
        g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
        goto LABEL_THREAD_WHIT_FREE_CONTEXT ;
    }

    /* 清理输入缓存 */ /* 只需清理尾部，必需确保为0 */
    memset(aucInputBuf + USCP_DEC_CODEC_INPUT_BUF, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    while (1)
    {
        /* 从FIFO获取数据 */
        if (USCP_DEC_CODEC_INPUT_BUF <= FIFO_GetFifoDataCount(&g_stDtr2DecFifo))
        {
            uiFifoReadSize = FIFO_Get(aucInputBuf, USCP_DEC_CODEC_INPUT_BUF, &g_stDtr2DecFifo) ;

            pucInputBuf = aucInputBuf ;

            while (0 < uiFifoReadSize)
            {
                /* 解析数据 */
                iRetVal = av_parser_parse2(g_stDecWorkarea.pstParser,
                    g_stDecWorkarea.pstCodecContext,
                    &(g_stDecWorkarea.pstPackage->data),
                    &(g_stDecWorkarea.pstPackage->size),
                    pucInputBuf,
                    uiFifoReadSize,
                    AV_NOPTS_VALUE,
                    AV_NOPTS_VALUE,
                    0) ;

                if (0 > iRetVal)
                {
                    g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
                    goto LABEL_THREAD_WHIT_FREE_CONTEXT ;
                }

                pucInputBuf    += iRetVal ;
                uiFifoReadSize -= iRetVal ;

                if (0 < g_stDecWorkarea.pstPackage->size)
                {
#if 1
                    iRetVal = __USCP_DEC_Decode(g_stDecWorkarea.pstCodecContext, g_stDecWorkarea.pstFrame, g_stDecWorkarea.pstPackage) ;

                    if (0 > iRetVal)
                    {
                        g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_EXIT ;
                        goto LABEL_THREAD_WHIT_FREE_CONTEXT ;
                    }
 #else
                    decode(g_stDecWorkarea.pstCodecContext, g_stDecWorkarea.pstFrame, g_stDecWorkarea.pstPackage, "output.yuv") ;
#endif               
                }

            }
        }
        else
        {
            Sleep(1) ;
        }

        if (1 == g_stDecWorkarea.uiUpdateStatistics)
        {
            /* 更新统计信息 */
            g_stDecWorkarea.uiUpdateStatistics = 0 ;

            __USCP_DEC_UpdateStatistics() ;
        }

        if (1 == g_stDecWorkarea.uiStopThread)
        {
            g_stDecWorkarea.enThreadStatus = DEC_THREAD_STATUS_END ;

            goto LABEL_THREAD_WHIT_FREE_CONTEXT ;
        }
    }

LABEL_THREAD_WHIT_FREE_CONTEXT :
    __USCP_DEC_FreeContext() ;
LABEL_THREAD_WHIT_CLOSE_PARSER :
    __USCP_DEC_CloseParser() ;
LABEL_THREAD_WHIT_FREE_FRAME :
    __USCP_DEC_FreeFrame() ;
LABEL_THREAD_WHIT_FREE_PACKAGE :
    __USCP_DEC_FreePackage() ;
LABEL_THREAD_WHIT_NO_PROCESS :
    return 0 ;
}

}

/******* End of file USCP_VID.cpp. *******/

