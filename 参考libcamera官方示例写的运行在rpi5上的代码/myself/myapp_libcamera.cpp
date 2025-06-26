
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

static std::shared_ptr<Camera> camera;

static void requestComplete(Request *request) ;

int fd ;

int main()
{
    int iRetVal ;
    // Code to follow
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();

    for (auto const &camera : cm->cameras())
        std::cout << camera->id() << std::endl;

    auto cameras = cm->cameras();

    if (cameras.empty()) 
    {
        std::cout << "No cameras were identified on the system."<< std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }

    std::cout << "Camera(s) founded."<< std::endl;

    std::string cameraId = cameras[0]->id();

    std::cout << "Camera id is "<<cameraId<< std::endl;

    camera = cm->get(cameraId);

    iRetVal = camera->acquire();

    if(0 == iRetVal)
    {
        std::cout << "Acquire camera success!"<< std::endl;
    }

    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );

    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

    streamConfig.size.width = 640;
    streamConfig.size.height = 480;
    streamConfig.pixelFormat = libcamera::formats::YUV420 ;
    config->sensorConfig = libcamera::SensorConfiguration();
    config->sensorConfig->outputSize.width = 640;
    config->sensorConfig->outputSize.height = 480;
    config->sensorConfig->bitDepth = 8;

    //libcamera::ControlList controls = controls::controls ;
    //controls.set(controls::FrameDurationLimits, libcamera::Span<const int64_t, 2>({ 100000, 100000 }));
     std::unique_ptr<libcamera::ControlList> camcontrols = std::make_unique<libcamera::ControlList>();
     camcontrols->set(libcamera::controls::FrameDurationLimits, libcamera::Span<const std::int64_t, 2>({16666, 16666}));

    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

    config->validate();
    std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;

    camera->configure(config.get());

    FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);

    for (StreamConfiguration &cfg : *config) 
    {
        int ret = allocator->allocate(cfg.stream());

        if (ret < 0) 
        {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -ENOMEM;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }

    Stream *stream = streamConfig.stream();

    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);
    std::vector<std::unique_ptr<Request>> requests;

    for (unsigned int i = 0; i < buffers.size(); ++i) 
    {
        std::unique_ptr<Request> request = camera->createRequest();

        if (!request)
        {
            //std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0)
        {
            //std::cerr << "Can't set buffer for request"<< std::endl;
            return ret;
        }

        requests.push_back(std::move(request));
    }

    fd = open("yuv420.yuv", O_RDWR | O_CREAT | O_TRUNC , 0777) ;

    camera->requestCompleted.connect(requestComplete);

    camera->start(camcontrols.get());

    for (std::unique_ptr<Request> &request : requests)
    camera->queueRequest(request.get());

    std::this_thread::sleep_for(3000ms);

    close(fd) ;
    camera->stop();
    allocator->free(stream);
    delete allocator;
    camera->release();
    camera.reset();
    cm->stop();


    return 0;
}

static void requestComplete(Request *request)
{
    int iLoop = 0 ;
    void *mmap_addr ;
    unsigned int length ;
    unsigned int offset ;

    if (request->status() == Request::RequestCancelled)
        return;

    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();


    for (auto bufferPair : buffers) 
    {
        iLoop++;
        //std::cout<<iLoop<<std::endl ;

        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();

        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";

        unsigned int nplane = 0;

        for (const FrameMetadata::Plane &plane : metadata.planes())
        {
            std::cout << plane.bytesused;
            if (++nplane < metadata.planes().size()) std::cout << "/";
        }
        
        std::cout << "    " ;

        /*
        // plane0
        length = buffer->planes()[0].length ;
        offset = buffer->planes()[0].offset ;

        mmap_addr = mmap(NULL,
                         length,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         buffer->planes()[0].fd.get(),
                         offset);

        write(fd, mmap_addr, length) ;   

        munmap(mmap_addr, length) ;

        std::cout << length << "/" << offset << "    "  ;

        
        // plane1
        length = buffer->planes()[1].length ;
        offset = buffer->planes()[1].offset ;

        mmap_addr = mmap(NULL,
                         length,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         buffer->planes()[0].fd.get(),
                         offset);

        write(fd, mmap_addr, length) ;   

        munmap(mmap_addr, length) ;

        std::cout << length << "/" << offset << "    "  ;

        // plane2
        length = buffer->planes()[2].length ;
        offset = buffer->planes()[2].offset ;

        mmap_addr = mmap(NULL,
                         length,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         buffer->planes()[0].fd.get(),
                         offset);

        write(fd, mmap_addr, length) ;   

        munmap(mmap_addr, length) ;

        std::cout << length << "/" << offset << "    "  ;
        */

        mmap_addr = mmap(NULL,
                         460800,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         buffer->planes()[0].fd.get(),
                         0);

        write(fd, mmap_addr, 460800) ;   

        munmap(mmap_addr, 460800) ;

        std::cout << std::endl;
    }

    request->reuse(Request::ReuseBuffers);
    camera->queueRequest(request);

}






































/*
#include <iostream>
#include <libcamera.h>

using namespace std ;
using namespace libcamera ;

std::unique_ptr<libcamera::CameraManager> g_CameraManager ;
std::vector<std::shared_ptr<libcamera::Camera>> g_CameraList ;
std::shared_ptr<Camera> g_Camera ;


int __CAM_InitCameraManager(void)
{
    int iRetVal ;
    g_CameraManager.reset() ;
    g_CameraManager = make_unique<CameraManager>() ;
    iRetVal = g_CameraManager->start() ;

    if(0 != iRetVal)
    {
        cout<<__FILE__<<" "<<__LINE__<<"  CAM : init camera manager failed. Return val is "<<iRetVal<<"."<<endl ;
        return -1 ;
    }

    return 0 ;
}

int __CAM_GetCamera(void)
{
    int iRetVal ;

    g_CameraList = g_CameraManager->cameras() ;

    if(0 == g_CameraList.size())
    {
        cout<<__FILE__<<" "<<__LINE__<<"  CAM : Get camera list failed. No camera founded."<<endl ;

        return -1 ;
    }
    else
    {
        cout<<__FILE__<<" "<<__LINE__<<"  CAM : Total "<<g_CameraList.size()<<" founded."<<endl ;
    }

    std::string const &CameraId = g_CameraList[0]->id();

    g_Camera = g_CameraManager->get(CameraId) ;

    if(nullptr == g_Camera)
    {
        cout<<__FILE__<<" "<<__LINE__<<"  CAM : Get camera failed."<<endl ;

        return -1 ;
    }
    else
    {
        cout<<__FILE__<<" "<<__LINE__<<"  CAM : get camera success."<<endl ;
    }

    iRetVal = g_Camera->acquire() ;

    if(0 == iRetVal)
    {
        cout<<__FILE__<<" "<<__LINE__<<"  CAM : Acquire camera success."<<endl ;
    }
    else
    {
        cout<<__FILE__<<" "<<__LINE__<<"  CAM : Acquire camera failed.  Return value is "<<iRetVal<<"."<<endl ;

        return -1 ;
    }

    return 0 ;
}

int main(void)
{
    __CAM_InitCameraManager() ;
    __CAM_GetCamera() ;

}
*/


