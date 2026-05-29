#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <av_engine.h>
#include <video_widget.h>
#include <QDebug>
#include <QLabel>
#include "form.h"
// Build option (choose one):
//   DEFINES += CALLER_TEST
#define CALLEE_TEST
//#define CALLER_TEST

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.resize(500,550);
    auto *layout = new QVBoxLayout(&window);
//    auto *localView = new avcall::VideoWidget;
//    auto *remoteView = new avcall::VideoWidget;
    //layout->addWidget(localView);
    //layout->addWidget(remoteView);
    QLabel* local_label = new QLabel(&window);
    QLabel* remote_label = new QLabel(&window);
    local_label->resize(320,240);
    remote_label->resize(320,240);
    layout->addWidget(local_label);
    layout->addWidget(remote_label);


    avcall::AvEngine engine;
    avcall::AvConfig cfg;
    cfg.signalingHost = "192.168.10.200"; // Replace with your signaling server IP.
    cfg.signalingPort = 9000;


    //QObject::connect(&engine, &avcall::AvEngine::localVideoFrame,
     //                localView, &avcall::VideoWidget::setFrame);
    QObject::connect(&engine, &avcall::AvEngine::localVideoFrame,
                     local_label, [=](const QImage &frame){

        QPixmap pix = QPixmap::fromImage(frame);
        pix = pix.scaled(local_label->size(), Qt::KeepAspectRatio, Qt::FastTransformation);
        local_label->setPixmap(pix);
        //qDebug()<<"local view";
    });
    QObject::connect(&engine, &avcall::AvEngine::remoteVideoFrame,
                     remote_label, [=](const QImage &frame){
        //qDebug()<<"ergrehgtrhtrhtyh56h6thtyhj";
        QPixmap pix = QPixmap::fromImage(frame);
        pix = pix.scaled(remote_label->size(), Qt::KeepAspectRatio, Qt::FastTransformation);
        remote_label->setPixmap(pix);
    });

    //QObject::connect(&engine, &avcall::AvEngine::remoteVideoFrame,
     //                remoteView, &avcall::VideoWidget::setFrame);

    QObject::connect(&engine, &avcall::AvEngine::error,
                     &window, [](const QString &msg) {
        qWarning() << "av error:" << msg;
    });

#if defined(CALLEE_TEST)
    cfg.userId = "bob";
    cfg.rtpPort = 15000;
    cfg.videoDevice = "/dev/video0";

    engine.setConfig(cfg);

    QObject::connect(&engine, &avcall::AvEngine::incomingCall,
                     &window, [&](const QString &peer) {
        qInfo() << "incoming call from" << peer;
        engine.accept();
    });

    if (!engine.start()) {
        qWarning() << "callee start failed";
    }

#elif defined(CALLER_TEST)
    cfg.userId = "alice";
    cfg.rtpPort = 5004;
    cfg.videoDevice = "/dev/video0";

    engine.setConfig(cfg);

    QObject::connect(&engine, &avcall::AvEngine::started,
                     &window, [&]() {
        // Delay call slightly so both peers finish registration.
        QTimer::singleShot(800, &window, [&]() {
            qInfo() << "calling bob";
            engine.call("bob");
        });
    });

    if (!engine.start()) {
        qWarning() << "caller start failed";
    }

#else
    qWarning() << "define CALLER_TEST or CALLEE_TEST in your .pro";
#endif

    window.show();
    const int rc = app.exec();
    engine.stop();
    return rc;
}
