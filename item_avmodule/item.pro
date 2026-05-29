QT       += core gui quickwidgets quick qml virtualkeyboard

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    alsa_capturer.cpp \
    alsa_player.cpp \
    audio_pipeline.cpp \
    av_engine.cpp \
    call_state_machine.cpp \
    face_rec_functions.cpp \
    main.cpp \
    pcmu_codec.cpp \
    rtp_session.cpp \
    signaling_client.cpp \
    users.cpp \
    v4l2_capturer.cpp \
    video_pipeline.cpp \
    widget.cpp

HEADERS += \
    alsa_capturer.h \
    alsa_player.h \
    audio_pipeline.h \
    av_engine.h \
    av_engine_p.h \
    av_types.h \
    call_state_machine.h \
    face_rec_functions.h \
    pcmu_codec.h \
    rtp_session.h \
    signaling_client.h \
    signaling_protocol.h \
    users.h \
    v4l2_capturer.h \
    video_pipeline.h \
    video_widget.h \
    widget.h

FORMS += \
    face_rec_functions.ui \
    users.ui \
    widget.ui

INCLUDEPATH += /home/ysq/Desktop/jthread/build_arm/install/include
LIBS += -L/home/ysq/Desktop/jthread/build_arm/install/lib -ljthread



#INCLUDEPATH += /home/ysq/Desktop/jrtplib/build_arm/install_jrtp/include
#LIBS += -L/home/ysq/Desktop/jrtplib/build_arm/install_jrtp/lib -ljrtp

INCLUDEPATH += /home/ysq/Desktop/jrtplib/build_arm1/install/include
LIBS += -L/home/ysq/Desktop/jrtplib/build_arm1/install/lib -ljrtp

#INCLUDEPATH += -I /home/meetyoo/t113/Tina-Linux/out/t113-bingpi_m2/staging_dir/target/usr/include/
#LIBS +=  -L /home/meetyoo/t113/Tina-Linux/out/t113-bingpi_m2/staging_dir/target/usr/lib/ -lasound
INCLUDEPATH += -I /home/ysq/target_t113_user/include
LIBS +=  -L /home/ysq/target_t113_user/lib/ -lasound


#INCLUDEPATH += /home/meetyoo/opencv_arm_build/_install/include/opencv4
#/usr/local/include/opencv4

# 添加OpenCV
#LIBS += -L/home/meetyoo/opencv_arm_build/_install/lib  -lopencv_videoio -lopencv_video -lopencv_core -lopencv_calib3d -lopencv_flann  \
#                -lopencv_features2d -lopencv_highgui -lopencv_face -lopencv_imgproc \
#                -lopencv_dnn -lopencv_imgcodecs   -lopencv_objdetect -lopencv_photo

INCLUDEPATH +=/home/ysq/_install/include/opencv4

LIBS += -L/home/ysq/_install/lib  -lopencv_videoio -lopencv_video -lopencv_core -lopencv_calib3d -lopencv_flann  \
-lopencv_features2d -lopencv_highgui -lopencv_face -lopencv_imgproc \
-lopencv_dnn -lopencv_imgcodecs   -lopencv_objdetect -lopencv_photo

#libgpiod
#INCLUDEPATH += /home/meetyoo/gpio/libgpiod/_install/include/
#LIBS += /home/meetyoo/gpio/libgpiod/_install/lib/libgpiod.a
INCLUDEPATH += /home/ysq/libgpiod-1.4.3/target_gpiod_so/include/
LIBS += /home/ysq/libgpiod-1.4.3/target_gpiod_so/lib/libgpiod.so
#qt
INCLUDEPATH += /home/ysq/qt-everywhere-src-5.12.9/arm-qt/include
LIBS +=  -L/home/ysq/qt-everywhere-src-5.12.9/arm-qt/lib
LIBS += -lQt5Core -lQt5Gui -lQt5Widgets -lQt5Test

LIBS += -lasound -lpthread




# Default rules for deployment.

target.path = /root
INSTALLS += target


RESOURCES += \
    pic.qrc
