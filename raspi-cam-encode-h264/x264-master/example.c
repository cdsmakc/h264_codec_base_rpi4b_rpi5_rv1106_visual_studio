
#include <asm/types.h>
#include <assert.h>
#include <errno.h>
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
#include <unistd.h>
#include <time.h>

//#include <io.h>       /* _setmode() */
#include <fcntl.h>    /* _O_BINARY */

#include <x264.h>


#define WRITE_TO_FILE              (            0u)

#define FRAME_NUMBER               (          500u)

/* 从摄像头获取图像的分辨率 */
#define CAMERA_RESOLUTION_WIDTH    (         1280u)
#define CAMERA_RESOLUTION_HEIGHT   (          720u)
#define CAMERA_FPS                 (           30u)

/* Video 设备文件描述符 */
int              g_iFDVideo ;

/* 写入(保存)文件描述符 */
int              g_iFDStore ;

/* 数据缓冲区 */
typedef struct tag_BufDesc
{
    void         *pvBufPtr ;
    size_t        szBufSize ;
} stBufDesc ;

stBufDesc        *g_pstBufDesc ;


/* x264相关定义 */
x264_param_t      g_stParam ;
x264_t           *g_pstX264Handler ;
x264_picture_t    g_stPic ;
x264_picture_t    g_PicOut ;
x264_nal_t       *g_stNAL ;
unsigned int      g_uiFrameCtr = 0 ;


/*******************************************************************************
- Function    : __QueryCameraCapability
- Description : 本函数查询摄像头能力。
- Input       : void
- Output      : null
- Return      : void
- Others      :
*******************************************************************************/
void __QueryCameraCapability(void)
{
    struct v4l2_capability  stCap ;
    struct v4l2_fmtdesc     stFormatDesc ;
    struct v4l2_frmsizeenum stFrameSize ;
    char                    acPrintBuf[64] ;
    int                     iRetVal ;

    /* 获取摄像头能力 */
    if(-1 == ioctl(g_iFDVideo, VIDIOC_QUERYCAP, &stCap))
    {
        printf("ioctl VIDIOC_QUERYCAP error.\n") ;
        exit(-1) ;
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
    }


    /* 获取并打印摄像头支持的格式 */
    stFormatDesc.index = 0 ;
    stFormatDesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE ;

    printf("--Format descriptor --------------------------------------\n") ;
    
    while(-1 != ioctl(g_iFDVideo, VIDIOC_ENUM_FMT, &stFormatDesc))
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
    
    printf("----------------------------------------------------------\n\n\n") ;

    /* 获取并打印摄像头支持的分辨率*/
    stFrameSize.index        = 0 ;
    stFrameSize.pixel_format = V4L2_PIX_FMT_YUYV ;
    
    printf("--Frame resolution descriptor ----------------------------\n") ;
    
    while(-1 != ioctl(g_iFDVideo, VIDIOC_ENUM_FRAMESIZES, &stFrameSize))
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
    
    printf("----------------------------------------------------------\n\n\n") ;

    return ;
}

/*******************************************************************************
- Function    : __SetCameraResolution
- Description : 本函数设置摄像头输出格式。
- Input       : void
- Output      : null
- Return      : void
- Others      :
*******************************************************************************/
void __SetCameraResolution(void)
{
    struct v4l2_format     stFormat ;

    memset(&stFormat, 0, sizeof(stFormat)) ;

    /* 设置输出格式 */
    stFormat.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    stFormat.fmt.pix.width       = CAMERA_RESOLUTION_WIDTH ;
    stFormat.fmt.pix.height      = CAMERA_RESOLUTION_HEIGHT ;
    stFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420 ;             /* 根据需要的格式填写 */
    
    if(-1 == ioctl(g_iFDVideo, VIDIOC_S_FMT, &stFormat)) 
    {
        printf("ioctl VIDIOC_S_FMT error.\n") ;
        exit(-1) ;
    }

    /* 检查设置效果 */
    stFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;
    
    if(-1 == ioctl(g_iFDVideo, VIDIOC_G_FMT, &stFormat))
    {
        printf("ioctl VIDIOC_G_FMT error.\n") ;
        exit(-1) ;
    }
    else
    {
        printf("--Set format result---------------------------------------\n") ;
        printf("-- Resolution width  : %d\n"
               "-- Resolution height : %d\n"
               "-- Pixel format      : %c%c%c%c\n",
               stFormat.fmt.pix.width, stFormat.fmt.pix.height,
               stFormat.fmt.pix.pixelformat & 0xFF,
               (stFormat.fmt.pix.pixelformat >> 8) & 0xFF,
               (stFormat.fmt.pix.pixelformat >> 16) & 0xFF,
               (stFormat.fmt.pix.pixelformat >> 24) & 0xFF);
        printf("----------------------------------------------------------\n\n\n") ;
    }

    return ;
}

/*******************************************************************************
- Function    : __SetCameraFPS
- Description : 本函数设置摄像头输出格式。
- Input       : void
- Output      : null
- Return      : void
- Others      :
*******************************************************************************/
void __SetCameraFPS(void)
{
    struct v4l2_streamparm stStreamParm ;
    
    stStreamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE ;

    /* 获取当前设置 */
    if(-1 == ioctl(g_iFDVideo, VIDIOC_G_PARM, &stStreamParm))
    {
        printf("ioctl VIDIOC_G_PARM error.\n") ;
        exit(-1) ;
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

    /* 修改FPS */
    stStreamParm.parm.capture.timeperframe.numerator   = 1 ;
    stStreamParm.parm.capture.timeperframe.denominator = CAMERA_FPS ;

    if(-1 == ioctl(g_iFDVideo, VIDIOC_S_PARM, &stStreamParm))
    {
        printf("ioctl VIDIOC_S_PARM error.\n") ;
        exit(-1) ;
    }

    /* 获取新设置 */
    if(-1 == ioctl(g_iFDVideo, VIDIOC_G_PARM, &stStreamParm))
    {
        printf("ioctl VIDIOC_G_PARM error.\n") ;
        exit(-1) ;
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

    return ;
}

/*******************************************************************************
- Function    : __AllocateBuffers
- Description : 本函数获取缓冲区，并将缓冲区映射到用户空间。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
void __AllocateBuffers(void)
{
    struct v4l2_requestbuffers stReqBuf ;
    struct v4l2_buffer         stBuf ;
    int                        iLoop ;

    memset(&stReqBuf, 0, sizeof(stReqBuf)) ;

    /* 申请缓冲区 */
    stReqBuf.count  = 8 ;
    stReqBuf.memory = V4L2_MEMORY_MMAP ;
    stReqBuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE ;

    if(-1 == ioctl(g_iFDVideo, VIDIOC_REQBUFS, &stReqBuf))
    {
        printf("Request buffers failed.\n") ;
        exit(-1) ;
    }

    /* 申请用户空间缓冲区描述符空间 */
    g_pstBufDesc = (stBufDesc *)calloc(stReqBuf.count, sizeof(stBufDesc)) ;

    if(NULL == g_pstBufDesc)
    {
        printf("Calloc buffer descriptor failed.\n") ;
        exit(-1) ;
    }

    /* 将申请的缓冲区映射到用户空间 */
    for(iLoop = 0 ; iLoop < stReqBuf.count ; iLoop++)
    {
        memset(&stBuf, 0, sizeof(stBuf)) ;

        stBuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        stBuf.index  = iLoop;
        stBuf.memory = V4L2_MEMORY_MMAP;
        
        if(-1 == ioctl(g_iFDVideo, VIDIOC_QUERYBUF, &stBuf))
        {
            printf("Get buffer information failed.\n") ;
            exit(-1) ;
        }
        
        g_pstBufDesc[iLoop].szBufSize = stBuf.length;
        g_pstBufDesc[iLoop].pvBufPtr  = mmap(NULL,
                                             stBuf.length,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED,
                                             g_iFDVideo,
                                             stBuf.m.offset);
        
        if(MAP_FAILED == g_pstBufDesc[iLoop].pvBufPtr)
        {
            printf("Map V4L2 buffer to user space failed.\n") ;
            exit(-1) ;
        }

        if(-1 == ioctl(g_iFDVideo, VIDIOC_QBUF, &stBuf))
        {
            printf("Queue buffer failed.\n") ;
            exit(-1) ;
        }
    }

    printf("--Buffer information---------------------------------------\n") ;
    
    for(iLoop = 0 ; iLoop < stReqBuf.count ; iLoop++)
    {
        printf("-- Frame buffer :%d   address :0x%x    size:%d\n", 
               iLoop, 
               (unsigned int) g_pstBufDesc[iLoop].pvBufPtr, 
               (int)g_pstBufDesc[iLoop].szBufSize) ;
    }
    printf("-----------------------------------------------------------\n") ;

    return ;
}


/*******************************************************************************
- Function    : __StartStream
- Description : 本函数启动数据接收。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
void __StartStream(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if(-1 == ioctl(g_iFDVideo, VIDIOC_STREAMON, &type))
    {
        printf("Start stream failed.\n") ;
        exit(-1) ;
    }

    return ;
}


/*******************************************************************************
- Function    : __StopStream
- Description : 本函数停止数据接收。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
void __StopStream(void)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if(-1 == ioctl(g_iFDVideo, VIDIOC_STREAMOFF, &type))
    {
        printf("Stop steam failed.\n") ;
        exit(-1) ;
    }

    return ;
}


/*******************************************************************************
- Function    : __ReadFrame
- Description : 本函数读取摄像头数据并写入到文件。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
void __ReadFrame(void)
{
    struct v4l2_buffer stBuf;
    size_t             szLumaSize ;
    size_t             szChromaSize ;
    int                iEncodeSize ;
    int                iNALCtr ;
    void              *pvLumaPtr ;
    void              *pvChroma1Ptr ;
    void              *pvChroma2Ptr ;

    memset(&stBuf, 0, sizeof(stBuf)) ;

    stBuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stBuf.memory = V4L2_MEMORY_MMAP;
    
    if(-1 == ioctl(g_iFDVideo, VIDIOC_DQBUF, &stBuf))
    {
        printf("De-queue buffer failed.\n") ;
        exit(-1) ;
    }

#if 1 == WRITE_TO_FILE
    if(-1 == write(g_iFDStore, g_pstBufDesc[stBuf.index].pvBufPtr, stBuf.bytesused))
    {
        printf("Write camera data failed.\n") ;
        exit(-1) ;
    }
#endif
    /* 编码 */
    szLumaSize   = CAMERA_RESOLUTION_WIDTH * CAMERA_RESOLUTION_HEIGHT ;
    szChromaSize = szLumaSize / 4 ;
    
    pvLumaPtr    = g_pstBufDesc[stBuf.index].pvBufPtr ;
    pvChroma1Ptr = pvLumaPtr + szLumaSize ;
    pvChroma2Ptr = pvChroma1Ptr + szChromaSize ;

    memcpy(g_stPic.img.plane[0], pvLumaPtr, szLumaSize) ;
    memcpy(g_stPic.img.plane[1], pvChroma1Ptr, szChromaSize) ;
    memcpy(g_stPic.img.plane[2], pvChroma2Ptr, szChromaSize) ;

    g_stPic.i_pts = g_uiFrameCtr;
    
    iEncodeSize = x264_encoder_encode(g_pstX264Handler, &g_stNAL, &iNALCtr, &g_stPic, &g_PicOut) ;
    
    if(0 > iEncodeSize)
    {
        printf("x264_encoder_encode retval error 1.\n") ;
    }
    else if(0 < iEncodeSize)
    {
        write(g_iFDStore, g_stNAL->p_payload, iEncodeSize) ;
    }

    g_uiFrameCtr++ ;

    if(-1 == ioctl(g_iFDVideo, VIDIOC_QBUF, &stBuf))
    {
        printf("Re-Queue buffer failed.\n") ;
        exit(-1) ;
    }
    
    return ;
}


/*******************************************************************************
- Function    : __UnmapBuffer
- Description : 本函数撤除缓冲区映射。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
void __UnmapBuffer(void)
{
    int iLoop ;
    
    for(iLoop = 0 ; iLoop < 4 ; iLoop++) 
    {
        munmap(g_pstBufDesc[iLoop].pvBufPtr, g_pstBufDesc[iLoop].szBufSize) ;
    }
    
    free(g_pstBufDesc) ;

    return ;
}


/*******************************************************************************
- Function    : __CaptureFrame
- Description : 本函数捕获帧数据。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
void __CaptureFrame(void)
{
    struct timeval stTimeVal ;
    fd_set         stFDRead ;
    int            iRetVal ;
    
    stTimeVal.tv_usec = 0 ;
    stTimeVal.tv_sec  = 2 ; 
    
    FD_ZERO(&stFDRead) ;
    FD_SET(g_iFDVideo, &stFDRead) ;

    iRetVal = select(g_iFDVideo + 1, &stFDRead, NULL, NULL, &stTimeVal) ;
    
    if(-1 == iRetVal)
    {
        perror("select") ;
        exit(-1) ;
    }
    else if(0 == iRetVal)
    {
        printf("timeout.\n") ;
        exit(-1) ;
    }
    printf("55\n");
    __ReadFrame() ;

    return ;
}



int main(int argc, char *argv[])
{
    struct timeval  stStartTime, stEndTime ;
    unsigned int    uiTimeIntVal ;
    int             iFrame ;
    int             iEncodeSize ;
    int             iNALCtr ;

    /**************************************************************************************************************
     * 准备工作 :
     *     1. 设置编码器;
     *     2. 打开编码器;
     *     3. 打开设备文件;
     *     4. 打开待写入文件。
     **************************************************************************************************************/
    /* 设置编码 */
    if(0 > x264_param_default_preset(&g_stParam, "superfast", NULL))
    {
        return -1 ;
    }

    g_stParam.i_bitdepth       = 8;
    g_stParam.i_csp            = X264_CSP_I420;
    g_stParam.i_width          = CAMERA_RESOLUTION_WIDTH;
    g_stParam.i_height         = CAMERA_RESOLUTION_HEIGHT;
    g_stParam.b_vfr_input      = 0;
    g_stParam.b_repeat_headers = 1;
    g_stParam.b_annexb         = 1;

    if(0 > x264_param_apply_profile(&g_stParam, "high"))
    {
        return -1 ;
    }

    if(0 > x264_picture_alloc(&g_stPic, g_stParam.i_csp, g_stParam.i_width, g_stParam.i_height))
    {
        return -1 ;
    }
    
    /* 启动编码器 */
    g_pstX264Handler = x264_encoder_open(&g_stParam) ;

    if(NULL == g_pstX264Handler)
    {
        x264_picture_clean(&g_stPic) ;

        return -1 ;
    }

    /* 打开摄像头和存储文件 */
    g_iFDVideo = open("/dev/video0", O_RDWR | O_NONBLOCK, 0) ;
    
    if(g_iFDVideo == -1)
    {
        x264_picture_clean(&g_stPic) ;

        printf("Can not open /dev/video0.\n") ;
        return -1;
    }

    
    g_iFDStore = open("./out.264", O_RDWR | O_CREAT, 0777) ;
    
    if(g_iFDStore == -1)
    {
        x264_picture_clean(&g_stPic) ;

        printf("Open out file is failed.\n") ;
        return -1;
    }

    /* 查询摄像头能力 */
    __QueryCameraCapability() ;

    /* 设置摄像头输出格式 */
    __SetCameraResolution() ;
    __SetCameraFPS() ;
    
    /* 申请并映射缓冲区 */
    __AllocateBuffers() ;
    
    /* 启动获取视频数据 */
    __StartStream() ;

    /**************************************************************************************************************
     * 采集与编码
     **************************************************************************************************************/
    gettimeofday(&stStartTime, NULL) ;
    
    for(iFrame = 0 ; iFrame < FRAME_NUMBER ; iFrame++)
    {
        __CaptureFrame() ;
    }
    
    while(x264_encoder_delayed_frames(g_pstX264Handler))
    {
        iEncodeSize = x264_encoder_encode(g_pstX264Handler, &g_stNAL, &iNALCtr, NULL, &g_PicOut) ;

        if(0 > iEncodeSize)
        {
            printf("x264_encoder_encode retval error 2.\n") ;
        }
        else if(0 < iEncodeSize)
        {
            write(g_iFDStore, g_stNAL->p_payload, iEncodeSize) ;
        }
    }
    
    gettimeofday(&stEndTime, NULL) ;
    
    uiTimeIntVal = stEndTime.tv_sec * 1000000 + stEndTime.tv_usec - stStartTime.tv_sec * 1000000 - stStartTime.tv_usec ;
    
    printf("FPS : %3.2f \n", (float)FRAME_NUMBER / ((float)uiTimeIntVal / 1000000.0f)) ;


    /**************************************************************************************************************
     * 结束采集与编码
     **************************************************************************************************************/
    /* 停止获取视频数据 */
    __StopStream() ;
    
    /* 撤销缓存映射并释放缓存 */
    __UnmapBuffer() ;

    /* 关闭设备文件 */
    close(g_iFDVideo) ;
    close(g_iFDStore) ;
    
    x264_encoder_close(g_pstX264Handler) ;
    x264_picture_clean(&g_stPic) ;

    return 0 ;
}


                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                    












