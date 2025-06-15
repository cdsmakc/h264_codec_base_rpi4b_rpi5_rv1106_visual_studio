/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_ENC.c
    Project name : RV1106 Camera Encode with h264
    Module name  : Encoder
    Date created : 2025年6月5日   22时09分50秒
    Author       : 
    Description  : 
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "RCE_defines.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块包含文件        ==") ;
CODE_SECTION("==========================") ;

/* 标准库 */
#include <asm/types.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

/* 第三方 */
#include "rk_common.h"
#include "rk_comm_venc.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_sys.h"


/* 内部 */
#include "RCE_types.h"
#include "RCE_config.h"
#include "RCE_CAM.h"
#include "RCE_ENC.h"
#include "RCE_DTR.h"


CODE_SECTION("==========================") ;
CODE_SECTION("==  模块全局变量        ==") ;
CODE_SECTION("==========================") ;

/* 模块工作区 */
RCE_ENC_WORKAREA_S            g_stENCWorkarea ;

/* RV1106编码器设置 */
STATIC VENC_CHN_ATTR_S        g_stChnAttr ;
STATIC VENC_RC_PARAM_S        g_stRcParam ;
STATIC VENC_RECV_PIC_PARAM_S  g_stRecvParam ;
STATIC VENC_STREAM_S          g_stStream ;
STATIC MB_POOL_CONFIG_S       g_stMBPoolCfg ;
STATIC MB_POOL                g_uiMBPool ;

/* 来自CAM模块 */
extern UCHAR                 *g_pucFrameBuffer ;
extern UINT                   g_uiFrameBufferValid ;

/* 配置信息 */
extern RCE_CONFIG_S           g_stRCEConfig ;

UINT                        ui64Time1 ;
UINT                        ui64Time2 ;
UINT                        ui64Time3 ;
UINT                        ui64Time4 ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块内部函数        ==") ;
CODE_SECTION("==========================") ;

UINT getMilliseconds() 
{
   struct timespec currentTime ;
   clock_gettime(CLOCK_MONOTONIC, &currentTime) ;

   UINT milliseconds = currentTime.tv_sec * 1000 + currentTime.tv_nsec / 1000000 ;

   return milliseconds ;
}

#if 1 == RCE_H264_TO_FILE
/*******************************************************************************
- Function    : __RCE_ENC_Open264File
- Description : 本函数打开输出文件。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_Open264File(VOID)
{
    g_stENCWorkarea.iStoreFile = open(RCE_H264_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC , 0777) ;
    
    if(-1 == g_stENCWorkarea.iStoreFile)
    {
        printf("File %s, func %s, line %d : Can not open file %s.\n", __FILE__, __func__, __LINE__, RCE_H264_FILE_NAME) ;

        return -1;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_Close264File
- Description : 本函数关闭输出文件。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_ENC_Close264File(VOID)
{
    close(g_stENCWorkarea.iStoreFile) ;

    return ;
}
#endif /* if 1 == RCE_H264_TO_FILE */

/*******************************************************************************
- Function    : __RCE_ENC_AllocStreamPack
- Description : 本函数申请输出码流包空间。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_AllocStreamPack(VOID)
{
    /* 申请部分缓存 */
    g_stStream.pstPack = (VENC_PACK_S *)(malloc(sizeof(VENC_PACK_S))) ;

    if(NULL == g_stStream.pstPack)
    {
        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_FreeStreamPack
- Description : 本函数申请释放码流包空间。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_ENC_FreeStreamPack(VOID)
{
    if(NULL != g_stStream.pstPack)
    {
        free(g_stStream.pstPack) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __RCE_ENC_CreateChannel
- Description : 本函数创建RV1106编码通道。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_CreateChannel(VOID)
{
    INT iRetVal ;

    g_stChnAttr.stVencAttr.enType                      = RK_VIDEO_ID_AVC ;
	g_stChnAttr.stVencAttr.enPixelFormat               = RK_FMT_YUV420SP ;
	g_stChnAttr.stVencAttr.u32MaxPicWidth              = g_stRCEConfig.usWidth ;
	g_stChnAttr.stVencAttr.u32MaxPicHeight             = g_stRCEConfig.usHeight ;
	g_stChnAttr.stVencAttr.u32PicWidth                 = g_stRCEConfig.usWidth ;
	g_stChnAttr.stVencAttr.u32PicHeight                = g_stRCEConfig.usHeight ;
	g_stChnAttr.stVencAttr.u32VirWidth                 = RK_ALIGN_2(g_stRCEConfig.usWidth) ;
	g_stChnAttr.stVencAttr.u32VirHeight                = RK_ALIGN_2(g_stRCEConfig.usHeight) ;
    g_stChnAttr.stVencAttr.u32StreamBufCnt             = RCE_ENC_CHN_ATTR_STREAM_BUF_COUNT ;
    g_stChnAttr.stVencAttr.u32BufSize                  = g_stRCEConfig.usWidth * g_stRCEConfig.usHeight * 3 / 2 ;

#if VENC_RC_MODE_H264CBR == RCE_ENC_RC_MODE
    /* 恒定码率 */
    g_stChnAttr.stRcAttr.enRcMode                      = RCE_ENC_RC_MODE ;
    g_stChnAttr.stRcAttr.stH264Cbr.u32Gop              = RCE_ENC_RC_GOP ;
    g_stChnAttr.stRcAttr.stH264Cbr.u32BitRate          = RCE_ENC_RC_BITRATE ;
    g_stChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1 ;
    g_stChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = g_stRCEConfig.uiFps ;
    g_stChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateDen  = 1 ;
    g_stChnAttr.stRcAttr.stH264Cbr.u32SrcFrameRateNum  = g_stRCEConfig.uiFps ;
#else 
    /* 可变码率 */
    g_stChnAttr.stRcAttr.enRcMode                      = RCE_ENC_RC_MODE ;
    g_stChnAttr.stRcAttr.stH264Vbr.u32Gop              = RCE_ENC_RC_GOP ;
    g_stChnAttr.stRcAttr.stH264Vbr.u32BitRate          = RCE_ENC_RC_BITRATE;
    g_stChnAttr.stRcAttr.stH264Vbr.fr32DstFrameRateDen = 1;
    g_stChnAttr.stRcAttr.stH264Vbr.fr32DstFrameRateNum = RCE_ENC_RC_FPS;
    g_stChnAttr.stRcAttr.stH264Vbr.u32SrcFrameRateDen  = 1;
    g_stChnAttr.stRcAttr.stH264Vbr.u32SrcFrameRateNum  = RCE_ENC_RC_FPS;
#endif
    iRetVal = RK_MPI_VENC_CreateChn(RCE_ENC_CHANNEL_ID, &g_stChnAttr) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : Create VENC channel fail. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_DestroyChannel
- Description : 本函数销毁RV1106编码通道。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_ENC_DestroyChannel(VOID)
{
    INT iRetVal ;

    iRetVal = RK_MPI_VENC_DestroyChn(RCE_ENC_CHANNEL_ID) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : Destroy VENC channel fail. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __RCE_ENC_SetRcParam
- Description : 本函数设置编码器的码率控制参数。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_SetRcParam(VOID)
{
    INT iRetVal ;

    memset(&g_stRcParam, 0, sizeof(g_stRcParam)) ;

    g_stRcParam.stParamH264.u32MinQp     = 10 ;
    g_stRcParam.stParamH264.u32MaxQp     = 51 ;
    g_stRcParam.stParamH264.u32MinIQp    = 10 ;
    g_stRcParam.stParamH264.u32MaxIQp    = 51 ;
    g_stRcParam.stParamH264.u32FrmMinQp  = 28 ;
    g_stRcParam.stParamH264.u32FrmMinIQp = 28 ;

    iRetVal = RK_MPI_VENC_SetRcParam(RCE_ENC_CHANNEL_ID, &g_stRcParam) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : Set VENC rate control param fail. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_StartRecvFrame
- Description : 本函数启动编码器接收图像帧。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_StartRecvFrame(VOID)
{
    INT iRetVal ;

    g_stRecvParam.s32RecvPicNum = -1 ; /* 连续编码 */

	iRetVal = RK_MPI_VENC_StartRecvFrame(RCE_ENC_CHANNEL_ID, &g_stRecvParam) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : Start recv frame fail. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_StopRecvFrame
- Description : 本函数停止编码器接收图像帧。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_ENC_StopRecvFrame(VOID)
{
    INT iRetVal ;

    iRetVal = RK_MPI_VENC_StopRecvFrame(RCE_ENC_CHANNEL_ID) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : Stop recv frame fail. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __RCE_ENC_CreateMBPool
- Description : 本函数创建内存池。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_CreateMBPool(VOID)
{
    g_stMBPoolCfg.u64MBSize                     = g_stRCEConfig.usWidth * g_stRCEConfig.usHeight * 3 / 2 ;
    g_stMBPoolCfg.u32MBCnt                      = RCE_ENC_MB_COUNT ;
    g_stMBPoolCfg.enAllocType                   = MB_ALLOC_TYPE_DMA ;
    g_stMBPoolCfg.bPreAlloc                     = RK_TRUE ;

    g_uiMBPool = RK_MPI_MB_CreatePool(&g_stMBPoolCfg) ;

    if(MB_INVALID_POOLID == g_uiMBPool)
    {
        printf("File %s, func %s, line %d : Create memory-block pool fail.\n", __FILE__, __func__, __LINE__) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_DestroyMBPool
- Description : 本函数销毁内存池。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_ENC_DestroyMBPool(VOID)
{
    INT iRetVal ;

    iRetVal = RK_MPI_MB_DestroyPool(g_uiMBPool) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : Destroy memory-block pool fail. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;
    }

    return ;
}

/*******************************************************************************
- Function    : __RCE_ENC_SendFrameToEnc
- Description : 本函数将未编码的图像数据发送到编码器。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.申请缓存失败。
                    -2 : fail.发送图像失败。
- Others      :
*******************************************************************************/
INT __RCE_ENC_SendFrameToEnc(VOID)
{
    MB_BLK              pvMBlk     = RK_NULL ;
    UCHAR              *pucVirAddr = RK_NULL ;
    VIDEO_FRAME_INFO_S  stFrame ;
    UINT                uiSize ;
    INT                 iRetVal ;

    /* 申请Memory-Block */
    uiSize = g_stRCEConfig.usWidth * g_stRCEConfig.usHeight * 3 / 2 ;
    pvMBlk = RK_MPI_MB_GetMB(g_uiMBPool, uiSize, RK_TRUE) ;

    if(RK_NULL == pvMBlk)
    {
        printf("File %s, func %s, line %d : Get memory block failed.\n", __FILE__, __func__, __LINE__) ;
        return -1 ;
    }

    /* 将存储块转换为虚拟地址 */
    pucVirAddr = (UCHAR *)(RK_MPI_MB_Handle2VirAddr(pvMBlk)) ;

    /* 将图像数据拷贝到缓存 */
    memcpy(pucVirAddr, g_pucFrameBuffer, uiSize) ;

    /* 刷新cache */
    RK_MPI_SYS_MmzFlushCache(pvMBlk, RK_FALSE) ;

    /* 填写帧结构 */
    stFrame.stVFrame.pMbBlk          = pvMBlk ;
    stFrame.stVFrame.u32Width        = g_stRCEConfig.usWidth ;
    stFrame.stVFrame.u32Height       = g_stRCEConfig.usHeight ;
    stFrame.stVFrame.u32VirWidth     = g_stRCEConfig.usWidth ;
    stFrame.stVFrame.u32VirHeight    = g_stRCEConfig.usHeight ;
    stFrame.stVFrame.enPixelFormat   = RK_FMT_YUV420SP ;
    stFrame.stVFrame.u32FrameFlag   |= 0 ;
    stFrame.stVFrame.enCompressMode  = COMPRESS_MODE_NONE ;

    /* 发送帧到编码器 */
    iRetVal = RK_MPI_VENC_SendFrame(RCE_ENC_CHANNEL_ID, &stFrame, -1) ;

    /* 释放缓存 */
    if(RK_SUCCESS == iRetVal)
    {
        iRetVal = RK_MPI_MB_ReleaseMB(pvMBlk) ;
    }
    else
    {
        printf("File %s, func %s, line %d : Send frame failed. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;
        return -2 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_RecvStreamFromEnc
- Description : 本函数尝试获取编码的图像数据。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_RecvStreamFromEnc(VOID)
{
    INT     iRetVal ;
    VOID   *pv264data ;

    iRetVal = RK_MPI_VENC_GetStream(RCE_ENC_CHANNEL_ID, &g_stStream, 1) ; /* 注意：第三个参数设置为0，实际上会阻塞100ms。设置成1ms，提高效率。 */

    if(RK_SUCCESS == iRetVal)
    {
        pv264data = RK_MPI_MB_Handle2VirAddr(g_stStream.pstPack->pMbBlk) ;

#if 1 == RCE_H264_TO_FILE
        write(g_stENCWorkarea.iStoreFile, pv264data, g_stStream.pstPack->u32Len) ;
        
#endif
        /* 将数据发送给DTR */
        RCE_DTR_PushFIFO(pv264data, g_stStream.pstPack->u32Len) ;

        /* 释放stream */
        iRetVal = RK_MPI_VENC_ReleaseStream(RCE_ENC_CHANNEL_ID, &g_stStream);

        if(RK_SUCCESS != iRetVal)
        {
            printf("File %s, func %s, line %d : Release stream failed. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;

            return -1 ;
        }

        /* 统计：更新编码输出数据量 */
        g_stENCWorkarea.stStatistics.ui64DataCountGenerated += g_stStream.pstPack->u32Len ;
    }
    else if(RK_ERR_VENC_BUF_EMPTY == iRetVal)
    {
        /* 无数据，可正常退出 */
        return 0 ;
    }
    else
    {
        printf("File %s, func %s, line %d : Get stream failed. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;
        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_InitRkMpi
- Description : 本函数初始化RK-MPI。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_InitRkMpi(VOID)
{
    INT iRetVal ;

    iRetVal = RK_MPI_SYS_Init() ;

	if (RK_SUCCESS != iRetVal) 
    {
        printf("File %s, func %s, line %d : Init RK MPI failed. Return value is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;

        return -1 ;
	}

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_ExitRkMpi
- Description : 本函数退出RK-MPI。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_ENC_ExitRkMpi(VOID)
{
    RK_MPI_SYS_Exit() ;
}

/*******************************************************************************
- Function    : __RCE_ENC_UpdateStatistics
- Description : 本函数更新统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_ENC_UpdateStatistics(VOID)
{
    STATIC UINT   uiPrevFrameCountEncoded    = 0 ;
    STATIC UINT64 ui64PrevDataCountGenerated = 0 ;

    /* 定时器信号，计算统计信息 */
    g_stENCWorkarea.stStatistics.uiRunTime += RCE_STATISTIC_PERIOD_USEC ;

    /* 计算累计编码的FPS */
    if(0 != g_stENCWorkarea.stStatistics.uiRunTime)
    {
        g_stENCWorkarea.stStatistics.fTotalEncodeFPS = (FLOAT)g_stENCWorkarea.stStatistics.uiFrameCountEncoded / 
                                                        ((FLOAT)g_stENCWorkarea.stStatistics.uiRunTime / 1000000.0f) ;
    }

    /* 计算实时编码的FPS */
    g_stENCWorkarea.stStatistics.fCurrentEncodeFPS = (FLOAT)(g_stENCWorkarea.stStatistics.uiFrameCountEncoded - uiPrevFrameCountEncoded) / 
                                                      ((FLOAT)RCE_STATISTIC_PERIOD_USEC / 1000000.0f) ;

    /* 计算累计编码的BPS */
    if(0 != g_stENCWorkarea.stStatistics.uiRunTime)
    {
        g_stENCWorkarea.stStatistics.uiTotalEncodeBPS = (UINT)((FLOAT)(g_stENCWorkarea.stStatistics.ui64DataCountGenerated * 8) / 
                                                        ((FLOAT)g_stENCWorkarea.stStatistics.uiRunTime / 1000000.0f)) ;
    }

    /* 计算实时编码的BPS */
    g_stENCWorkarea.stStatistics.uiCurrentEncodeBPS = (FLOAT)((g_stENCWorkarea.stStatistics.ui64DataCountGenerated - ui64PrevDataCountGenerated) * 8) / 
                                                      ((FLOAT)RCE_STATISTIC_PERIOD_USEC / 1000000.0f) ;

    /* 更新静态变量以便下一次计算 */
    uiPrevFrameCountEncoded    = g_stENCWorkarea.stStatistics.uiFrameCountEncoded ;
    ui64PrevDataCountGenerated = g_stENCWorkarea.stStatistics.ui64DataCountGenerated ;

    return ;
}

CODE_SECTION("==========================") ;
CODE_SECTION("==  线程入口            ==") ;
CODE_SECTION("==========================") ;

/*******************************************************************************
- Function    : RCE_ENC_Thread
- Description : 本函数为ENC模块的线程函数。
- Input       : VOID
- Output      : NULL
- Return      : VOID *
- Others      :
*******************************************************************************/
VOID *RCE_ENC_Thread(VOID *pvArgs)
{
    g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_RUN ;

    /* 打开输出文件 */
#if 1 == RCE_H264_TO_FILE
    if(-1 == __RCE_ENC_Open264File())
    {
        /* 打开输出文件失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_NO_PROCESS ;
    }
#endif

    /* 初始化RK-MPI */
    if(-1 == __RCE_ENC_InitRkMpi())
    {
        /* 初始化失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
#if 1 == RCE_H264_TO_FILE
        goto LABEL_EXIT_WITH_CLOSE_FILE ;
#else
        goto LABEL_EXIT_WITH_NO_PROCESS ;
#endif 
    }

    /* 申请码流包空间 */
    if(-1 == __RCE_ENC_AllocStreamPack())
    {
        /* 申请失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_EXIT_RK_MPI ;
    }

    /* 创建编码通道 */
    if(-1 == __RCE_ENC_CreateChannel())
    {
        /* 创建编码通道失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_FREE_STREAM_PACK ;
    }

    /* 设置码流控制参数 */
    if(-1 == __RCE_ENC_SetRcParam())
    {
        /* 设置码流控制参数失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_DESTROY_CHANNEL ;
    }

    /* 申请缓存池 */
    if(-1 == __RCE_ENC_CreateMBPool())
    {
        /* 申请缓存池失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_DESTROY_CHANNEL ;
    }

    /* 启动接收图像帧 */
    if(-1 == __RCE_ENC_StartRecvFrame())
    {
        /* 启动接收图像帧失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_DESTROY_MB_POOL ;
    }

    while(1)
    {
        usleep(100) ;

        if(1 == g_uiFrameBufferValid)
        {
            /* 发送图像到编码器 */
            if(-1 == __RCE_ENC_SendFrameToEnc())
            {
                g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
                goto LABEL_EXIT_WITH_STOP_RECV_FRAME ;
            }

            /* 数据拷贝完成，可以通知CAM模块释放 */
            g_uiFrameBufferValid = 0 ;

            /* 统计：更新已编码的数据量和帧数 */
            g_stENCWorkarea.stStatistics.ui64DataCountEncoded += g_stRCEConfig.usWidth * g_stRCEConfig.usHeight * 3 / 2 ;
            g_stENCWorkarea.stStatistics.uiFrameCountEncoded++ ;
        }

        if(-1 == __RCE_ENC_RecvStreamFromEnc())
        {
            g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
            goto LABEL_EXIT_WITH_STOP_RECV_FRAME ;
        }

        /* 更新统计信息 */
        if(0 != g_stENCWorkarea.uiUpdateStatistics)
        {
            g_stENCWorkarea.uiUpdateStatistics = 0 ;
            
            __RCE_ENC_UpdateStatistics() ;
        }

        if(1 == g_stENCWorkarea.uiStopThread)
        {
            /* 线程结束 */
            g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_END ;
            
            goto LABEL_EXIT_WITH_STOP_RECV_FRAME ;
        }
    }

LABEL_EXIT_WITH_STOP_RECV_FRAME :
    __RCE_ENC_StopRecvFrame() ;
LABEL_EXIT_WITH_DESTROY_MB_POOL :
    __RCE_ENC_DestroyMBPool() ;
LABEL_EXIT_WITH_DESTROY_CHANNEL :
    __RCE_ENC_DestroyChannel() ;
LABEL_EXIT_WITH_FREE_STREAM_PACK :
    __RCE_ENC_FreeStreamPack() ;
LABEL_EXIT_WITH_EXIT_RK_MPI :
    __RCE_ENC_ExitRkMpi() ;
#if 1 == RCE_H264_TO_FILE
LABEL_EXIT_WITH_CLOSE_FILE :
    __RCE_ENC_Close264File() ;
#endif
LABEL_EXIT_WITH_NO_PROCESS :
    pthread_exit(NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/******* End of file RCE_ENC.c. *******/  
