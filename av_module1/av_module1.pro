QT       += core gui network widgets








TARGET = avmodule












DEFINES += ENABLE_JRTP

INCLUDEPATH += \
    $$PWD/include \
    $$PWD/src

# ---- Public headers (for consumer) ----
HEADERS += \
    $$PWD/include/av_types.h \
    $$PWD/include/av_engine.h \
    $$PWD/include/video_widget.h \
    form.h

# ---- Internal headers ----
HEADERS += \
    $$PWD/src/av_engine_p.h \
    $$PWD/src/call_state_machine.h \
    $$PWD/src/signaling_protocol.h \
    $$PWD/src/signaling_client.h \
    $$PWD/src/rtp_session.h \
    $$PWD/src/video_pipeline.h \
    $$PWD/src/audio_pipeline.h \
    $$PWD/src/pcmu_codec.h \
    $$PWD/src/v4l2_capturer.h \
    $$PWD/src/alsa_capturer.h \
    $$PWD/src/alsa_player.h

# ---- Sources ----
SOURCES += \
    $$PWD/src/av_engine.cpp \
    $$PWD/src/call_state_machine.cpp \
    $$PWD/src/signaling_client.cpp \
    $$PWD/src/rtp_session.cpp \
    $$PWD/src/video_pipeline.cpp \
    $$PWD/src/audio_pipeline.cpp \
    $$PWD/src/pcmu_codec.cpp \
    $$PWD/src/v4l2_capturer.cpp \
    $$PWD/src/alsa_capturer.cpp \
    $$PWD/src/alsa_player.cpp \
    $$PWD/src/main.cpp \
    form.cpp


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

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    form.ui
