/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE.c
    Project name : Raspiberry pi 4b Camera Encode with h264
    Module name  : Raspiberry pi 4b Camera Encode with h264
    Date created : 2025年5月1日   9时56分32秒
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

/* 第三方 */

/* 内部 */
#include "RCE_config.h"
#include "RCE_CAM.h"
#if 1 != RCE_USE_HARDWARE_ENCODER
#include "RCE_ENC.h"
#endif
#include "RCE_DTR.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块全局变量        ==") ;
CODE_SECTION("==========================") ;

/* 模块工作区 */
extern RCE_CAM_WORKAREA_S  g_stCAMWorkarea ;
#if 1 != RCE_USE_HARDWARE_ENCODER
extern RCE_ENC_WORKAREA_S  g_stENCWorkarea ;
#endif /* 1 != RCE_USE_HARDWARE_ENCODER */
extern RCE_DTR_WORKAREA_S  g_stDTRWorkarea ;

/* 标记 */
UINT                       uiLogStatistics = 1 ;

/* 线程ID */
pthread_t                  g_tidCAM ;
pthread_t                  g_tidDTR ;
#if 1 != RCE_USE_HARDWARE_ENCODER
pthread_t                  g_tidENC ;
#endif /* 1 != RCE_USE_HARDWARE_ENCODER */


CODE_SECTION("==========================") ;
CODE_SECTION("==  内部函数            ==") ;
CODE_SECTION("==========================") ;

/*******************************************************************************
- Function    : __RCE_StopThreadCAM
- Description : 本函数停止CAM线程。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_StopThreadCAM(VOID)
{
    INT iMaxRetry = 1000 ;

    /* 设置标记，命令CAM线程退出 */
    g_stCAMWorkarea.uiStopThread = 1 ;

    /* 等待线程结束 */
    while(CAM_THREAD_STATUS_END != g_stCAMWorkarea.enThreadStatus && CAM_THREAD_STATUS_EXIT != g_stCAMWorkarea.enThreadStatus)
    {
        if(0 >= iMaxRetry)
        {
            printf("File %s, func %s, line %d : Wait thread CAM exit timeout. \n", __FILE__, __func__, __LINE__) ;

            return ;
        }

        usleep(1000) ;
    }

    printf("Thread CAM stopped.\n") ;
    fflush(stdout) ;

    return ;
}


#if 1 != RCE_USE_HARDWARE_ENCODER
/*******************************************************************************
- Function    : __RCE_StopThreadENC
- Description : 本函数停止ENC线程。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_StopThreadENC(VOID)
{
    INT iMaxRetry = 1000 ;

    /* 设置标记，命令ENC线程退出 */
    g_stENCWorkarea.uiStopThread = 1 ;

    /* 等待线程结束 */
    while(ENC_THREAD_STATUS_END != g_stENCWorkarea.enThreadStatus && ENC_THREAD_STATUS_EXIT != g_stENCWorkarea.enThreadStatus)
    {
        if(0 >= iMaxRetry)
        {
            printf("File %s, func %s, line %d : Wait thread ENC exit timeout. \n", __FILE__, __func__, __LINE__) ;

            return ;
        }

        usleep(1000) ;
    }

    printf("Thread ENC stopped.\n") ;
    fflush(stdout) ;

    return ;
}
#endif /* 1 != RCE_USE_HARDWARE_ENCODER */

/*******************************************************************************
- Function    : __RCE_StopThreadDTR
- Description : 本函数停止DTR线程。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_StopThreadDTR(VOID)
{
    INT iMaxRetry = 1000 ;

    /* 设置标记，命令DTR线程退出 */
    g_stDTRWorkarea.uiStopThread = 1 ;

    /* 等待线程结束 */
    while(DTR_THREAD_STATUS_END != g_stDTRWorkarea.enThreadStatus && DTR_THREAD_STATUS_EXIT != g_stDTRWorkarea.enThreadStatus)
    {
        if(0 >= iMaxRetry)
        {
            printf("File %s, func %s, line %d : Wait thread DTR exit timeout. \n", __FILE__, __func__, __LINE__) ;

            return ;
        }

        usleep(1000) ;
    }

    printf("Thread DTR stopped.\n") ;
    fflush(stdout) ;

    return ;
}

/*******************************************************************************
- Function    : __RCE_DPS_DumpCAMStatistics
- Description : 本函数在终端中显示CAM线程的运行状态和统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_DPS_DumpCAMStatistics(VOID)
{
    CHAR acPrintBufTotalBPS[80] ;
    CHAR acPrintBufCurrentBPS[80] ;
    CHAR acPrintBufDataRecved[80] ;

    /* 整理累计图像数据速率 */
    if(g_stCAMWorkarea.stStatistics.uiTotalBPS >= 1000000)
    {
        sprintf(acPrintBufTotalBPS, 
                "%3.3f Mbps", 
                (FLOAT)g_stCAMWorkarea.stStatistics.uiTotalBPS / 1000000.0f) ;
    }
    else if(g_stCAMWorkarea.stStatistics.uiTotalBPS >= 1000)
    {
        sprintf(acPrintBufTotalBPS, 
                "%3.3f Kbps", 
                (FLOAT)g_stCAMWorkarea.stStatistics.uiTotalBPS / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufTotalBPS, 
                "%u bps", 
                g_stCAMWorkarea.stStatistics.uiTotalBPS) ;
    }

    /* 整理实时图像数据速率 */
    if(g_stCAMWorkarea.stStatistics.uiCurrentBPS >= 1000000)
    {
        sprintf(acPrintBufCurrentBPS, 
                "%3.3f Mbps", 
                (FLOAT)g_stCAMWorkarea.stStatistics.uiCurrentBPS / 1000000.0f) ;
    }
    else if(g_stCAMWorkarea.stStatistics.uiCurrentBPS >= 1000)
    {
        sprintf(acPrintBufCurrentBPS, 
                "%3.3f Kbps", 
                (FLOAT)g_stCAMWorkarea.stStatistics.uiCurrentBPS / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufCurrentBPS, 
                "%u bps", 
                g_stCAMWorkarea.stStatistics.uiCurrentBPS) ;
    }

    /* 整理累计从摄像头获取的数据量 */
    if(g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera >= 1024 * 1024 *1024)
    {
        sprintf(acPrintBufDataRecved, 
                "%3.3f GB", 
                (FLOAT)g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera / (FLOAT)(1024 * 1024 *1024)) ;
    }
    else if(g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera >= 1024 * 1024)
    {
        sprintf(acPrintBufDataRecved, 
                "%3.3f MB", 
                (FLOAT)g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera / (FLOAT)(1024 * 1024)) ;
    }
    else if(g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera >= 1024)
    {
        sprintf(acPrintBufDataRecved, 
                "%3.3f KB", 
                (FLOAT)g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera / 1024.0f) ;
    }
    else
    {
        sprintf(acPrintBufDataRecved, 
                "%llu B", 
                g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera) ;
    }

    printf("** CAM : ************************\n"
            "--      total frame count from camera   : %u            \n"
            "--      send to ENC thread frame count  : %u            \n"
            "--      encode rate (only soft encode)  : %2.2f%%       \n"
            "--      total fps                       : %3.2f fps     \n" 
            "--      current fps                     : %3.2f fps     \n"
            "--      total bps                       : %s            \n" 
            "--      current bps                     : %s            \n"
            "--      total data received             : %s            \n",
            g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera,
            g_stCAMWorkarea.stStatistics.uiFrameCountToEncoder,
            g_stCAMWorkarea.stStatistics.fSoftEncodeRate,
            g_stCAMWorkarea.stStatistics.fTotalFPS,
            g_stCAMWorkarea.stStatistics.fCurrentFPS,
            acPrintBufTotalBPS,
            acPrintBufCurrentBPS,
            acPrintBufDataRecved) ;

    return ;
}

#if 1 != RCE_USE_HARDWARE_ENCODER
/*******************************************************************************
- Function    : __RCE_DPS_DumpENCStatistics
- Description : 本函数在终端中显示ENC线程的运行状态和统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_DPS_DumpENCStatistics(VOID)
{
    CHAR acPrintBufDataCountEncoded[80] ;
    CHAR acPrintBufDataCountGenerated[80] ;
    CHAR acPrintBufTotalEncodeBPS[80] ;
    CHAR acPrintBufCurrentEncodeBPS[80] ;

    /* 整理累计被编码数据量信息 */
    if(g_stENCWorkarea.stStatistics.ui64DataCountEncoded >= 1000000000)
    {
        sprintf(acPrintBufDataCountEncoded, 
                "%3.3f GB", 
                (FLOAT)g_stENCWorkarea.stStatistics.ui64DataCountEncoded / 1000000000.0f) ;
    }
    else if(g_stENCWorkarea.stStatistics.ui64DataCountEncoded >= 1000000)
    {
        sprintf(acPrintBufDataCountEncoded, 
                "%3.3f MB", 
                (FLOAT)g_stENCWorkarea.stStatistics.ui64DataCountEncoded / 1000000.0f) ;
    }
    else if(g_stENCWorkarea.stStatistics.ui64DataCountEncoded >= 1000)
    {
        sprintf(acPrintBufDataCountEncoded, 
                "%3.3f KB", 
                (FLOAT)g_stENCWorkarea.stStatistics.ui64DataCountEncoded / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufDataCountEncoded, 
                "%llu B", 
                g_stENCWorkarea.stStatistics.ui64DataCountEncoded) ;
    }

    /* 整理累计编码生成数据量信息 */
    if(g_stENCWorkarea.stStatistics.ui64DataCountGenerated >= 1000000000)
    {
        sprintf(acPrintBufDataCountGenerated, 
                "%3.3f GB", 
                (FLOAT)g_stENCWorkarea.stStatistics.ui64DataCountGenerated / 1000000000.0f) ;
    }
    else if(g_stENCWorkarea.stStatistics.ui64DataCountGenerated >= 1000000)
    {
        sprintf(acPrintBufDataCountGenerated, 
                "%3.3f MB", 
                (FLOAT)g_stENCWorkarea.stStatistics.ui64DataCountGenerated / 1000000.0f) ;
    }
    else if(g_stENCWorkarea.stStatistics.ui64DataCountGenerated >= 1000)
    {
        sprintf(acPrintBufDataCountGenerated, 
                "%3.3f KB", 
                (FLOAT)g_stENCWorkarea.stStatistics.ui64DataCountGenerated / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufDataCountGenerated, 
                "%llu B", 
                g_stENCWorkarea.stStatistics.ui64DataCountGenerated) ;
    }

    /* 整理累计编码bps信息 */
    if(g_stENCWorkarea.stStatistics.uiTotalEncodeBPS >= 1000000)
    {
        sprintf(acPrintBufTotalEncodeBPS, 
                "%3.3f Mbps", 
                (FLOAT)g_stENCWorkarea.stStatistics.uiTotalEncodeBPS / 1000000.0f) ;
    }
    else if(g_stENCWorkarea.stStatistics.uiTotalEncodeBPS >= 1000)
    {
        sprintf(acPrintBufTotalEncodeBPS, 
                "%3.3f Kbps", 
                (FLOAT)g_stENCWorkarea.stStatistics.uiTotalEncodeBPS / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufTotalEncodeBPS, 
                "%u bps", 
                g_stENCWorkarea.stStatistics.uiTotalEncodeBPS) ;
    }

    /* 整理实时编码bps信息 */
    if(g_stENCWorkarea.stStatistics.uiCurrentEncodeBPS >= 1000000)
    {
        sprintf(acPrintBufCurrentEncodeBPS, 
                "%3.3f Mbps", 
                (FLOAT)g_stENCWorkarea.stStatistics.uiCurrentEncodeBPS / 1000000.0f) ;
    }
    else if(g_stENCWorkarea.stStatistics.uiCurrentEncodeBPS >= 1000)
    {
        sprintf(acPrintBufCurrentEncodeBPS, 
                "%3.3f Kbps", 
                (FLOAT)g_stENCWorkarea.stStatistics.uiCurrentEncodeBPS / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufCurrentEncodeBPS, 
                "%u bps", 
                g_stENCWorkarea.stStatistics.uiCurrentEncodeBPS) ;
    }

    printf("** ENC : ************************\n"
            "--      total frame count encoded       : %u            \n"
            "--      total data count encoded        : %s            \n"
            "--      total data generated by encoder : %s            \n"
            "--      total encode fps                : %3.2f fps     \n" 
            "--      current encode fps              : %3.2f fps     \n"
            "--      total generate bps              : %s            \n" 
            "--      current generate bps            : %s            \n",
            g_stENCWorkarea.stStatistics.uiFrameCountEncoded,
            acPrintBufDataCountEncoded,
            acPrintBufDataCountGenerated,
            g_stENCWorkarea.stStatistics.fTotalEncodeFPS,
            g_stENCWorkarea.stStatistics.fCurrentEncodeFPS,
            acPrintBufTotalEncodeBPS,
            acPrintBufCurrentEncodeBPS) ;
}
#endif

/*******************************************************************************
- Function    : __RCE_DPS_DumpDTRStatistics
- Description : 本函数在终端中显示DTR线程的运行状态和统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_DPS_DumpDTRStatistics(VOID)
{
    CHAR acPrintBufDataCountFromFIFO[80] ;
    CHAR acPrintBufDataCountToSocket[80] ;
    CHAR acPrintBufBPSFromFIFO[80] ;
    CHAR acPrintBufBPSToSocekt[80] ;

    /* 整理DTR线程从FIFO中获取到的数据量信息 */
    if(g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO >= 1024 * 1024 * 1024)
    {
        sprintf(acPrintBufDataCountFromFIFO, 
                "%3.3f GB", 
                (FLOAT)g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO / (FLOAT)(1024 * 1024 * 1024)) ;
    }
    else if(g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO >= 1024 * 1024)
    {
        sprintf(acPrintBufDataCountFromFIFO, 
                "%3.3f MB", 
                (FLOAT)g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO / (FLOAT)(1024 * 1024)) ;
    }
    else if(g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO >= 1024)
    {
        sprintf(acPrintBufDataCountFromFIFO, 
                "%3.3f KB", 
                (FLOAT)g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO / 1024.0f) ;
    }
    else
    {
        sprintf(acPrintBufDataCountFromFIFO, 
                "%llu B", 
                g_stDTRWorkarea.stStatistics.ui64DataCountFromFIFO) ;
    }

    /* 整理DTR线程发送到Socket的数据量信息 */
    if(g_stDTRWorkarea.stStatistics.ui64DataCountToSocket >= 1024 * 1024 * 1024)
    {
        sprintf(acPrintBufDataCountToSocket, 
                "%3.3f GB", 
                (FLOAT)g_stDTRWorkarea.stStatistics.ui64DataCountToSocket / (FLOAT)(1024 * 1024 * 1024)) ;
    }
    else if(g_stDTRWorkarea.stStatistics.ui64DataCountToSocket >= 1024 * 1024)
    {
        sprintf(acPrintBufDataCountToSocket, 
                "%3.3f MB", 
                (FLOAT)g_stDTRWorkarea.stStatistics.ui64DataCountToSocket / (FLOAT)(1024 * 1024)) ;
    }
    else if(g_stDTRWorkarea.stStatistics.ui64DataCountToSocket >= 1024)
    {
        sprintf(acPrintBufDataCountToSocket, 
                "%3.3f KB", 
                (FLOAT)g_stDTRWorkarea.stStatistics.ui64DataCountToSocket / 1024.0f) ;
    }
    else
    {
        sprintf(acPrintBufDataCountToSocket, 
                "%llu B", 
                g_stDTRWorkarea.stStatistics.ui64DataCountToSocket) ;
    }

    /* 整理DTR线程从FIFO中获取到的数据速率信息 */
    if(g_stDTRWorkarea.stStatistics.uiCurrentBPSFromFIFO >= 1000000)
    {
        sprintf(acPrintBufBPSFromFIFO, 
                "%3.3f Mbps", 
                (FLOAT)g_stDTRWorkarea.stStatistics.uiCurrentBPSFromFIFO / 1000000.0f) ;
    }
    else if(g_stDTRWorkarea.stStatistics.uiCurrentBPSFromFIFO >= 1000)
    {
        sprintf(acPrintBufBPSFromFIFO, 
                "%3.3f Kbps", 
                (FLOAT)g_stDTRWorkarea.stStatistics.uiCurrentBPSFromFIFO / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufBPSFromFIFO, 
                "%u bps", 
                g_stDTRWorkarea.stStatistics.uiCurrentBPSFromFIFO) ;
    }

    /* 整理DTR线程发送到Socket的数据速率信息 */
    if(g_stDTRWorkarea.stStatistics.uiCurrentBPSToSocket >= 1000000)
    {
        sprintf(acPrintBufBPSToSocekt, 
                "%3.3f Mbps", 
                (FLOAT)g_stDTRWorkarea.stStatistics.uiCurrentBPSToSocket / 1000000.0f) ;
    }
    else if(g_stDTRWorkarea.stStatistics.uiCurrentBPSToSocket >= 1000)
    {
        sprintf(acPrintBufBPSToSocekt, 
                "%3.3f Kbps", 
                (FLOAT)g_stDTRWorkarea.stStatistics.uiCurrentBPSToSocket / 1000.0f) ;
    }
    else
    {
        sprintf(acPrintBufBPSToSocekt, 
                "%u bps", 
                g_stDTRWorkarea.stStatistics.uiCurrentBPSToSocket) ;
    }

    printf("** DTR : ************************\n"
            "--      total data received from fifo   : %s            \n"
            "--      total data send to socket       : %s            \n"
            "--      current bps from fifo           : %s            \n" 
            "--      current bps to socket           : %s            \n",
            acPrintBufDataCountFromFIFO,
            acPrintBufDataCountToSocket,
            acPrintBufBPSFromFIFO,
            acPrintBufBPSToSocekt) ;

    return ;
}

/*******************************************************************************
- Function    : __RCE_DumpStatistics
- Description : 本函数在终端中显示运行状态。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_DumpStatistics(VOID)
{
    STATIC UINT uiScreanClearred = 0 ;


    if(0 == uiScreanClearred)
    {
        printf("\033[2J") ;    /* 清除屏幕 */
        printf("\033[1;1H") ;  /* 光标移动到左上角 */
        fflush(stdout) ; 
        uiScreanClearred = 1 ;
    }

    printf("---------------------------------------------------------------------------------------------------\n") ;

    /* 打印CAM线程信息 */
    if(1)
    {
        /* 打印CAM线程统计信息 */
        __RCE_DPS_DumpCAMStatistics() ;

#if 1 != RCE_USE_HARDWARE_ENCODER
        /* 打印ENC线程信息 */
        __RCE_DPS_DumpENCStatistics() ;
#endif
        /* 打印DTR线程统计信息 */
        __RCE_DPS_DumpDTRStatistics() ;
    }

    printf("---------------------------------------------------------------------------------------------------\n") ;

    printf("\033[1;1H") ; /* 光标移动到左上角 */
    fflush(stdout);

    return ;
}

/*******************************************************************************
- Function    : __RCE_SignalHandler
- Description : 本函数响应用户SIGINT信号，并退出程序。
- Input       : iSignum
                    必定收到SIGINT信号。
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_SignalHandler(INT iSignum)
{
    if(SIGINT == iSignum)
    {
        /* 停止打印，并将光标放到尾部 */
        uiLogStatistics = 0 ;
        printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n") ;

        __RCE_StopThreadDTR() ;
#if 1 != RCE_USE_HARDWARE_ENCODER
        __RCE_StopThreadENC() ;
#endif
        __RCE_StopThreadCAM() ;

        exit(0) ;
    }

    if(SIGALRM == iSignum)
    {
        /* 通知其他线程更新统计信息 */
        g_stCAMWorkarea.uiUpdateStatistics = 1 ;
#if 1 != RCE_USE_HARDWARE_ENCODER
        g_stENCWorkarea.uiUpdateStatistics = 1 ;
#endif
        g_stDTRWorkarea.uiUpdateStatistics = 1 ;

        /* 打印 */
        uiLogStatistics = 1 ;

        return ;
    }
}

CODE_SECTION("==========================") ;
CODE_SECTION("==  主函数              ==") ;
CODE_SECTION("==========================") ;

INT main(VOID)
{
    struct itimerval stTimerSets ;

    /* 响应用户SIGINT信号 */
    signal(SIGINT, __RCE_SignalHandler) ;
    signal(SIGALRM, __RCE_SignalHandler) ;

    /* 创建CAM线程 */
    pthread_create(&g_tidCAM, NULL, RCE_CAM_Thread, NULL) ;
#if 1 != RCE_USE_HARDWARE_ENCODER
    pthread_create(&g_tidENC, NULL, RCE_ENC_Thread, NULL) ;
#endif
    pthread_create(&g_tidDTR, NULL, RCE_DTR_Thread, NULL) ;

    /* 等待一段时间，让子线程打印完毕 */
    usleep(500000) ;

    /* 创建定时器 */
    stTimerSets.it_value.tv_sec     = 0 ;
    stTimerSets.it_value.tv_usec    = RCE_STATISTIC_PERIOD_USEC ;

    stTimerSets.it_interval.tv_sec  = 0 ;
    stTimerSets.it_interval.tv_usec = RCE_STATISTIC_PERIOD_USEC ;

    setitimer(ITIMER_REAL, &stTimerSets, NULL) ;

    while(1)
    {

        if(1 == uiLogStatistics)
        {
            __RCE_DumpStatistics() ;

            uiLogStatistics = 0 ;
        }

        usleep(10000) ;
    }

    return 0 ;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/******* End of file RCE.c. *******/  
