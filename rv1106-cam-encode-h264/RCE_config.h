/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_config.h
    Project name : RV1106 Camera Encode with h264
    Module name  : 
    Date created : 2025年6月6日   17时52分31秒
    Author       : 
    Description  : 
*******************************************************************************/
#ifndef __RCE_CONFIG_H__
#define __RCE_CONFIG_H__
  
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "RCE_types.h"

/*
 * 注意事项：
 * 1. 目前不支持修改分辨率，因为VIDIOC_S_PARM命令会失败。
 * 2. luckfox的镜像，默认会启动rkipc。这个应用会占用硬件资源。可以进入后killall rkipc撤销这个应用。
 *    2.1 如果用killall rkipc撤销应用，本应用可以运行。
 *    2.2 修改“/oem/usr/bin/RkLunch.sh”文件。这个文件在系统启动时运行。
 *              #rkipc -a /oem/usr/share/iqfiles &
 *        但是注释掉这句话后，带来一系列问题。首先是原本配置的eth0不通了。可以修改“/etc/init.d/rcS”文件，
 *        开头增加：
 *              ifconfig eth0 192.168.3.106 up
 *              route add default gw 192.168.0.1
 *        这样的话有线网就通了。
 *        但是很多ko没有加载。
 *    2.3 曲线救国的方法是，使能rkipc的运行，然后在“/etc/init.d/rcS” 文件末尾增加killall rkipc，在rkipc运行后
 *        干掉，就好了。
 * 3. RK_MPI_VENC_GetStream这个函数，第三个参数设置为0，按照手册说明，应该是非阻塞。但是实测在无数据时，会阻塞
 *    100ms，从而导致运行效率低下。该参数设置为1，在无数据时，会按照设置的参数运行，提高运行效率。
 */

/* ini配置文件 */
#define RCE_CONFIG_FILE              ("./RCE_config.ini")

/* 全局配置信息 */
typedef struct tagRCE_CONFIG
{
    /* 图像信息 */
    USHORT usWidth ;        /* 图像宽度 */           /* 缺省值1280              */
    USHORT usHeight ;       /* 图像高度 */           /* 缺省值720               */
    UINT   uiFps ;          /* 图像帧率 */           /* 缺省值30                */

    /* 编码器设置 */
    UINT   uiHWBitrate ;    /* 设置硬件编码时的输出码率 */
    
    /* 输出文件设置 */
    UINT   uiWrToFile ;     /* 是否写入到文件。0：不写入文件；1：写入文件 */ /* 缺省值0，不写入文件 */
    CHAR   acFile[128] ;    /* 文件名 */

    /* 网络设置 */
    CHAR   acLocalIp[64] ;  /* 本地IP */
    USHORT usLocalPort ;    /* 本地端口 */

    CHAR   acRemoteIp[64] ; /* 远端IP */
    USHORT usRemotePort ;   /* 远端端口 */
} RCE_CONFIG_S ;

/*********************************************************
 * 缺省配置
 *********************************************************/
/* 图像宽度、高度、帧率缺省值 */
#define RCE_CAMERA_RESOLUTION_WIDTH_DEFAULT      (                        1280u)  
#define RCE_CAMERA_RESOLUTION_HEIGHT_DEFAULT     (                         720u)
#define RCE_CAMERA_FPS_DEFAULT                   (                          60u)

/* 保存文件名缺省值 */
#define RCE_H264_FILE_NAME_DEFAULT               (                    "enc.264")

/* 硬件编码输出码率缺省值 */
#define RCE_H264_HW_BITRATE                      (                     2000000u)

/* 通信配置缺省值 */
#define RCE_LOCAL_IP_DEFAULT                     (              "192.168.3.101")    /* 本地IP地址 */
#define RCE_LOCAL_PORT_DEFAULT                   (                       12001u)    /* 本地端口号 */

#define RCE_REMOTE_IP_DEFAULT                    (              "192.168.3.100")    /* 远端IP地址 */
#define RCE_REMOTE_PORT_DEFAULT                  (                       12000u)    /* 远端端口号 */

/* 统计间隔时间 */
#define RCE_STATISTIC_PERIOD_USEC                (                      250000u)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __RCE_CONFIG_H__ */
  
/******* End of file RCE_config.h. *******/  
