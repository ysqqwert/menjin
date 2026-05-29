# ═══════════════════════════════════════════════════════════════
#  av_module.pri — Audio/Video Calling SDK
#
#  Usage in your .pro:
#    include(av_module/av_module.pri)
#
#  Depends on: jrtplib (libjrtp-dev), ALSA (libasound2-dev)
# ═══════════════════════════════════════════════════════════════

DEFINES += ENABLE_JRTP

INCLUDEPATH += \
    $$PWD/include \
    $$PWD/src

# ---- Public headers (for consumer) ----
HEADERS += \
    $$PWD/include/av_types.h \
    $$PWD/include/av_engine.h \
    $$PWD/include/video_widget.h

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
    $$PWD/src/alsa_player.cpp

# ---- Platform libraries ----
unix:!macx {
    # Optional for cross-compile:
    #   qmake "AV_SYSROOT=/path/to/sysroot"
    #   qmake "AV_EXTRA_INCLUDEPATH=/path1 /path2" "AV_EXTRA_LIBPATH=/lib1 /lib2"
    !isEmpty(AV_SYSROOT) {
        INCLUDEPATH += \
            $$AV_SYSROOT/usr/include \
            $$AV_SYSROOT/usr/include/jrtplib3

        LIBS += -L$$AV_SYSROOT/usr/lib -L$$AV_SYSROOT/lib
    }

    !isEmpty(AV_EXTRA_INCLUDEPATH) {
        for(path, AV_EXTRA_INCLUDEPATH) {
            INCLUDEPATH += $$path
        }
    }

    !isEmpty(AV_EXTRA_LIBPATH) {
        for(path, AV_EXTRA_LIBPATH) {
            LIBS += -L$$path
        }
    }

    LIBS += -ljrtp -lasound -lpthread
}
