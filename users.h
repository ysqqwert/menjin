#ifndef USERS_H
#define USERS_H

#include <QWidget>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include "face_rec_functions.h"
namespace Ui {
class Users;
}
class QQuickWidget;

typedef struct{

    QString name;
    QString number;
    bool face_captured=false;
    std::vector<cv::Mat> features{3};

}users;

class Users : public QWidget
{
    Q_OBJECT

public:
    explicit Users(QWidget *parent = nullptr);
    explicit Users(QWidget* parent,QLabel* label);
    ~Users();
    void startCamera();
    void stopCamera();
    std::vector<users> users_v;
    users* current_user = nullptr;
    QTimer* timer = nullptr;
    bool faceDected=false;
    bool lastFaceDetected = false;
    face_rec_functions* rec;

signals:

    void out(void);


private slots:
    void on_register_btn_clicked();

    void on_faceCap_btn_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_chuangjian_clicked();

    void on_pushButton_zhuce_clicked();

    void on_pushButton_capture_clicked();

    void on_pushButton_tuichu_clicked();

public:
    Ui::Users *ui;
    //QQuickWidget *keyboard = nullptr;

private:
    QString userDataFilePath() const;
    void loadUsersFromDisk();
    bool saveUsersToDisk() const;
};

#endif // USERS_H
