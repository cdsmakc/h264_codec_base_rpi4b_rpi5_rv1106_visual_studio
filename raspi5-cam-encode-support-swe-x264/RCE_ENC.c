/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_ENC.c
    Project name : Raspiberry pi 4b Camera Encode with h264
    Module name  : Encoder
    Date created : 2025年5月1日   11时36分50秒
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
#include "x264-master/x264.h"

/* 内部 */
#include "RCE_config.h"
#include "RCE_types.h"
#include "RCE_CAM.h"
#include "RCE_ENC.h"
#include "RCE_DTR.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块全局变量        ==") ;
CODE_SECTION("==========================") ;

/* 模块工作区 */
RCE_ENC_WORKAREA_S       g_stENCWorkarea ;

/* 编码器相关 */
STATIC x264_param_t      g_stParam ;
STATIC x264_picture_t    g_stPic ;
STATIC x264_t           *g_pstX264Handler ;
STATIC x264_nal_t       *g_pstNAL ;
STATIC x264_picture_t    g_stPicOut ;

/* 来自CAM模块 */
extern UCHAR            *g_pucFrameBuffer ;
extern UINT              g_uiFrameBufferValid ;

/* 配置信息 */
extern RCE_CONFIG_S      g_stRCEConfig ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块内部函数        ==") ;
CODE_SECTION("==========================") ;

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
    if(1 == g_stRCEConfig.uiWrToFile)
    {
        g_stENCWorkarea.iStoreFile = open(g_stRCEConfig.acFile, O_RDWR | O_CREAT | O_TRUNC , 0777) ;
        
        if(-1 == g_stENCWorkarea.iStoreFile)
        {
            printf("File %s, func %s, line %d : Can not open file %s.\n", __FILE__, __func__, __LINE__, g_stRCEConfig.acFile) ;

            return -1;
        }        
    }

    return 0 ;
}

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
INT __RCE_ENC_Close264File(VOID)
{
    if(1 == g_stRCEConfig.uiWrToFile)
    {
        close(g_stENCWorkarea.iStoreFile) ;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_ENC_ConfigEncoder
- Description : 本函数配置x264编码器。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_ConfigEncoder(VOID)
{
    /* 设置编码 */
    if(0 > x264_param_default_preset(&g_stParam, g_stRCEConfig.acPreset, NULL))
    {
        printf("File %s, func %s, line %d : set x264 param default fail.\n", __FILE__, __func__, __LINE__) ;

        return -1 ;
    }

    g_stParam.i_bitdepth       = 8 ;
    g_stParam.i_csp            = X264_CSP_I420 ;
    g_stParam.i_width          = g_stRCEConfig.usWidth ;
    g_stParam.i_height         = g_stRCEConfig.usHeight ;
    g_stParam.b_vfr_input      = 0 ;
    g_stParam.b_repeat_headers = 1 ;
    g_stParam.b_annexb         = 1 ;

    if(0 > x264_param_apply_profile(&g_stParam, g_stRCEConfig.acProfile))
    {
        printf("File %s, func %s, line %d : apply x264 profile fail.\n", __FILE__, __func__, __LINE__) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_AllocPicture
- Description : 本函数申请Picture空间。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_AllocPicture(VOID)
{
    if(0 > x264_picture_alloc(&g_stPic, g_stParam.i_csp, g_stParam.i_width, g_stParam.i_height))
    {
        printf("File %s, func %s, line %d : alloc picture fail.\n", __FILE__, __func__, __LINE__) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_ENC_CleanPicture
- Description : 本函数释放Picture空间。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
- Others      :
*******************************************************************************/
INT __RCE_ENC_CleanPicture(VOID)
{
    x264_picture_clean(&g_stPic) ;

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_ENC_OpenEncoder
- Description : 本函数打开编码器。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ENC_OpenEncoder(VOID)
{
    /* 启动编码器 */
    g_pstX264Handler = x264_encoder_open(&g_stParam) ;

    if(NULL == g_pstX264Handler)
    {
        printf("File %s, func %s, line %d : open x264 encoder fail.\n", __FILE__, __func__, __LINE__) ;

        return -1 ;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_ENC_CloseEncoder
- Description : 本函数关闭编码器。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
- Others      :
*******************************************************************************/
INT __RCE_ENC_CloseEncoder(VOID)
{
    x264_encoder_close(g_pstX264Handler) ;

    return 0 ;
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
    size_t             szLumaSize   = g_stRCEConfig.usWidth * g_stRCEConfig.usHeight ; 
    size_t             szChromaSize = szLumaSize / 4 ;
    int                iEncodeSize ;
    int                iNALCtr ;
    void              *pvLumaPtr    = (VOID *)g_pucFrameBuffer ;     /* Y 分量起始位置 */
    void              *pvChroma1Ptr = pvLumaPtr + szLumaSize ;       /* U 分量起始位置 */
    void              *pvChroma2Ptr = pvChroma1Ptr + szChromaSize ;  /* V 分量起始位置 */

    g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_RUN ;

    /* 打开输出文件 */

    if(-1 == __RCE_ENC_Open264File())
    {
        /* 打开输出文件失败，退出 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_NO_PROCESS ;
    }

    /* 配置编码器 */
    if(0 != __RCE_ENC_ConfigEncoder())
    {
        /* 配置编码器失败，退出线程 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_FILE ;
    }

    /* 申请Picture空间 */
    if(0 != __RCE_ENC_AllocPicture())
    {
        /* 配置Picture空间失败，退出线程 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_FILE ;
    }

    /* 启动编码器 */
    if(0 != __RCE_ENC_OpenEncoder())
    {
        /* 启动编码器失败，退出线程 */
        g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLEAN_PICTURE ;
    }

    while(1)
    {
        usleep(100) ;

        if(1 == g_uiFrameBufferValid)
        {
            /* 拷贝数据 */
            memcpy(g_stPic.img.plane[0], pvLumaPtr, szLumaSize) ;
            memcpy(g_stPic.img.plane[1], pvChroma1Ptr, szChromaSize) ;
            memcpy(g_stPic.img.plane[2], pvChroma2Ptr, szChromaSize) ;

            /* 数据拷贝完成，可以通知CAM模块释放 */
            g_uiFrameBufferValid = 0 ;

            /* 统计：更新已编码的数据量和帧数 */
            g_stENCWorkarea.stStatistics.ui64DataCountEncoded += (szLumaSize + szChromaSize + szChromaSize) ;
            g_stENCWorkarea.stStatistics.uiFrameCountEncoded++ ;

            /* 编码 */
            iEncodeSize = x264_encoder_encode(g_pstX264Handler, &g_pstNAL, &iNALCtr, &g_stPic, &g_stPicOut) ;
    
            if(0 > iEncodeSize)
            {
                printf("File %s, func %s, line %d : encode return invalid.\n", __FILE__, __func__, __LINE__) ;
            }
            else if(0 < iEncodeSize)
            {
                if(1 == g_stRCEConfig.uiWrToFile)
                {
                    write(g_stENCWorkarea.iStoreFile, g_pstNAL->p_payload, iEncodeSize) ;                    
                }

                /* 将数据发送给DTR */
                RCE_DTR_PushFIFO(g_pstNAL->p_payload, iEncodeSize) ;

                /* 统计：更新编码输出数据量 */
                g_stENCWorkarea.stStatistics.ui64DataCountGenerated += iEncodeSize ;
            }
        }

        /* 更新统计信息 */
        if(0 != g_stENCWorkarea.uiUpdateStatistics)
        {
            g_stENCWorkarea.uiUpdateStatistics = 0 ;
            
            __RCE_ENC_UpdateStatistics() ;
        }

        if(1 == g_stENCWorkarea.uiStopThread)
        {
            /* 结束线程前，需要将最后数据拷贝出 */
            while(x264_encoder_delayed_frames(g_pstX264Handler))
            {
                iEncodeSize = x264_encoder_encode(g_pstX264Handler, &g_pstNAL, &iNALCtr, NULL, &g_stPicOut) ;
        
                if(0 > iEncodeSize)
                {
                    printf("File %s, func %s, line %d : encode return invalid when stop thread.\n", __FILE__, __func__, __LINE__) ;
                }
                else if(0 < iEncodeSize)
                {
                    if(1 == g_stRCEConfig.uiWrToFile)
                    {
                        write(g_stENCWorkarea.iStoreFile, g_pstNAL->p_payload, iEncodeSize) ;                    
                    }
                    
                    /* 将数据发送给DTR */
                    RCE_DTR_PushFIFO(g_pstNAL->p_payload, iEncodeSize) ;

                    /* 统计：更新编码输出数据量 */
                    g_stENCWorkarea.stStatistics.ui64DataCountGenerated += iEncodeSize ;
                }
            }

            /* 线程结束 */
            g_stENCWorkarea.enThreadStatus = ENC_THREAD_STATUS_END ;
            
            goto LABEL_EXIT_WITH_CLOSE_ENCODER ;
        }
    }
LABEL_EXIT_WITH_CLOSE_ENCODER :
    __RCE_ENC_CloseEncoder() ;
LABEL_EXIT_WITH_CLEAN_PICTURE :
    __RCE_ENC_CleanPicture() ;
LABEL_EXIT_WITH_CLOSE_FILE :
    __RCE_ENC_Close264File() ;
LABEL_EXIT_WITH_NO_PROCESS :
    pthread_exit(NULL);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

/******* End of file RCE_ENC.c. *******/  
