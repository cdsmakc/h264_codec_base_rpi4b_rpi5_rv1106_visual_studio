/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_CAM.c
    Project name : RV1106 Camera Encode with h264
    Module name  : Camera
    Date created : 2025年6月5日   19时17分46秒
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
#include <linux/fb.h>
#include <linux/videodev2.h>
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
#include <signal.h>

/* 第三方 */
#include "rk_aiq_comm.h"
#include "rk_aiq_user_api2_sysctl.h"

/* 内部 */
#include "RCE_types.h"
#include "RCE_config.h"
#include "RCE_CAM.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块全局变量        ==") ;
CODE_SECTION("==========================") ;

/* 模块工作区 */
RCE_CAM_WORKAREA_S          g_stCAMWorkarea ;

/* video设备缓存区 */
STATIC VOID                *g_apvBufferAddr[RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM] ; /* 8 x RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM Byte */
STATIC UINT                 g_auiBufferSize[RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM] ; /* 4 x RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM Byte */

/* 待编码缓存区 */
UCHAR                      *g_pucFrameBuffer ;
UINT                        g_uiFrameBufferValid = 0 ;

/* ISP */
STATIC rk_aiq_static_info_t g_stAiqStaticInfo ;
STATIC rk_aiq_sys_ctx_t    *g_pstAiqCtx ;

/* 配置信息 */
extern RCE_CONFIG_S         g_stRCEConfig ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块内部函数        ==") ;
CODE_SECTION("==========================") ;

/*******************************************************************************
- Function    : __RCE_CAM_StartIsp
- Description : 本函数初始化ISP，并启动ISP。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_StartIsp(VOID)
{
    INT iRetVal ;

    /* 获取静态的摄像头信息 */
	rk_aiq_uapi2_sysctl_enumStaticMetasByPhyId(RCE_CAM_CAMERA_ID, &g_stAiqStaticInfo) ;

    /* 配置buf数量。和后面的V4L2申请buf数量是否有关？ */
    rk_aiq_uapi2_sysctl_preInit_devBufCnt(g_stAiqStaticInfo.sensor_info.sensor_name, "rkraw_rx", RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM) ;

    /* 配置isp */
    g_pstAiqCtx = rk_aiq_uapi2_sysctl_init(g_stAiqStaticInfo.sensor_info.sensor_name, RCE_CAM_CAMERA_IQ_FILE_PATH, NULL, NULL) ;

    /* 准备AIQ运行上下文 */
    iRetVal = rk_aiq_uapi2_sysctl_prepare(g_pstAiqCtx, 0, 0, RK_AIQ_WORKING_MODE_NORMAL) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : AIQ prepare failed. Retval is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;

        return -1 ;
    }

    /* 启动AIQ */
    iRetVal = rk_aiq_uapi2_sysctl_start(g_pstAiqCtx) ;

    if(0 != iRetVal)
    {
        printf("File %s, func %s, line %d : AIQ start failed. Retval is %d.\n", __FILE__, __func__, __LINE__, iRetVal) ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_CAM_StopIsp
- Description : 本函数停止ISP的工作，并撤销ISP的初始化。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_CAM_StopIsp(VOID)
{
    /* 停止ISP */
    rk_aiq_uapi2_sysctl_stop(g_pstAiqCtx, false) ;

    /* 撤销初始化 */
    rk_aiq_uapi2_sysctl_deinit(g_pstAiqCtx) ;

    g_pstAiqCtx = NULL ;

    return ;
}

/*******************************************************************************
- Function    : __RCE_CAM_OpenCamera
- Description : 本函数打开摄像头设备。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_OpenCamera(VOID)
{
    g_stCAMWorkarea.iCameraFile = open(RCE_CAM_CAMERA_DEVICE, O_RDWR | O_NONBLOCK, 0) ;
    
    if(-1 == g_stCAMWorkarea.iCameraFile)
    {
        printf("File %s, func %s, line %d : Can not open %s.\n", __FILE__, __func__, __LINE__, RCE_CAM_CAMERA_DEVICE) ;

        return -1;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_CloseCamera
- Description : 本函数关闭摄像头设备。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
- Others      :
*******************************************************************************/
INT __RCE_CAM_CloseCamera(VOID)
{
    if(0 < g_stCAMWorkarea.iCameraFile)
    {
        close(g_stCAMWorkarea.iCameraFile) ;
    }
    
    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_GCP_QueryCameraCapability
- Description : 本函数查询摄像头能力。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_GCP_QueryCameraCapability(VOID)
{
    struct v4l2_capability  stCap ;
    INT                     iRetVal ;

    /* 获取摄像头能力 */
    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_QUERYCAP, &stCap) ;

    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_QUERYCAP error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;
        
        return -1 ;
    }
    else
    {
        /* 打印摄像头能力 */
        printf("--Camera capability --------------------------------------\n") ;
        printf("-- driver         : %s \n", stCap.driver) ;
        printf("-- card           : %s \n", stCap.card) ;
        printf("-- bus info       : %s \n", stCap.bus_info) ;
        printf("-- version        : %u.%u.%u \n",  (stCap.version >> 16) & 0xFF, (stCap.version >> 8) & 0xFF, (stCap.version) & 0xFF) ;
        printf("-- capabilities   : 0x%08X \n",  stCap.capabilities) ;
        printf("-- device_caps    : 0x%08X \n",  stCap.device_caps) ;
        printf("----------------------------------------------------------\n\n\n") ;

        return 0 ;
    }
}


/*******************************************************************************
- Function    : __RCE_CAM_GCP_QueryCameraFormat
- Description : 本函数查询摄像头支持的输出格式。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0 : success.
- Others      :
*******************************************************************************/
INT __RCE_CAM_GCP_QueryCameraFormat(VOID)
{
    struct v4l2_fmtdesc stFormatDesc ;
    CHAR                acPrintBuf[64] ;
    INT                 iRetVal ;

    /* 获取并打印摄像头支持的格式 */
    stFormatDesc.index = 0 ;
    stFormatDesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ;

    printf("--Format descriptor --------------------------------------\n") ;
    
    while(1)
    {
        iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_ENUM_FMT, &stFormatDesc) ;

        if(-1 == iRetVal)
        {
            break ;
        }
        else
        {
            memset(acPrintBuf, ' ', sizeof(acPrintBuf)) ;
            
            iRetVal = sprintf(acPrintBuf,      
                              "-- %02d. %s", 
                              stFormatDesc.index + 1, 
                              stFormatDesc.description) ;
    
            acPrintBuf[iRetVal] = ' ' ;
            
            sprintf(acPrintBuf + 32, "(%c%c%c%c)\n",  
                    (stFormatDesc.pixelformat >> 0) & 0xff, 
                    (stFormatDesc.pixelformat >> 8) & 0xff, 
                    (stFormatDesc.pixelformat >> 16) & 0xff, 
                    (stFormatDesc.pixelformat >> 24) & 0xff) ;
            
            printf("%s", acPrintBuf) ;
            
            stFormatDesc.index++ ;
        }
    }
    
    printf("----------------------------------------------------------\n\n\n") ;

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_GCP_QueryCameraResolution
- Description : 本函数查询摄像头支持的分辨率。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0 : success.
- Others      :
*******************************************************************************/
INT __RCE_CAM_GCP_QueryCameraResolution(VOID)
{
    struct v4l2_frmsizeenum stFrameSize ;
    INT                     iRetVal ;

    /* 获取并打印摄像头支持的分辨率*/
    stFrameSize.index        = 0 ;
    stFrameSize.pixel_format = V4L2_PIX_FMT_NV12 ;
    
    printf("--Frame resolution descriptor ----------------------------\n") ;
    
    while(1)
    {
        iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_ENUM_FRAMESIZES, &stFrameSize) ;

        if(-1 == iRetVal)
        {
            break ;
        }
        else
        {
            printf("-- %d. %c%c%c%c ", 
                   stFrameSize.index + 1, 
                   (stFrameSize.pixel_format >>  0) & 0xff, 
                   (stFrameSize.pixel_format >>  8) & 0xff, 
                   (stFrameSize.pixel_format >> 16) & 0xff, 
                   (stFrameSize.pixel_format >> 24) & 0xff) ;

            if(V4L2_FRMSIZE_TYPE_DISCRETE == stFrameSize.type)
            {
                printf("type : DISCRETE\n\t\t width : %d\n\t\t height : %d\n", stFrameSize.discrete.width, stFrameSize.discrete.height) ;
            }
            else if(V4L2_FRMSIZE_TYPE_CONTINUOUS == stFrameSize.type)
            {
                printf("type : CONTINUOUS\n") ;
            }
            else if(V4L2_FRMSIZE_TYPE_STEPWISE == stFrameSize.type)
            {
                printf("type : STEPWISE\n"
                       "\t\t min_width   : %d\n"
                       "\t\t max_width   : %d\n"
                       "\t\t step_width  : %d\n"
                       "\t\t min_height  : %d\n"
                       "\t\t max_height  : %d\n"
                       "\t\t step_height : %d\n", 
                       stFrameSize.stepwise.min_width,
                       stFrameSize.stepwise.max_width,
                       stFrameSize.stepwise.step_width,
                       stFrameSize.stepwise.min_height,
                       stFrameSize.stepwise.max_height,
                       stFrameSize.stepwise.step_height) ;
            }

            stFrameSize.index++ ;
        }
    }
    
    printf("----------------------------------------------------------\n\n\n") ;

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_GetCameraParam
- Description : 本函数查询摄像头特征、支持格式和分辨率。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_GetCameraParam(VOID)
{    
    /* 获取摄像头能力 */
    if(0 != __RCE_CAM_GCP_QueryCameraCapability())
    {
        return -1 ;
    }

    /* 获取摄像头支持的输出格式 */
    __RCE_CAM_GCP_QueryCameraFormat() ;

    /* 获取摄像头支持的分辨率 */
    __RCE_CAM_GCP_QueryCameraResolution() ;

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_SCP_SetCameraResolution
- Description : 本函数设置摄像头输出分辨率。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_SCP_SetCameraResolution(VOID)
{
    struct v4l2_format     stFormat ;
    INT                    iRetVal ;

    memset(&stFormat, 0, sizeof(stFormat)) ;

    /* 设置输出格式 */
    stFormat.type                   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ;
    stFormat.fmt.pix_mp.width       = g_stRCEConfig.usWidth ;
    stFormat.fmt.pix_mp.height      = g_stRCEConfig.usHeight ;
    stFormat.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12 ;
    stFormat.fmt.pix_mp.num_planes  = 1 ;
    
    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_S_FMT, &stFormat) ;

    if(-1 == iRetVal) 
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_S_FMT error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }

    /* 检查设置效果 */
    stFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ;

    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_G_FMT, &stFormat) ;
    
    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_G_FMT error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;
        
        return -1 ;
    }
    else
    {
        printf("--Set format result---------------------------------------\n") ;
        printf("-- Resolution width  : %d\n"
               "-- Resolution height : %d\n"
               "-- Pixel format      : %c%c%c%c\n"
               "-- Num or planes     : %d\n",
               stFormat.fmt.pix_mp.width, stFormat.fmt.pix_mp.height,
               stFormat.fmt.pix_mp.pixelformat & 0xFF,
               (stFormat.fmt.pix_mp.pixelformat >> 8) & 0xFF,
               (stFormat.fmt.pix_mp.pixelformat >> 16) & 0xFF,
               (stFormat.fmt.pix_mp.pixelformat >> 24) & 0xFF,
               stFormat.fmt.pix_mp.num_planes);
        printf("----------------------------------------------------------\n\n\n") ;
        
        return 0 ;
    }
}


/*******************************************************************************
- Function    : __RCE_CAM_SCP_SetCameraFPS
- Description : 本函数设置摄像头输出帧率。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_SCP_SetCameraFPS(VOID)
{

    struct v4l2_streamparm stStreamParm ;
    INT                    iRetVal ;

    stStreamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ;
#if 0 
    /* 获取当前设置 */
    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_G_PARM, &stStreamParm) ;

    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_G_PARM error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }
    else
    {
        printf("--Previous camera stream param----------------------------\n") ;
        printf("-- Capability        : %08x\n"
               "-- Capturemode       : %08x\n"
               "-- Numerator         : %d\n"
               "-- Denominator       : %d\n",
               stStreamParm.parm.capture.capability, 
               stStreamParm.parm.capture.capturemode, 
               stStreamParm.parm.capture.timeperframe.numerator, 
               stStreamParm.parm.capture.timeperframe.denominator) ;
        printf("----------------------------------------------------------\n\n\n") ;
    }
#endif
#if 0
    /* 修改FPS */
    stStreamParm.parm.capture.timeperframe.numerator   = 1 ;
    stStreamParm.parm.capture.timeperframe.denominator = g_stRCEConfig.uiFps ;

    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_S_PARM, &stStreamParm) ;

    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_S_PARM error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }
#endif
#if 0
    /* 获取新设置 */
    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_G_PARM, &stStreamParm) ;

    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_G_PARM error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }
    else
    {
        printf("--Current camera stream param-----------------------------\n") ;
        printf("-- Capability        : %08x\n"
               "-- Capturemode       : %08x\n"
               "-- Numerator         : %d\n"
               "-- Denominator       : %d\n",
               stStreamParm.parm.capture.capability, 
               stStreamParm.parm.capture.capturemode, 
               stStreamParm.parm.capture.timeperframe.numerator, 
               stStreamParm.parm.capture.timeperframe.denominator) ;
        printf("----------------------------------------------------------\n\n\n") ;
    }    
#endif
    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_SetCameraParam
- Description : 本函数设置摄像头输出参数。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_SetCameraParam(VOID)
{
    /* 设置摄像头分辨率 */
    if(0 != __RCE_CAM_SCP_SetCameraResolution())
    {
        return -1 ;
    }

    /* 设置摄像头帧率 */
    if(0 != __RCE_CAM_SCP_SetCameraFPS())
    {
        return -1 ;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_RequestBuffers
- Description : 本函数申请缓存。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_RequestBuffers(VOID)
{
    struct v4l2_requestbuffers stReqBuf ;
    INT                        iRetVal ;

    memset(&stReqBuf, 0, sizeof(stReqBuf)) ;

    /* 申请缓冲区 */
    stReqBuf.count  = RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM ;
    stReqBuf.memory = V4L2_MEMORY_MMAP ;
    stReqBuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ;

    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_REQBUFS, &stReqBuf) ;
    printf(" iRetVal is %d count is %d, stReqBuf.memory is %d type is %d\n", iRetVal, stReqBuf.count, stReqBuf.memory, stReqBuf.type);
    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_REQBUFS error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }
    else if(RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM != stReqBuf.count)
    {
        printf("File %s, func %s, line %d : Request buffer count is not enough.\n", __FILE__, __func__, __LINE__) ;

        return -1 ;
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_MapBuffers
- Description : 本函数将摄像头设备的缓存映射到用户空间。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
#define planes_num  (1u)
INT __RCE_CAM_MapBuffers(VOID)
{
    struct v4l2_buffer         stBuf ;
    struct v4l2_plane          astPlanes[planes_num] ;
    INT                        iRetVal ;
    INT                        iLoopBuf ;
    INT                        iLoopMappedBuf ;

    /* 将申请的缓冲区映射到用户空间 */
    for(iLoopBuf = 0 ; iLoopBuf < RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM ; iLoopBuf++)
    {
        memset(&stBuf, 0, sizeof(stBuf)) ;
        memset(astPlanes, 0, sizeof(struct v4l2_plane) * planes_num) ;

        stBuf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ;
        stBuf.index    = iLoopBuf ;
        stBuf.memory   = V4L2_MEMORY_MMAP ;
        stBuf.length   = planes_num ;
        stBuf.m.planes = astPlanes ;

        iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_QUERYBUF, &stBuf) ;
        
        if(-1 == iRetVal)
        {
            printf("File %s, func %s, line %d : ioctl VIDIOC_QUERYBUF error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;
            
            return -1 ;
        }

        printf("stBuf.m.planes[0].length is %d\n", stBuf.m.planes[0].length) ;
        printf("astPlanes[0].length is %d \n", astPlanes[0].length) ;
        printf("astPlanes[0].m.mem_offset is %d \n", astPlanes[0].m.mem_offset) ;

        g_auiBufferSize[iLoopBuf] = stBuf.m.planes[0].length ;
        g_apvBufferAddr[iLoopBuf] = mmap(NULL,
                                         stBuf.m.planes[0].length,
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED,
                                         g_stCAMWorkarea.iCameraFile,
                                         stBuf.m.planes[0].m.mem_offset);
        
        if(MAP_FAILED == g_apvBufferAddr[iLoopBuf])
        {
            printf("File %s, func %s, line %d : mmap error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

            /* 对于已经mmap成功的，要munmap */
            for(iLoopMappedBuf = 0 ; iLoopMappedBuf < iLoopBuf ; iLoopMappedBuf++)
            {
                munmap(g_apvBufferAddr[iLoopMappedBuf], g_auiBufferSize[iLoopMappedBuf]) ;
            }

            return -1 ;
        }

        /* 将所有缓存置入队列 */
        iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_QBUF, &stBuf) ;

        if(-1 == iRetVal)
        {
            printf("File %s, func %s, line %d : ioctl VIDIOC_QBUF error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

            /* 对于已经mmap成功的，要munmap */
            for(iLoopMappedBuf = 0 ; iLoopMappedBuf < iLoopBuf ; iLoopMappedBuf++)
            {
                munmap(g_apvBufferAddr[iLoopMappedBuf], g_auiBufferSize[iLoopMappedBuf]) ;
            }

            return -1 ;
        }
    }

    printf("--Buffer information---------------------------------------\n") ;
    
    for(iLoopBuf = 0 ; iLoopBuf < RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM ; iLoopBuf++)
    {
        printf("-- Frame buffer :%d   address :0x%08X    size:%d\n", 
               iLoopBuf, 
               (UINT)g_apvBufferAddr[iLoopBuf], 
               g_auiBufferSize[iLoopBuf]) ;
    }
    
    printf("-----------------------------------------------------------\n") ;

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_MapBuffers
- Description : 本函数将摄像头设备的缓存映射到用户空间。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
- Others      :
*******************************************************************************/
INT __RCE_CAM_UnmapBuffers(VOID)
{
    INT iLoopBuf ;
    
    for(iLoopBuf = 0 ; iLoopBuf < RCE_CAM_CAMERA_ALLOCATE_BUFFER_NUM ; iLoopBuf++) 
    {
        munmap(g_apvBufferAddr[iLoopBuf], g_auiBufferSize[iLoopBuf]) ;
    }
    
    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_StreamCmd
- Description : 本函数启动或停止摄像头数据流。
- Input       : enCmd
                    CAM_STREAM_COMMAND_START : 启动数据流。
                    CAM_STREAM_COMMAND_STOP  : 停止数据流。
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_StreamCmd(RCE_CAM_STREAM_CMD_E enCmd)
{
    enum v4l2_buf_type enType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ;
    INT                iRetVal ;

    if(CAM_STREAM_COMMAND_START == enCmd)
    {
        iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_STREAMON, &enType) ;

        if(0 != iRetVal)
        {
            printf("File %s, func %s, line %d : start stream error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;
            printf("retval is %d\n", iRetVal) ;
            return -1 ;
        }
    }

    if(CAM_STREAM_COMMAND_STOP == enCmd)
    {
        iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_STREAMOFF, &enType) ;

        if(0 != iRetVal)
        {
            printf("File %s, func %s, line %d : stop stream error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

            return -1 ;
        }
    }

    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_PCD_WaitFrame
- Description : 本函数等待一帧数据采集就绪。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_PCD_WaitFrame(VOID)
{
    struct timeval stTimeVal ;
    fd_set         stFDRead ;
    INT            iRetVal ;
    
    stTimeVal.tv_usec = 0 ;
    stTimeVal.tv_sec  = 2 ; 
    
    FD_ZERO(&stFDRead) ;
    FD_SET(g_stCAMWorkarea.iCameraFile, &stFDRead) ;

    iRetVal = select(g_stCAMWorkarea.iCameraFile + 1, &stFDRead, NULL, NULL, &stTimeVal) ;
    
    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : wait for frame error. ioctl : %s.\n", __FILE__, __func__, __LINE__, strerror(errno)) ;

        return -1 ;
    }
    else if(0 == iRetVal)
    {
        printf("File %s, func %s, line %d : wait for frame timeout. return 0.\n", __FILE__, __func__, __LINE__) ;
        
        return -1 ;
    }
    
    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_PCD_PrcessFrame
- Description : 本函数将就绪的图像缓存从队列中取出，然后通告编码模块编码。编码完
                成后，重新将缓存置入队列。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_PCD_PrcessFrame(VOID)
{
    struct v4l2_buffer stBuf;
    struct v4l2_plane  astPlanes[planes_num] ;
    INT                iRetVal ;

    memset(&stBuf, 0, sizeof(stBuf)) ;

    stBuf.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stBuf.memory   = V4L2_MEMORY_MMAP;
    stBuf.index    = 0 ;
    stBuf.m.planes = astPlanes;
    stBuf.length   = planes_num ;

    memset(astPlanes, 0, sizeof(struct v4l2_plane) * planes_num) ;

    /* 从队列中取出就绪缓存 */
    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_DQBUF, &stBuf) ;
        
    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_DQBUF error.\n", __FILE__, __func__, __LINE__) ;
        
        return -1 ;
    }
    else
    {
        /* 统计：更新接收帧数 */
        g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera++ ;

        /* 数据发送给ENC */
        if(0 == g_uiFrameBufferValid)
        {
            /* 复制图像 */
            memcpy(g_pucFrameBuffer, g_apvBufferAddr[stBuf.index], astPlanes[0].bytesused) ;

            /* 设置标记，等待编码完成 */
            g_uiFrameBufferValid = 1 ;

            /* 统计：更新发送到编码器的帧数 */
            g_stCAMWorkarea.stStatistics.uiFrameCountToEncoder++ ;
        }
        else
        {
            /* 上一帧未编码完成，丢弃这一帧 */
        }

        /* 统计：更新接受到的数据量 */
        g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera += astPlanes[0].bytesused ;
    }

    /* 编码完成，或者丢弃帧，将缓存重新置入队列 */
    iRetVal = ioctl(g_stCAMWorkarea.iCameraFile, VIDIOC_QBUF, &stBuf) ;

    if(-1 == iRetVal)
    {
        printf("File %s, func %s, line %d : ioctl VIDIOC_QBUF error.\n", __FILE__, __func__, __LINE__) ;
        
        return -1 ;
    }
    
    return 0 ;
}


/*******************************************************************************
- Function    : __RCE_CAM_ProcessCameraData
- Description : 本函数等待一帧图像数据，然后对图像数据编码。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : success.
                    -1 : fail.
- Others      :
*******************************************************************************/
INT __RCE_CAM_ProcessCameraData(VOID)
{
    /* 等待帧 */
    if(0 != __RCE_CAM_PCD_WaitFrame())
    {
        return -1 ;
    }

    /* 处理帧 */
    if(0 != __RCE_CAM_PCD_PrcessFrame())
    {
        return -1 ;
    }
    
    return 0 ;
}

/*******************************************************************************
- Function    : __RCE_CAM_UpdateStatistics
- Description : 本函数更新统计信息。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID __RCE_CAM_UpdateStatistics(VOID)
{
    STATIC UINT   uiPrevFrameCountFromCamera = 0 ;
    STATIC UINT64 ui64PrevDataCountFromCamera = 0 ;

    /* 定时器信号，计算统计信息 */
    g_stCAMWorkarea.stStatistics.uiRunTime += RCE_STATISTIC_PERIOD_USEC ;

    /* 计算累计FPS */
    if(0 != g_stCAMWorkarea.stStatistics.uiRunTime)
    {
        g_stCAMWorkarea.stStatistics.fTotalFPS = (FLOAT)g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera / 
                                                 ((FLOAT)g_stCAMWorkarea.stStatistics.uiRunTime / 1000000.0f) ;
    }

    /* 计算实时FPS */
    g_stCAMWorkarea.stStatistics.fCurrentFPS = (FLOAT)(g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera - uiPrevFrameCountFromCamera) / 
                                               ((FLOAT)RCE_STATISTIC_PERIOD_USEC / 1000000.0f) ;

    /* 计算累计bps */
    if(0 != g_stCAMWorkarea.stStatistics.uiRunTime)
    {
        g_stCAMWorkarea.stStatistics.uiTotalBPS = (UINT)((FLOAT)(g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera * 8) / 
                                                  ((FLOAT)g_stCAMWorkarea.stStatistics.uiRunTime / 1000000.0f)) ;
    }

    /* 计算实时bps */
    g_stCAMWorkarea.stStatistics.uiCurrentBPS = (FLOAT)((g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera - ui64PrevDataCountFromCamera) * 8) / 
                                                ((FLOAT)RCE_STATISTIC_PERIOD_USEC / 1000000.0f) ;

    /* 计算软编码实施比率 */
    if(0 != g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera)
    {
        g_stCAMWorkarea.stStatistics.fSoftEncodeRate = 100.0f * (FLOAT)g_stCAMWorkarea.stStatistics.uiFrameCountToEncoder / 
                                                       (FLOAT)g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera ;
    }
    else
    {
        g_stCAMWorkarea.stStatistics.fSoftEncodeRate =0.0f ;
    }

    /* 更新静态变量以便下一次计算 */
    uiPrevFrameCountFromCamera  = g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera ;
    ui64PrevDataCountFromCamera = g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera ;

    return ;
}

CODE_SECTION("==========================") ;
CODE_SECTION("==  线程入口            ==") ;
CODE_SECTION("==========================") ;
/*******************************************************************************
- Function    : RCE_CAM_Thread
- Description : 本函数为CAM模块的线程函数。
- Input       : VOID
- Output      : NULL
- Return      : VOID *
- Others      :
*******************************************************************************/
VOID *RCE_CAM_Thread(VOID *pvArgs)
{
    memset(&g_stCAMWorkarea, 0, sizeof(g_stCAMWorkarea)) ;

    /* 指示线程正在运行 */
    g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_RUN ;

    /* 启动ISP */
    if(0 != __RCE_CAM_StartIsp())
    {
        /* 启动ISP失败，退出 */
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_NO_PROCESS ;
    }

    /* 打开摄像头设备 */
    if(0 != __RCE_CAM_OpenCamera())
    {
        /* 打开摄像头失败，退出线程 */
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_STOP_ISP ;
    }
        
    /* 获取摄像头参数 */
    if(0 != __RCE_CAM_GetCameraParam())
    {
        /* 获取摄像头参数错误，退出线程 */
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_DEVICE ;
    }

    /* 设定摄像头参数 */
    if(0 != __RCE_CAM_SetCameraParam())
    {
        /* 设定摄像头参数错误，退出线程 */
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_DEVICE ;
    }

    /* 申请缓存 */
    if(0 != __RCE_CAM_RequestBuffers())
    {
        /* 申请缓存失败，退出线程 */
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_DEVICE ;
    }
    
    /* 缓存映射到用户空间 */
    if(0 != __RCE_CAM_MapBuffers())
    {
        /* 映射失败，退出线程 */
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_CLOSE_DEVICE ;
    }

    /* 启动图像输出 */
    if(0 != __RCE_CAM_StreamCmd(CAM_STREAM_COMMAND_START))
    {
        /* 启动失败，退出线程，需要释放内存映射 */
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_MUNMAP ;
    }

    while(1)
    {
        if(0 != __RCE_CAM_ProcessCameraData())
        {
            g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
            goto LABEL_EXIT_WITH_STOP_STREAM ;
        }

        /* 更新统计信息 */
        if(0 != g_stCAMWorkarea.uiUpdateStatistics)
        {
            g_stCAMWorkarea.uiUpdateStatistics = 0 ;
            
            __RCE_CAM_UpdateStatistics() ;
        }

        /* 判断线程是否需要退出 */
        if(1 == g_stCAMWorkarea.uiStopThread)
        {
            g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_END ;

            goto LABEL_EXIT_WITH_STOP_STREAM ;
        }
    }

LABEL_EXIT_WITH_STOP_STREAM :
    __RCE_CAM_StreamCmd(CAM_STREAM_COMMAND_STOP) ;
LABEL_EXIT_WITH_MUNMAP :
    __RCE_CAM_UnmapBuffers() ;
LABEL_EXIT_WITH_CLOSE_DEVICE :
    __RCE_CAM_CloseCamera() ;
LABEL_EXIT_WITH_STOP_ISP :
    __RCE_CAM_StopIsp() ;
LABEL_EXIT_WITH_NO_PROCESS :
    pthread_exit(NULL);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/******* End of file RCE_CAM.c. *******/  
