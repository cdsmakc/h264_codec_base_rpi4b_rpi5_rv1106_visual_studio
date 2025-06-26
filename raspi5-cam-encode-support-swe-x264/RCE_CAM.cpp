/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_CAM.cpp
    Project name : Raspiberry pi 5 Camera Encode with h264
    Module name  : Camera
    Date created : 2025年6月25日   14时38分46秒
    Author       : 
    Description  : 
*******************************************************************************/

#include "RCE_defines.h"

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块包含文件        ==") ;
CODE_SECTION("==========================") ;

/* 标准库 */
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

/* 第三方 */
#include <libcamera.h>

/* 内部 */
#include "RCE_config.h"
#include "RCE_types.h"
#include "RCE_CAM.h"
#include "RCE_CAM_Class.hpp"

CODE_SECTION("==========================") ;
CODE_SECTION("==  名称空间            ==") ;
CODE_SECTION("==========================") ;
using namespace std ;
using namespace libcamera ;
using namespace std::chrono_literals ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  模块全局变量        ==") ;
CODE_SECTION("==========================") ;

RCE_CAM objRceCam ;

/* 模块工作区 */
RCE_CAM_WORKAREA_S  g_stCAMWorkarea ;

/* 待编码缓存区 */
UCHAR              *g_pucFrameBuffer ;
UINT                g_uiFrameBufferValid = 0 ;

/* 配置信息 */
extern RCE_CONFIG_S g_stRCEConfig ;

CODE_SECTION("==========================") ;
CODE_SECTION("==  类方法定义          ==") ;
CODE_SECTION("==========================") ;

RCE_CAM::RCE_CAM(void)
{
    return ;
}

/*******************************************************************************
- Function    : RCE_CAM_StartCameraManager
- Description : 创建CameraManager对象，并启动CameraManager。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT RCE_CAM::RCE_CAM_StartCameraManager(VOID)
{
    INT iRetVal = 0 ;

    // 创建CameraManager对象
    pobjCameraManager = std::make_unique<CameraManager>() ;

    // 启动CameraManager
    iRetVal = pobjCameraManager->start() ;

    if(0 != iRetVal)
    {
        cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
            <<"Start camera manager failed. Return value is "<<iRetVal<<"."<<endl ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : RCE_CAM_StopCameraManager
- Description : 停止CameraManager。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID RCE_CAM::RCE_CAM_StopCameraManager(VOID)
{
    pobjCameraManager->stop() ;
}

/*******************************************************************************
- Function    : RCE_CAM_ListCameraId
- Description : 列出camera id。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID RCE_CAM::RCE_CAM_ListCameraId(VOID)
{
    for (auto const &camera : pobjCameraManager->cameras())
    {
        cout<<"Camera id list :"<<camera->id()<< endl;
    }
}

/*******************************************************************************
- Function    : RCE_CAM_GetCamera
- Description : 获取摄像头。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。获取摄像头列表失败。
                    -2 : 失败。获取摄像头失败。
- Others      :
*******************************************************************************/
INT RCE_CAM::RCE_CAM_GetCamera(VOID)
{
    auto cameras = pobjCameraManager->cameras() ;
   
    // 查找摄像头
    if (cameras.empty()) 
    {
        cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
            <<"No cameras were founded on the system."<<endl ;

        return -1 ;
    }

    // 获取第1个摄像头的ID
    std::string cameraId = cameras[0]->id() ;

    // 获取第1个摄像头
    pobjCamera = pobjCameraManager->get(cameraId) ;

    if(nullptr == pobjCamera)
    {
        cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
            <<"Get camera failed."<<endl ;

        return -2 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : RCE_CAM_AcquireCamera
- Description : 获取摄像头。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
                        (-19)-ENODEV The camera has been disconnected from the system
                        (-16)-EBUSY  The camera is not free and can't be acquired by the caller
- Others      :
*******************************************************************************/
INT RCE_CAM::RCE_CAM_AcquireCamera(VOID)
{
    INT iRetVal = 0 ;

    iRetVal = pobjCamera->acquire() ;

    if(0 != iRetVal)
    {
        cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
            <<"Acquire camera failed. Return value is "<<iRetVal<<"."<<endl ;

        return -1 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : RCE_CAM_ReleaseCamera
- Description : 释放摄像头。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID RCE_CAM::RCE_CAM_ReleaseCamera(VOID)
{
    pobjCamera->release() ;
    pobjCamera.reset() ;
}

/*******************************************************************************
- Function    : RCE_CAM_SetFormat
- Description : 设置摄像头格式。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。生成缺省配置失败。
                    -2 : 失败。应用格式失败。
- Others      :
*******************************************************************************/
INT RCE_CAM::RCE_CAM_SetFormat(VOID)
{
    INT iRetVal = 0 ;
    
    // 生成当前配置
    pobjCameraConfig = pobjCamera->generateConfiguration({StreamRole::Viewfinder}) ;

    if(nullptr == pobjCameraConfig)
    {
        cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
            <<"Generate camera configuration failed."<<endl ;

        return -1 ;
    }

    // 获取当前流配置
    StreamConfiguration &StreamConfig = pobjCameraConfig->at(0) ;

    cout<<"Default viewfinder configuration is : "<<StreamConfig.toString()<<endl ;

    // 配置分辨率和像素格式
    StreamConfig.size.width  = g_stRCEConfig.usWidth ;
    StreamConfig.size.height = g_stRCEConfig.usHeight ;
    StreamConfig.pixelFormat = libcamera::formats::YUV420 ;

    // 以下代码未必有用，不加或许也可
    // pobjCameraConfig->sensorConfig                    = libcamera::SensorConfiguration() ;
    // pobjCameraConfig->sensorConfig->outputSize.width  = g_stRCEConfig.usWidth ;
    // pobjCameraConfig->sensorConfig->outputSize.height = g_stRCEConfig.usHeight ;
    // pobjCameraConfig->sensorConfig->bitDepth          = 8 ;

    // 验证并优化设置
    pobjCameraConfig->validate() ;

    // 打印设置
    cout<<"Validated viewfinder configuration is : "<<StreamConfig.toString()<<endl ;

    // 应用设置
    iRetVal = pobjCamera->configure(pobjCameraConfig.get()) ;

    if(0 != iRetVal)
    {
        cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
            <<"Acquire camera failed. Return value is "<<iRetVal<<"."<<endl ;

        return -2 ;
    }

    return 0 ;
}

/*******************************************************************************
- Function    : RCE_CAM_SetFps
- Description : 设置摄像头帧率。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID RCE_CAM::RCE_CAM_SetFps(VOID)
{
    UINT uiFrameDuration ;

    // 计算帧持续时间（以微秒为单位）
    uiFrameDuration = 1000000 / g_stRCEConfig.uiFps ;

    // 获取控制项
    pobjCameraControls = std::make_unique<libcamera::ControlList>() ;

    // 设置帧持续时间
     pobjCameraControls->set(libcamera::controls::FrameDurationLimits, libcamera::Span<const std::int64_t, 2>({uiFrameDuration, uiFrameDuration})) ;
}

/*******************************************************************************
- Function    : RCE_CAM_AllocBuffer
- Description : 为图像申请缓存，并将缓存添加到请求中。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。为流申请缓存失败。
                    -2 : 失败。创建请求失败。
                    -3 : 失败。将缓存添加到请求失败。
- Others      :
*******************************************************************************/
INT RCE_CAM::RCE_CAM_AllocBuffer(VOID)
{
    INT iRetVal = 0 ;

    // 为流分配缓冲区
    pobjAllocator = new FrameBufferAllocator(pobjCamera) ;

    for (StreamConfiguration &Cfg : *pobjCameraConfig) 
    {
        iRetVal = pobjAllocator->allocate(Cfg.stream()) ;

        if (0 > iRetVal) 
        {
            cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
                <<"Allocate buffers failed. Return value is "<<iRetVal<<"."<<endl ;

            return -1 ;
        }

        size_t BufferNum = pobjAllocator->buffers(Cfg.stream()).size() ;

        cout<<"Allocated "<<BufferNum<<" buffers for stream."<< std::endl ;
    }

    // 获取当前被配置的流
    StreamConfiguration &StreamConfig = pobjCameraConfig->at(0) ;
    Stream *Stream = StreamConfig.stream() ;

    const std::vector<std::unique_ptr<FrameBuffer>> &Buffers = pobjAllocator->buffers(Stream);

    for (UINT i = 0; i < Buffers.size(); ++i) 
    {
        std::unique_ptr<Request> pobjRequest = pobjCamera->createRequest() ;

        if (nullptr == pobjRequest)
        {
            cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
                <<"Create request failed."<<endl ;

            return -2 ;
        }

        const std::unique_ptr<FrameBuffer> &Buffer = Buffers[i] ;
        iRetVal = pobjRequest->addBuffer(Stream, Buffer.get()) ;

        if (0 > iRetVal)
        {
            cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
                <<"Add buffer to request failed."<<endl ;

            return -3 ;
        }

        vecpobjRequests.push_back(std::move(pobjRequest));
    }

    return 0 ;
}

/*******************************************************************************
- Function    : RCE_CAM_ReleaseBuffer
- Description : 释放图像缓存。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID RCE_CAM::RCE_CAM_ReleaseBuffer(VOID)
{
    pobjAllocator->free(pobjCameraConfig->at(0).stream()) ;
    delete pobjAllocator ;
}

/*******************************************************************************
- Function    : RCE_CAM_SetRequestCb
- Description : 设置请求回调函数。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
- Others      :
*******************************************************************************/
INT RCE_CAM::RCE_CAM_SetRequestCb(VOID (*__RequesetCb)(Request *))
//INT RCE_CAM::RCE_CAM_SetRequestCb(VOID)
{
    pobjCamera->requestCompleted.connect(__RequesetCb) ;

    return 0 ;
}

/*******************************************************************************
- Function    : RCE_CAM_StartCamera
- Description : 启动摄像头，并将请求加入到队列。
- Input       : VOID
- Output      : NULL
- Return      : INT
                    0  : 成功。
                    -1 : 失败。
- Others      :
*******************************************************************************/
INT RCE_CAM::RCE_CAM_StartCamera(VOID)
{
    INT iRetVal = 0 ;

    iRetVal = pobjCamera->start(pobjCameraControls.get()) ;

    // 启动相机，使相机处于running状态
    if(0 != iRetVal)
    {
        cout<<"File "<<__FILE__<<", func "<<__func__<<", line"<<__LINE__<<" : "
            <<"Start camera failed. Return value "<<iRetVal<<"."<<endl ;

        return -1 ;
    }

    // 将request加入队列，注意仅处于running状态时才可加入
    for (std::unique_ptr<Request> &request : vecpobjRequests)
    {
        pobjCamera->queueRequest(request.get());
    }
    
    return 0 ;
}

/*******************************************************************************
- Function    : RCE_CAM_StopCamera
- Description : 停止摄像头。
- Input       : VOID
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID RCE_CAM::RCE_CAM_StopCamera(VOID)
{
    pobjCamera->stop() ;
}

/*******************************************************************************
- Function    : RCE_CAM_ProcessData
- Description : request响应，处理数据。
- Input       : pobjRequest : 由libcamera驱动处理。
- Output      : NULL
- Return      : VOID
- Others      :
*******************************************************************************/
VOID RCE_CAM_ProcessData(Request *pobjRequest)
{
    VOID  *pvMmapAddr ;
    UINT   uiLength = g_stRCEConfig.usWidth * g_stRCEConfig.usHeight * 3 / 2 ;
    UINT   uiOffset = 0 ;

    if (pobjRequest->status() == Request::RequestCancelled)
    {
        return;
    }
        
    const std::map<const Stream *, FrameBuffer *> &Buffers = pobjRequest->buffers();

    for (auto bufferPair : Buffers) 
    {
        // 更新统计：从摄像头接收的帧数和数据量
        g_stCAMWorkarea.stStatistics.uiFrameCountFromCamera++ ;
        g_stCAMWorkarea.stStatistics.ui64DataCountFromCamera += uiLength ;

        // 获取图像缓冲
        FrameBuffer *Buffer = bufferPair.second;

        if(0 == g_uiFrameBufferValid)
        {
            // 将buffer映射到用户空间
            pvMmapAddr = mmap(NULL,
                            uiLength,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED,
                            Buffer->planes()[0].fd.get(),
                            uiOffset);

            // 复制图像
            memcpy(g_pucFrameBuffer, pvMmapAddr, uiLength) ;

            // 设置标记，等待编码完成 
            g_uiFrameBufferValid = 1 ;

            // 释放映射
            munmap(pvMmapAddr, uiLength) ;

            // 更新统计：更新发送到编码器的帧数 
            g_stCAMWorkarea.stStatistics.uiFrameCountToEncoder++ ;
        }
    }

    // 恢复请求，并重新排队请求
    pobjRequest->reuse(Request::ReuseBuffers);
    //objRceCam.pobjCamera->queueRequest(pobjRequest);
    objRceCam.RCE_CAM_QueueRequset(pobjRequest) ;

    return ;
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
    

    if(0 != objRceCam.RCE_CAM_StartCameraManager())
    {
        // 创建并启动CameraManager失败，退出线程 
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_NO_PROCESS ;
    }

    if(0 != objRceCam.RCE_CAM_GetCamera())
    {
        // 获取摄像头失败，退出线程
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_STOP_CAMERA_MANAGER ;
    }

    if(0 != objRceCam.RCE_CAM_AcquireCamera())
    {
        // 锁定摄像头失败，退出线程
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_STOP_CAMERA_MANAGER ;
    }

    if(0 != objRceCam.RCE_CAM_SetFormat())
    {
        // 配置摄像头格式失败，退出线程
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_RELEASE_CAMERA ;
    }

    // 设置帧率
    objRceCam.RCE_CAM_SetFps() ;

    if(0 != objRceCam.RCE_CAM_AllocBuffer())
    {
        // 申请图像缓存失败，退出线程
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_RELEASE_CAMERA ;
    }

    //if(0 != objRceCam.RCE_CAM_SetRequestCb(RCE_CAM::RCE_CAM_ProcessData))

    if(0 != objRceCam.RCE_CAM_SetRequestCb(RCE_CAM_ProcessData))
    {
        // 连接数据处理回调函数失败，退出线程
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_RELEASE_BUFFER ;
    }

    if(0 != objRceCam.RCE_CAM_StartCamera())
    {
        // 启动相机失败，退出线程
        g_stCAMWorkarea.enThreadStatus = CAM_THREAD_STATUS_EXIT ;
        goto LABEL_EXIT_WITH_RELEASE_BUFFER ;
    }

    while(1)
    {
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

            goto LABEL_EXIT_WITH_CLOSE_CAMERA ;
        }

        usleep(1000) ;
    }

LABEL_EXIT_WITH_CLOSE_CAMERA:
    objRceCam.RCE_CAM_StopCamera() ;
LABEL_EXIT_WITH_RELEASE_BUFFER : 
    objRceCam.RCE_CAM_ReleaseBuffer() ;
LABEL_EXIT_WITH_RELEASE_CAMERA :
    objRceCam.RCE_CAM_ReleaseCamera() ;
LABEL_EXIT_WITH_STOP_CAMERA_MANAGER :
    objRceCam.RCE_CAM_StopCameraManager() ;
LABEL_EXIT_WITH_NO_PROCESS :
    pthread_exit(NULL);
}

/******* End of file RCE_CAM.cpp. *******/  
