/*******************************************************************************
 Copyright -- Corporation. 2016. All rights reserved.
--------------------------------------------------------------------------------
    File name    : RCE_CAM_Class.hpp
    Project name : Raspiberry pi 5 Camera Encode with h264
    Module name  : Camera
    Date created : 2025年6月25日   14时38分46秒
    Author       : 
    Description  : 
*******************************************************************************/

#ifndef __RCE_CAM_CLASS_H__
#define __RCE_CAM_CLASS_H__

#include "RCE_defines.h"
#include "RCE_types.h"

#include <libcamera.h>

using namespace libcamera;

CODE_SECTION("==========================") ;
CODE_SECTION("==  类定义              ==") ;
CODE_SECTION("==========================") ;

/* 应用类 */
class RCE_CAM
{
private :
    std::unique_ptr<CameraManager>          pobjCameraManager ;
    std::shared_ptr<Camera>                 pobjCamera ;
    std::unique_ptr<CameraConfiguration>    pobjCameraConfig ;
    std::unique_ptr<ControlList>            pobjCameraControls ;
    FrameBufferAllocator                   *pobjAllocator ;
    std::vector<std::unique_ptr<Request>>   vecpobjRequests ;
public :
    RCE_CAM(VOID) ;
public :
    // camera manager
    INT  RCE_CAM_StartCameraManager(VOID) ;
    VOID RCE_CAM_StopCameraManager(VOID) ;

    // list camera and get camera
    VOID RCE_CAM_ListCameraId(VOID) ;
    INT  RCE_CAM_GetCamera(VOID) ;

    // acquire / release camera
    INT  RCE_CAM_AcquireCamera(VOID) ;
    VOID RCE_CAM_ReleaseCamera(VOID) ;

    // set camera pixel format, resolution, fps
    INT  RCE_CAM_SetFormat(VOID) ;
    VOID RCE_CAM_SetFps(VOID) ;

    // buffer
    INT  RCE_CAM_AllocBuffer(VOID) ;
    VOID RCE_CAM_ReleaseBuffer(VOID) ;

    // request callback
    INT  RCE_CAM_SetRequestCb(VOID (*__RequesetCb)(Request *)) ;
    VOID RCE_CAM_QueueRequset(Request *pobjRequest){pobjCamera->queueRequest(pobjRequest);}
    //INT  RCE_CAM_SetRequestCb(VOID) ;

    // camera start / stop
    INT  RCE_CAM_StartCamera(VOID) ;
    VOID RCE_CAM_StopCamera(VOID) ;

    // data process
    //static VOID RCE_CAM_ProcessData(Request *Request) ;
} ;

#endif /* __RCE_CAM_CLASS_H__ */
/******* End of file RCE_CAM_Class.hpp. *******/  
