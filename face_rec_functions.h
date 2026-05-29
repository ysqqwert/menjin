#ifndef FACE_REC_FUNCTIONS_H
#define FACE_REC_FUNCTIONS_H

#include <QWidget>                  // Qt窗口基类，用于创建GUI界面
#include <opencv2/opencv.hpp>       // OpenCV核心库，包含基本图像处理功能
#include <QImage>                   // Qt图像类，用于图像显示和处理
#include <opencv2/objdetect.hpp>    // OpenCV目标检测模块，包含Haar Cascade分类器
#include <opencv2/face.hpp>          // OpenCV人脸识别模块（当前未使用，保留用于扩展）
#include <opencv2/dnn.hpp>
#include <QLabel>
#include <atomic>
#include <mutex>
#include <thread>
namespace Ui {
class face_rec_functions;
}

class face_rec_functions : public QWidget
{
    Q_OBJECT

public:
    explicit face_rec_functions(QWidget *parent = nullptr);
    ~face_rec_functions();
    void startProcessing();
    void stopProcessing();
    bool updateFrame(QLabel* image);
    int features_capture(cv::Mat& mat);
    double face_recongize(cv::Mat& mat);
    void cap_init();
    // 采集按钮点击槽函数，采集人脸特征

private:                            // 私有成员区域                // UI界面指针，指向Qt Designer生成的界面对象
    cv::VideoCapture cap;           // OpenCV视频捕获对象，用于从摄像头获取视频流
    bool loadFaceCascade(const QString &path);  // 加载Haar Cascade人脸检测分类器
    void processingLoop();          // 后台线程循环采集与检测
    QImage cvMatToQImage(const cv::Mat& mat);  // 将OpenCV的Mat格式转换为Qt的QImage格式
    cv::CascadeClassifier face_cascade;        // Haar Cascade分类器对象，用于人脸检测
    bool faceCascadeReady = false;             // 分类器是否加载成功

    std::vector<cv::Rect> faces;    // 存储检测到的人脸矩形区域列表
    cv::Mat lastFrame;              // 缓存最近一帧，供采集特征复用
    QImage lastImage;               // 缓存最近一帧渲染结果
    bool hasFace = false;           // 缓存最近一帧是否检测到人脸
    std::mutex frameMutex;          // 保护帧缓存和检测结果
    std::thread workerThread;       // 后台采集线程
    std::atomic<bool> running{false}; // 线程运行标记

    int collectCount = 0;             // 已采集的人脸特征数量计数器
    const int collectTarget = 3;      // 目标采集次数，需要采集3张人脸才能进行比对

    cv::dnn::Net faceNet;            // OpenCV DNN网络对象，用于加载和运行ONNX模型
    //QVector<cv::Mat> registeredFeatures;  // 存储已注册的人脸特征向量列表
    cv::Mat getFaceFeature(const cv::Mat& faceImg);
    Ui::face_rec_functions *ui;
};

#endif // FACE_REC_FUNCTIONS_H
