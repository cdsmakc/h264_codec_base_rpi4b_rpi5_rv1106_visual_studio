/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE.c
    Project name : RV1106 Camera Encode with h264
    Module name  : RV1106 Camera Encode with h264
    Date created : 2025年6月6日   17时53分32秒
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
#include <sys/time.h>

/* 第三方 */
#include "./iniparser-4.1/src/iniparser.h"

/* 内部 */
#include "RCE_config.h"
#include "RCE_CAM.h"
#include "RCE_ENC.h"
#include "RCE_DTR.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块全局变量        ==") ;
CODE_SECTION("==========================") ;

/* 模块工作区 */
extern RCE_CAM_WORKAREA_S  g_stCAMWorkarea ;
extern RCE_ENC_WORKAREA_S  g_stENCWorkarea ;
extern RCE_DTR_WORKAREA_S  g_stDTRWorkarea ;

/* 标记 */
UINT                       uiLogStatistics = 1 ;
UINT                       uiExit          = 0 ;

/* 线程ID */
pthread_t                  g_tidCAM ;
pthread_t                  g_tidDTR ;
pthread_t                  g_tidENC ;

/* 配置信息 */
RCE_CONFIG_S               g_stRCEConfig ;

/* CAM至ENC的帧缓存 */
extern UCHAR              *g_pucFrameBuffer ;
extern UINT                g_uiFrameBufferValid ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  日志颜色定义        ==") ;
CODE_SECTION("==========================") ;

#define RCE_LOG_FOREGROUND_COLOR_BLACK           ("\033[30m")
#define RCE_LOG_FOREGROUND_COLOR_RED             ("\033[31m")
#define RCE_LOG_FOREGROUND_COLOR_GREEN           ("\033[32m")
#define RCE_LOG_FOREGROUND_COLOR_YELLOW          ("\033[33m")
#define RCE_LOG_FOREGROUND_COLOR_BLUE            ("\033[34m")
#define RCE_LOG_FOREGROUND_COLOR_MAGENTA         ("\033[35m")
#define RCE_LOG_FOREGROUND_COLOR_CYAN            ("\033[36m")
#define RCE_LOG_FOREGROUND_COLOR_WHITE           ("\033[37m")

#define RCE_LOG_BACKGROUND_COLOR_BLACK           ("\033[40m")
#define RCE_LOG_BACKGROUND_COLOR_RED             ("\033[41m")
#define RCE_LOG_BACKGROUND_COLOR_GREEN           ("\033[42m")
#define RCE_LOG_BACKGROUND_COLOR_YELLOW          ("\033[43m")
#define RCE_LOG_BACKGROUND_COLOR_BLUE            ("\033[44m")
#define RCE_LOG_BACKGROUND_COLOR_MAGENTA         ("\033[45m")
#define RCE_LOG_BACKGROUND_COLOR_CYAN            ("\033[46m")
#define RCE_LOG_BACKGROUND_COLOR_WHITE           ("\033[47m")

#define RCE_LOG_BACKGROUND_COLOR_STOP            ("\033[0m")


CODE_SECTION("==========================") ;
CODE_SECTION("==  内部函数            ==") ;
CODE_SECTION("==========================") ;

/*******************************************************************************
- Function    : __RCE_ParseConfig
- Description : 本函数解析配置文件，从而获取配置。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_ParseConfig(VOID)
{
    dictionary *pstDictionary ;

    /* 加载ini文件 */
    pstDictionary = iniparser_load(RCE_CONFIG_FILE) ;

    if(NULL == pstDictionary)
    {
        printf("File %s, func %s, line %d : Cannot load ini config file %s. \n", __FILE__, __func__, __LINE__, RCE_CONFIG_FILE) ;

        return -1 ;
    }

    /* 获取配置项 */
    g_stRCEConfig.usWidth      = iniparser_getint(pstDictionary, "VIDEO_CFG:video_cfg_frame_width", RCE_CAMERA_RESOLUTION_WIDTH_DEFAULT) ;
    g_stRCEConfig.usHeight     = iniparser_getint(pstDictionary, "VIDEO_CFG:video_cfg_frame_height", RCE_CAMERA_RESOLUTION_HEIGHT_DEFAULT) ;
    g_stRCEConfig.uiFps        = iniparser_getint(pstDictionary, "VIDEO_CFG:video_cfg_frame_fps", RCE_CAMERA_FPS_DEFAULT) ;

    g_stRCEConfig.uiHWBitrate  = iniparser_getint(pstDictionary, "ENCODER_CFG:encoder_cfg_hw_bitrate", RCE_H264_HW_BITRATE) ;

    g_stRCEConfig.uiWrToFile   = iniparser_getint(pstDictionary, "OUTPUT_FILE_CFG:output_file_cfg_enable", 0) ;
    strcpy(g_stRCEConfig.acFile, iniparser_getstring(pstDictionary, "OUTPUT_FILE_CFG:output_file_name", RCE_H264_FILE_NAME_DEFAULT)) ;

    g_stRCEConfig.usLocalPort  = iniparser_getint(pstDictionary, "COMMUNICATION_CFG:comm_cfg_local_port", RCE_LOCAL_PORT_DEFAULT) ;
    g_stRCEConfig.usRemotePort = iniparser_getint(pstDictionary, "COMMUNICATION_CFG:comm_cfg_remote_port", RCE_REMOTE_PORT_DEFAULT) ;

    strcpy(g_stRCEConfig.acLocalIp, iniparser_getstring(pstDictionary, "COMMUNICATION_CFG:comm_cfg_local_ip", RCE_LOCAL_IP_DEFAULT)) ;
    strcpy(g_stRCEConfig.acRemoteIp, iniparser_getstring(pstDictionary, "COMMUNICATION_CFG:comm_cfg_remote_ip", RCE_REMOTE_IP_DEFAULT)) ;

    iniparser_freedict(pstDictionary);

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_DumpConfig
- Description : 本函数打印全局配置。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_DumpConfig(VOID)
{
    printf(RCE_LOG_FOREGROUND_COLOR_GREEN) ;
    printf("** CFG : ************************\n"
            "--      Video width                     : %u \n"
            "--      Video height                    : %u \n"
            "--      Video fps                       : %u \n"
            "--      Hardware encoder bitrate        : %u \n"
            "--      Write to file                   : %s \n"
            "--          File name :                 : %s \n" 
            "--      Local  ip                       : %s \n"
            "--      Local  port                     : %u \n" 
            "--      Remote ip                       : %s \n"
            "--      Remote port                     : %u \n",
            g_stRCEConfig.usWidth, 
            g_stRCEConfig.usHeight, 
            g_stRCEConfig.uiFps,
            g_stRCEConfig.uiHWBitrate,
            (0 == g_stRCEConfig.uiWrToFile) ? "No" : "Yes",
            g_stRCEConfig.acFile,
            g_stRCEConfig.acLocalIp,
            g_stRCEConfig.usLocalPort,
            g_stRCEConfig.acRemoteIp,
            g_stRCEConfig.usRemotePort) ;
    printf(RCE_LOG_BACKGROUND_COLOR_STOP) ;

    return ;
}

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

    printf(RCE_LOG_FOREGROUND_COLOR_RED) ;
    printf( "** CAM : ************************\n"
            "--      total frame count from camera   : %u            \n"
            "--      send to ENC thread frame count  : %u            \n"
            "--      encode rate                     : %2.2f%%       \n"
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
    printf(RCE_LOG_BACKGROUND_COLOR_STOP) ;

    return ;
}

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

    printf(RCE_LOG_FOREGROUND_COLOR_BLUE) ;
    printf( "** ENC : ************************\n"
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
    printf(RCE_LOG_BACKGROUND_COLOR_STOP) ;

    return ;
}

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

    printf(RCE_LOG_FOREGROUND_COLOR_CYAN) ;
    printf( "** DTR : ************************\n"
            "--      total data received from fifo   : %s            \n"
            "--      total data send to socket       : %s            \n"
            "--      current bps from fifo           : %s            \n" 
            "--      current bps to socket           : %s            \n",
            acPrintBufDataCountFromFIFO,
            acPrintBufDataCountToSocket,
            acPrintBufBPSFromFIFO,
            acPrintBufBPSToSocekt) ;
    printf(RCE_LOG_BACKGROUND_COLOR_STOP) ;

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
        /* 打印配置信息 */
        __RCE_DumpConfig() ;

        /* 打印CAM线程统计信息 */
        __RCE_DPS_DumpCAMStatistics() ;

        /* 打印ENC线程信息 */
        __RCE_DPS_DumpENCStatistics() ;

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
        uiExit = 1 ;
    }

    if(SIGALRM == iSignum)
    {
        /* 通知其他线程更新统计信息 */
        g_stCAMWorkarea.uiUpdateStatistics = 1 ;
        g_stENCWorkarea.uiUpdateStatistics = 1 ;
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

    /* 获取配置参数 */
    if(0 != __RCE_ParseConfig())
    {
        return -1 ;
    }

    /* 申请CAM到ENC的缓存块 */
    g_pucFrameBuffer = malloc(g_stRCEConfig.usWidth * g_stRCEConfig.usHeight * 3 /2) ;

    if(NULL == g_pucFrameBuffer)
    {
        printf("File %s, func %s, line %d : Malloc frame buffer from CAM to ENC failed. \n", __FILE__, __func__, __LINE__) ;
        return -2 ;
    }

    /* 创建CAM线程 */
    pthread_create(&g_tidCAM, NULL, RCE_CAM_Thread, NULL) ;
    pthread_create(&g_tidENC, NULL, RCE_ENC_Thread, NULL) ;
    pthread_create(&g_tidDTR, NULL, RCE_DTR_Thread, NULL) ;

    /* 等待一段时间，让子线程打印完毕 */
    usleep(2000000) ;

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

        if(1 == uiExit)
        {
            /* 停止打印，并将光标放到尾部 */
            //uiLogStatistics = 0 ;

            /* 等待正在进行中的打印完成 */
            usleep(50000) ;

            printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n") ;

            __RCE_StopThreadCAM() ;
            __RCE_StopThreadENC() ;
            __RCE_StopThreadDTR() ;

            break ;
        }

        usleep(10000) ;
    }

    free(g_pucFrameBuffer) ;

    return 0 ;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/******* End of file RCE.c. *******/  
