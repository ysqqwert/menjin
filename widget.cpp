#include "widget.h"
#include "ui_widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>   // gettimeofday 所在头文件
#include <time.h>
#include<QTimer>
#include <QMessageBox>
#include <QThread>
#include <QVBoxLayout>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    previewLabel = new QLabel(ui->widget);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setScaledContents(true);
    QVBoxLayout *previewLayout = new QVBoxLayout(ui->widget);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->addWidget(previewLabel);

    timer = new QTimer(this);
    previewTimer = new QTimer(this);


    connect(timer,&QTimer::timeout,this,[=](){
        struct timeval tv;
        struct tm*     time_info;

        // 1. 获取当前时间（秒 + 微秒）
        gettimeofday(&tv, NULL);

        // 2. 转为本地时间（年、月、日、时、分、秒）
        time_info = localtime(&tv.tv_sec);

        // 3. 直接输出
        int year   = time_info->tm_year + 1900;  // 年要 +1900
        int month  = time_info->tm_mon + 1;      // 月要 +1
        int day    = time_info->tm_mday;         // 日
        int hour   = time_info->tm_hour;         // 时
        int min    = time_info->tm_min;          // 分
        int sec    = time_info->tm_sec;          // 秒
        ui->label->setText(QString("%1: %2: %3").arg(hour).arg(min).arg(sec));
        ui->label_2->setText(QString("%1-%2-%3").arg(year).arg(month).arg(day));

    });
    timer->start(1000);

    connect(previewTimer, &QTimer::timeout, this, [this]() {
        if (!users || !users->rec || !previewLabel) {
            return;
        }
        users->rec->updateFrame(previewLabel);
    });
}

Widget::~Widget()
{
    delete ui;
}


void Widget::on_pushButton_clicked()
{
    if (!users) {
        users = new Users();
        connect(users, &Users::out, this, [this]() {
            if (users) {
                users->stopCamera();
            }
            delete users;
            users = nullptr;
            this->show();
        });
    }

    this->hide();
    users->show();

}

void Widget::on_pushButton_3_clicked()
{
    if (!users) {
        users = new Users();
        connect(users, &Users::out, this, [this]() {
            if (users) {
                users->stopCamera();
            }
            delete users;
            users = nullptr;
            this->show();
        });
    }

    if (users->users_v.empty()) {
        QMessageBox::warning(this, "人脸识别", "数据库中没有用户，请先录入用户信息。");
        return;
    }


    users->startCamera();
    if (previewTimer && !previewTimer->isActive()) {
            previewTimer->start(66);
        }

    cv::Mat currentFeat;
    bool captured = false;
    for (int retry = 0; retry < 20; ++retry) {
        QThread::msleep(100);
        if (!users || !users->rec) {
            break;
        }
        if (users->rec->features_capture(currentFeat) > 0 && !currentFeat.empty()) {
            captured = true;
            break;
        }
    }

    if (!captured) {
        QMessageBox::warning(this, "人脸识别", "未检测到有效人脸，请正对摄像头后重试。");
        return;
    }

    constexpr double kRecognitionThreshold = 0.5;
    double bestSim = -1.0;
    QString bestName;
    QString bestNumber;

    for (const auto &dbUser : users->users_v) {
        for (const auto &dbFeat : dbUser.features) {
            if (dbFeat.empty() || dbFeat.total() != currentFeat.total()) {
                continue;
            }
            const double sim = currentFeat.dot(dbFeat);
            if (sim > bestSim) {
                bestSim = sim;
                bestName = dbUser.name;
                bestNumber = dbUser.number;
            }
        }
    }

    if (bestSim < 0.0) {
        QMessageBox::warning(this, "人脸识别", "数据库中没有可用的人脸特征。");
        return;
    }

    if (bestSim >= kRecognitionThreshold) {
        QMessageBox::information(
            this,
            "识别成功",
            QString("识别到用户：%1\n工号：%2\n相似度：%3")
                .arg(bestName, bestNumber)
                .arg(bestSim, 0, 'f', 3));
    } else {
        QMessageBox::information(
            this,
            "识别失败",
            QString("未匹配到已登记用户。\n最高相似度：%1（阈值：%2）")
                .arg(bestSim, 0, 'f', 3)
                .arg(kRecognitionThreshold, 0, 'f', 3));
    }
}