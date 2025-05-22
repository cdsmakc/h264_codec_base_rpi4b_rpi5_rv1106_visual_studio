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

/* 图像宽度 */
#define RCE_CAMERA_RESOLUTION_WIDTH              (                        1280u)  

/* 图像高度*/
#define RCE_CAMERA_RESOLUTION_HEIGHT             (                         720u)

/* 图像帧率 */
#define RCE_CAMERA_FPS                           (                          60u)

/* 是否使用硬件编码 */
#define RCE_USE_HARDWARE_ENCODER                 (                           1u)

/* CAM输出格式，勿修改 */
#if 1 == RCE_USE_HARDWARE_ENCODER
#define RCE_CAMERA_PIX_FMT                       (            V4L2_PIX_FMT_H264)
#else
#define RCE_CAMERA_PIX_FMT                       (          V4L2_PIX_FMT_YUV420)
#endif

/* 编码结果是否保存为文件 */
#define RCE_H264_TO_FILE                         (                           0u)

/* 保存文件名，勿修改 */
#if 1 == RCE_H264_TO_FILE
#if 1 == RCE_USE_HARDWARE_ENCODER
#define RCE_H264_FILE_NAME                       (                  "H-ENC.264")
#else
#define RCE_H264_FILE_NAME                       (                  "S-ENC.264")
#endif
#endif

/* 通信配置 */
#define RCE_LOCAL_IP                             (              "192.168.3.101")    /* 本地IP地址 */
#define RCE_LOCAL_PORT                           (                       12001u)    /* 本地端口号 */

#define RCE_REMOTE_IP                            (              "192.168.3.100")    /* 远端IP地址 */
#define RCE_REMOTE_PORT                          (                       12000u)    /* 远端端口号 */

/* 统计间隔时间 */
#define RCE_STATISTIC_PERIOD_USEC                (                      250000u)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __RCE_CONFIG_H__ */
  
/******* End of file RCE_config.h. *******/  
