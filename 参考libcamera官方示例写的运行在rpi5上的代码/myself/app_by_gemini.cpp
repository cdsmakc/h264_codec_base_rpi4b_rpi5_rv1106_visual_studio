#include <libcamera/libcamera.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

using namespace libcamera;
using namespace std;

int main() {
    // 初始化 libcamera
    CameraManager *cameraManager = CameraManager::instance();
    cameraManager->start();

    // 获取第一个可用的相机
    shared_ptr<Camera> camera = cameraManager->cameras()[0];
    if (!camera) {
        cerr << "没有找到可用的相机！" << endl;
        return -1;
    }

    // 配置相机参数
    StreamRoles roles;
    roles.push_back(StreamRole::VideoRecording);
    CameraConfiguration *config = camera->generateConfiguration(roles);
    config->at(0)->setResolution({1280, 720});
    config->at(0)->setPixelFormat(PixelFormat::YUV420);
    config->at(0)->setBufferCount(3);

    // 设置帧率为60fps
    config->at(0)->setFrameDuration(1'000'000 / 60);  // 60 fps

    // 应用配置
    if (camera->configure(config) < 0) {
        cerr << "配置相机失败！" << endl;
        return -1;
    }

    // 打开相机
    if (camera->start() < 0) {
        cerr << "启动相机失败！" << endl;
        return -1;
    }

    // 捕获并保存YUV420图像
    cout << "开始捕获图像..." << endl;

    // 使用帧队列进行图像捕获
    for (int i = 0; i < 1; i++) { // 捕获一帧
        unique_ptr<FrameBuffer> buffer;
        if (camera->capture(&buffer) < 0) {
            cerr << "捕获图像失败！" << endl;
            return -1;
        }

        // 将YUV420数据保存到文件
        ofstream outFile("capture.yuv", ios::binary);
        outFile.write(reinterpret_cast<char *>(buffer->planes()[0].data()), buffer->planes()[0].size());
        outFile.close();

        cout << "图像已保存为 capture.yuv" << endl;
    }

    // 关闭相机
    camera->stop();

    return 0;
}
