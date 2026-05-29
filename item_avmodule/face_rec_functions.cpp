#include "face_rec_functions.h"
#include "ui_face_rec_functions.h"
#include <QImage>                     // Qt图像类，用于图像处理
#include <QPixmap>                    // Qt像素图类，用于图像显示
#include <opencv2/opencv.hpp>         // OpenCV核心库
#include <QDebug>                     // Qt调试输出类
#include <QLabel>
#include <algorithm>
#include <chrono>
face_rec_functions::face_rec_functions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::face_rec_functions)
{
    //ui->setupUi(this);               // 设置UI界面，将界面控件与当前Widget对象关联
    // 加载Haar Cascade人脸检测分类器
   cap_init();
}

face_rec_functions::~face_rec_functions()
{
    stopProcessing();
    if (cap.isOpened()) {             // 检查摄像头是否已打开
        cap.release();                 // 释放摄像头资源，关闭视频流
    }
    delete ui;
}


void face_rec_functions::cap_init(){
    faceCascadeReady = loadFaceCascade("/root/haarcascade_frontalface_default.xml");
    if (!faceCascadeReady) {
        qDebug() << "人脸分类器加载失败，请检查模型路径";
    }

    // 使用OpenCV打开摄像头
    cap.open(0);                      // 打开默认摄像头，0表示第一个可用摄像头，也可以使用"/dev/video0"等设备路径
    if (!cap.isOpened()) {            // 检查摄像头是否成功打开
        qDebug() << "打开摄像头失败";  // 输出错误信息
    } else {
        // 设置摄像头分辨率
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
        cap.set(cv::CAP_PROP_BUFFERSIZE, 1);       // 降低缓冲，减少画面延迟
        qDebug() << "摄像头打开成功";  // 输出成功信息
    }

    // 从ONNX文件加载深度学习模型，用于人脸特征提取
    // 参数: ONNX模型文件路径
    faceNet = cv::dnn::readNetFromONNX("/mnt/sd/recface_onnx/zeng_sim.onnx");

}

void face_rec_functions::startProcessing()
{
    if (!cap.isOpened() || running) {
        return;
    }
    running = true;
    workerThread = std::thread(&face_rec_functions::processingLoop, this);
}

void face_rec_functions::stopProcessing()
{
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

QImage face_rec_functions::cvMatToQImage(const cv::Mat& mat) {
    switch (mat.type()) {             // 根据Mat的数据类型进行不同处理
        case CV_8UC4: {               // 4通道8位无符号整数（BGRA格式）
            // 创建QImage，直接使用Mat的数据指针，避免数据拷贝
            // mat.data: 图像数据指针
            // mat.cols: 图像宽度（列数）
            // mat.rows: 图像高度（行数）
            // mat.step: 每行的字节数（步长）
            // QImage::Format_RGB32: 32位RGB格式
            QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB32);
            return image.rgbSwapped().copy();  // 交换RGB通道（OpenCV是BGR，Qt是RGB），并复制数据
        }
        case CV_8UC3: {               // 3通道8位无符号整数（BGR格式）
            // 创建RGB888格式的QImage
            QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
            return image.rgbSwapped().copy();  // 交换RGB通道并复制数据
        }
        case CV_8UC1: {               // 单通道8位无符号整数（灰度图）
            // 创建灰度图格式的QImage
            QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
            return image.copy();      // 灰度图不需要交换通道，直接复制
        }
        default:                       // 其他不支持的格式
            qDebug() << "不支持的图像格式";  // 输出错误信息
            return QImage();           // 返回空的QImage对象
    }
}





// 加载Haar Cascade人脸检测分类器
// 参数: path - XML分类器文件的路径（QString类型）
// 返回: 加载是否成功（true表示成功，false表示失败）
bool face_rec_functions::loadFaceCascade(const QString &path) {
    // 将QString转换为std::string，然后加载分类器文件
    return face_cascade.load(path.toStdString());
}
// 定时器触发的更新函数，获取视频帧、检测人脸并显示
// 该函数每30毫秒被调用一次，实现实时视频显示
bool face_rec_functions::updateFrame(QLabel* image)
{
    QImage img;
    bool detected = false;
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        img = lastImage;
        detected = hasFace;
    }

    if (!img.isNull()) {
        image->setPixmap(QPixmap::fromImage(img));
    }
    return detected;
}

void face_rec_functions::processingLoop()
{
    int detectCounter = 0;
    const int detectInterval = 10;

    while (running) {
        cv::Mat frame;
        if (!cap.read(frame) || frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        std::vector<cv::Rect> detectedFaces;
        const bool doDetect = (++detectCounter % detectInterval == 0);
        if (doDetect && faceCascadeReady) {
            cv::Mat smallFrame;
            //cv::resize(frame, smallFrame, cv::Size(320, 240));

            cv::Mat matGray;
            cv::cvtColor(frame, matGray, cv::COLOR_BGR2GRAY);

            face_cascade.detectMultiScale(matGray, detectedFaces, 1.2, 5, 0, cv::Size(60, 60));

            const double scaleX = double(frame.cols) / frame.cols;
            const double scaleY = double(frame.rows) / frame.rows;
            for (auto &face : detectedFaces) {
                face.x = int(face.x * scaleX);
                face.y = int(face.y * scaleY);
                face.width = int(face.width * scaleX);
                face.height = int(face.height * scaleY);
                face &= cv::Rect(0, 0, frame.cols, frame.rows);
            }
        } else {
            std::lock_guard<std::mutex> lock(frameMutex);
            detectedFaces = faces;
        }

        for (const auto &face : detectedFaces) {
            cv::rectangle(frame, face, cv::Scalar(0, 0, 255), 2);
        }

        const QImage img = cvMatToQImage(frame);
        if (img.isNull()) {
            continue;
        }

        const bool hasFaceNow = !detectedFaces.empty();
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            faces = std::move(detectedFaces);
            lastFrame = frame.clone();
            lastImage = img;
            hasFace = hasFaceNow;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}
// 从人脸图像中提取特征向量（使用深度学习模型）
// 参数: faceImg - 输入的人脸图像（OpenCV Mat格式）
// 返回: 归一化后的特征向量（Mat格式）
cv::Mat face_rec_functions::getFaceFeature(const cv::Mat& faceImg) {
    cv::Mat input;                    // 创建输入图像对象
    // 将人脸图像调整到模型要求的输入尺寸（112x112）
    cv::resize(faceImg, input, cv::Size(112, 112));

    // 将像素值从[0,255]归一化到[0,1]范围
    // CV_32F: 32位浮点数类型
    // 1.0/255: 归一化系数
    input.convertTo(input, CV_32F, 1.0/255);

    // 将图像转换为DNN模型需要的blob格式（批量、通道、高、宽）
    // 参数说明:
    // input: 输入图像
    // 1.0: 缩放因子（已归一化，所以为1.0）
    // cv::Size(112,112): 目标尺寸
    // cv::Scalar(0,0,0): 均值（已归一化，所以为0）
    // true: swapRB=true，交换红蓝通道（OpenCV是BGR，模型可能需要RGB）
    // false: crop=false，不裁剪图像
    cv::Mat blob = cv::dnn::blobFromImage(input, 1.0, cv::Size(112,112), cv::Scalar(0,0,0), true, false);

    // 将blob设置为DNN网络的输入
    faceNet.setInput(blob);

    // 执行前向传播，获取网络输出（特征向量）
    cv::Mat feat = faceNet.forward();

    // L2归一化特征向量，使特征向量的模长为1
    // 归一化后，两个向量的点积等于它们的余弦相似度
    cv::normalize(feat, feat);

    // 返回特征向量的副本（避免返回局部变量的引用）
    return feat.clone();
}



// 比对按钮点击事件处理函数，将当前人脸与已注册的人脸进行比对
// 该函数会计算相似度并判断是否为同一人
int face_rec_functions::features_capture(cv::Mat& mat){
    std::vector<cv::Rect> localFaces;
    cv::Mat frame;
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        localFaces = faces;
        frame = lastFrame.clone();
    }

    if (localFaces.empty()) {
        qDebug() << "未检测到人脸，无法采集";  // 输出提示信息
        return -1;                      // 提前返回，不执行采集操作
    }


    // 从检测到的多个人脸中选择面积最大的人脸（通常是最清晰、最正面的）
    // std::max_element: 查找最大元素
    // faces.begin(), faces.end(): 查找范围
    // lambda表达式: 比较两个人脸的面积，返回面积较小的（用于找最大）
    auto maxFace = std::max_element(localFaces.begin(), localFaces.end(),
        [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });

    // 检查是否找到最大人脸（理论上不会为空，但安全起见进行检查）
    if (maxFace == localFaces.end()) return -1;

    // 复用最近一帧，避免采集时再次读摄像头造成卡顿
    if (frame.empty()) {
        qDebug() << "当前没有可用视频帧";  // 输出错误信息
        return -1;                        // 提前返回
    }

    // 裁剪人脸区域
    // 创建人脸矩形区域对象
    cv::Rect faceRect(maxFace->x, maxFace->y, maxFace->width, maxFace->height);
    faceRect &= cv::Rect(0, 0, frame.cols, frame.rows);
    if (faceRect.width <= 0 || faceRect.height <= 0) {
        return -1;
    }
    // 从原图中提取人脸区域并克隆（避免引用原图数据）
    cv::Mat faceMat = frame(faceRect).clone();
    // 调整人脸图像尺寸到112x112（模型输入要求）
    cv::resize(faceMat, faceMat, cv::Size(112, 112));

    // 提取人脸特征向量
    cv::Mat feat = getFaceFeature(faceMat);

    // 将特征向量添加到已注册特征列表（克隆数据，避免引用局部变量）
    mat = feat.clone();                   // 增加采集计数
    qDebug() << "采集成功";  // 输出成功信息
    return 1;
}

double face_rec_functions::face_recongize(cv::Mat& mat)
{
    // 检查是否已完成足够数量的采集（需要采集3张才能比对）


    // 检查是否检测到人脸
    std::vector<cv::Rect> localFaces;
    cv::Mat frame;
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        localFaces = faces;
        frame = lastFrame.clone();
    }

    if (localFaces.empty()) {
        qDebug() << "未检测到人脸，无法比对";  // 输出提示信息
        return -1;                        // 提前返回
    }

    // 从检测到的多个人脸中选择面积最大的人脸
    auto maxFace = std::max_element(localFaces.begin(), localFaces.end(),
        [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });
    // 检查是否找到最大人脸
    if (maxFace == localFaces.end()) return -1;

    // 复用最近一帧，避免重复读摄像头带来的卡顿
    if (frame.empty()) {
        qDebug() << "当前没有可用视频帧";  // 输出错误信息
        return -1;                        // 提前返回
    }

    // 裁剪人脸区域
    // 创建人脸矩形区域对象
    cv::Rect faceRect(maxFace->x, maxFace->y, maxFace->width, maxFace->height);
    faceRect &= cv::Rect(0, 0, frame.cols, frame.rows);
    if (faceRect.width <= 0 || faceRect.height <= 0) {
        return -1;
    }
    // 从原图中提取人脸区域并克隆
    cv::Mat faceMat = frame(faceRect).clone();
    // 调整人脸图像尺寸到112x112（模型输入要求）
    cv::resize(faceMat, faceMat, cv::Size(112, 112));

    // 提取当前人脸的特征向量
    cv::Mat feat = getFaceFeature(faceMat);

    // 与所有已注册的特征向量进行比对，找出最大相似度
    double maxSim = -1;               // 初始化最大相似度为-1
    // 遍历所有已注册的特征向量
//    for (const auto& regFeat : registeredFeatures) {
//        // 计算余弦相似度（由于特征向量已L2归一化，点积等于余弦相似度）
//        // dot(): 计算两个向量的点积
//        double sim = feat.dot(regFeat);
//        // 更新最大相似度
//        if (sim > maxSim) maxSim = sim;
//    }
    double sim = feat.dot(mat);

    // 输出最大相似度值
    qDebug() << "最大相似度:" << sim;

    // 根据相似度阈值判断是否为同一人
    // 阈值0.5是经验值，可根据实际应用调整

    return sim;
}
