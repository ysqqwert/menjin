#include "users.h"
#include "ui_users.h"
#include <QTimer>
#include <QDebug>
#include <QQuickWidget>
#include <QDir>
#include <QStandardPaths>
Users::Users(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Users)
{
    ui->setupUi(this);
    ui->pushButton_zhuce->setDisabled(true);
    ui->lineEdit->setDisabled(true);
    ui->lineEdit_2->setDisabled(true);
    ui->pushButton_capture->setDisabled(true);
    ui->lineEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
    ui->lineEdit_2->setAttribute(Qt::WA_InputMethodEnabled,true);
    ui->label_4->setScaledContents(true);
    rec = new face_rec_functions(this);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        const bool hasFace = rec->updateFrame(ui->label_4);
        if (hasFace != lastFaceDetected) {
            ui->status_label->setText(hasFace ? QString("检测到人脸") : QString("未检测到人脸"));
            lastFaceDetected = hasFace;
        }
        faceDected = hasFace;
    });
    //startCamera();
    ui->status_label->setText(QString("请采集三次人脸"));
    //keyboard = new QQuickWidget;
    //ui->keyboard->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ui->keyboard->setAttribute(Qt::WA_TranslucentBackground);
    ui->keyboard->setClearColor(Qt::transparent);
    ui->keyboard->setAttribute(Qt::WA_AcceptTouchEvents, true); // 触摸设备重要
    ui->keyboard->setSource(QUrl(QStringLiteral("qrc:/keyboard.qml")));
    ui->keyboard->setFocusPolicy(Qt::NoFocus);
    //ui->keyboard->setFixedSize(500, 300);
    //ui->gridLayout_2->addWidget(keyboard);

    loadUsersFromDisk();

}

Users::Users(QWidget* parent,QLabel* label):QWidget(parent),
    ui(new Ui::Users)
{
    ui->setupUi(this);
    ui->pushButton_zhuce->setDisabled(true);
    rec = new face_rec_functions(this);
    timer = new QTimer(this);
    label->setScaledContents(true);
    connect(timer, &QTimer::timeout, this, [this, label]() {
        faceDected = rec->updateFrame(label);
    });
    startCamera();
    current_user = new users();
    ui->status_label->setText(QString("请采集三次人脸"));

    loadUsersFromDisk();
}

Users::~Users()
{
    stopCamera();
    delete current_user;
    current_user = nullptr;
    delete ui;
}

void Users::startCamera()
{
    if (rec) {
        rec->startProcessing();
    }
    if (timer && !timer->isActive()) {
        timer->start(33);
    }
}

void Users::stopCamera()
{
    if (timer && timer->isActive()) {
        timer->stop();
    }
    if (rec) {
        rec->stopProcessing();
    }
}



void Users::on_pushButton_chuangjian_clicked()
{
    delete current_user;
    current_user = new users();


    ui->pushButton_capture->setDisabled(false);
    ui->lineEdit->setDisabled(false);
    ui->lineEdit_2->setDisabled(false);

}

void Users::on_pushButton_zhuce_clicked()
{
    if (!current_user) {
        ui->status_label->setText("请先创建用户");
        return;
    }

    const QString name = ui->lineEdit->text().trimmed();
    const QString number = ui->lineEdit_2->text().trimmed();
    if(!name.isEmpty() && !number.isEmpty()){
        current_user->name = name;
        current_user->number = number;
        users_v.push_back(*current_user);
        saveUsersToDisk();
        qDebug()<<"注册成功";
        ui->status_label->setText("注册成功");
    }else{
        ui->status_label->setText("注册失败");
        return;
    }

    delete current_user;
    current_user = nullptr;

    if (!users_v.empty()) {
        const users& user = users_v.back();
        qDebug()<<user.name<<user.number;
    }
}

void Users::on_pushButton_capture_clicked()
{
    //采集人脸特征



        startCamera();

    cv::Mat mat;
    static int i=0;

    if(faceDected){
        if(rec->features_capture(mat)>0){
            if (!current_user) {
                ui->status_label->setText(QString("请先创建用户"));
                return;
            }
            if (i >= 0 && i < 3) {
                current_user->features[i]=mat.clone();
            } else {
                ui->status_label->setText(QString("采集次数超出范围，请重新创建用户"));
                return;
            }
            i=i+1;

            ui->status_label->setText(QString("第 %1 次采集成功").arg(i));

        }
    }else{

        ui->status_label->setText(QString("未检测到人脸"));
    }

    if(i==3){
        ui->pushButton_zhuce->setDisabled(false);
        ui->pushButton_capture->setDisabled(true);
        i = 0;
    }


}

void Users::on_pushButton_tuichu_clicked()
{
    stopCamera();
    emit out();
}

QString Users::userDataFilePath() const
{
    const QString dataDir = QStringLiteral("/mnt/sd/userdata");
       QDir dir(dataDir);
       if (!dir.exists()) {
           dir.mkpath(".");
       }
       return dir.filePath("users.yml");
}

void Users::loadUsersFromDisk()
{
    const QString path = userDataFilePath();
    cv::FileStorage fs(path.toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened()) {
        qDebug() << "未找到历史用户数据文件:" << path;
        return;
    }

    users_v.clear();
    const cv::FileNode usersNode = fs["users"];
    if (usersNode.type() != cv::FileNode::SEQ) {
        qDebug() << "用户数据格式错误:" << path;
        return;
    }

    for (const auto &node : usersNode) {
        users oneUser;
        std::string name, number;
        int faceCaptured = 0;
        node["name"] >> name;
        node["number"] >> number;
        node["face_captured"] >> faceCaptured;

        oneUser.name = QString::fromStdString(name);
        oneUser.number = QString::fromStdString(number);
        oneUser.face_captured = (faceCaptured != 0);

        oneUser.features.clear();
        const cv::FileNode featureNode = node["features"];
        if (featureNode.type() == cv::FileNode::SEQ) {
            for (const auto &feature : featureNode) {
                cv::Mat featMat;
                feature >> featMat;
                oneUser.features.push_back(featMat);
            }
        }

        while (oneUser.features.size() < 3) {
            oneUser.features.emplace_back();
        }

        users_v.push_back(std::move(oneUser));
    }

    qDebug() << "加载用户数量:" << users_v.size();
}

bool Users::saveUsersToDisk() const
{
    const QString path = userDataFilePath();
    cv::FileStorage fs(path.toStdString(), cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        qDebug() << "无法写入用户数据文件:" << path;
        return false;
    }

    fs << "users" << "[";
    for (const auto &oneUser : users_v) {
        fs << "{";
        fs << "name" << oneUser.name.toStdString();
        fs << "number" << oneUser.number.toStdString();
        fs << "face_captured" << static_cast<int>(oneUser.face_captured);
        fs << "features" << "[";
        for (const auto &featMat : oneUser.features) {
            fs << featMat;
        }
        fs << "]";
        fs << "}";
    }
    fs << "]";

    qDebug() << "用户数据已保存:" << path;
    return true;
}
