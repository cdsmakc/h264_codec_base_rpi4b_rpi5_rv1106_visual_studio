/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_config.h
    Project name : Raspiberry pi 4b Camera Encode with h264
    Module name  : 
    Date created : 2025年4月30日   8时59分31秒
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
 * 1. 1080P分辨率，需要调节GPU内存划分。否则VIDIOC_STREAMON指令会报错。修改/boot/firmware/config.txt文件，增加至128M，或者更大（有说超过256M会出问题）。
 *    gpu_mem=128
 * 2. 使用V4L2驱动编程，本质是使用树莓派的bcm2835-v4l2驱动。该驱动中有以下代码：
 * 		if (f->fmt.pix.width <= max_video_width &&
 *          f->fmt.pix.height <= max_video_height)
 *          camera_port =
 *              &dev->component[COMP_CAMERA]->output[CAM_PORT_VIDEO];
 * 		else
 *           camera_port =
 *              &dev->component[COMP_CAMERA]->output[CAM_PORT_CAPTURE];
 *     根据网上的介绍，这段代码将相机（以及树莓派内部相关硬件），根据本分辨率的不同，设置为不同的模式。默认情况下，
 *     max_video_width = 1280，max_video_height = 720。这意味着，只要设置分辨率横向超过1280宽度，或者纵向超过720高度，就会导致相机将输出设置为CAPTURE
 *     模式。该模式下，摄像头被当做照相机对待，具有巨大的帧延时，大约只有5~7 FPS输出速率。为了支持更高分辨率，需要在加载bcm2835-v4l2驱动时，修改该门限。
 *     2.1 方法1 ：命令行修改
 *           卸载驱动，执行 ：sudo rmmod bcm2835-v4l2
 *           装载驱动，执行 ：sudo modprobe bcm2835-v4l2 max_video_width=3280 max_video_height=2464  
 *           该方法可行，但是重启后配置丢失。
 *     2.2 方法2 ：修改/etc/modules
 *           在该文件后增加一句：
 *           bcm2835-v4l2 max_video_width=3280 max_video_height=2464
 *           实测这种方法无效。
 *     2.3 方法3 : 增加配置文件/etc/modprobe.d/bcm2835-v4l2.conf
 *           参考：https://forums.raspberrypi.com/viewtopic.php?t=307637
 *           在/etc/modprobe.d/路径下增加文件bcm2835-v4l2.conf文件，并编写内容：
 *           options bcm2835-v4l2 max_video_width=3280
 *           options bcm2835-v4l2 max_video_height=2464
 *           实测这种方法可行。
 * 3. 需要注意，由于CIS硬件实现，与驱动实现的关系，树莓派摄像头V2，在720p时具有更广的视场角；但是在1080p时，视场角更小。
 *      参考文档https://picamera.readthedocs.io/en/release-1.13/fov.html
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
    UINT   uiHWEncoder ;    /* 是否使用硬件编码器。0：使用软件编码；1：使用硬件编码 */ /* 缺省值1，使用硬件编码器 */
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
