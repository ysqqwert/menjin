#ifndef AV_MODULE_TYPES_H
#define AV_MODULE_TYPES_H

#include <QString>
#include <QtGlobal>

namespace avcall {

/// Call state for the finite state machine
enum class CallState {
    Idle = 0,    // No active call
    Calling,     // Outgoing call, waiting for peer to accept
    Ringing,     // Incoming call, waiting for local user decision
    Connected    // Call established, media flowing
};

/// Configuration for AvEngine
struct AvConfig {
    // Identity
    QString userId;

    // Signaling server
    QString signalingHost = QStringLiteral("127.0.0.1");
    quint16 signalingPort = 9000;

    // RTP (single port, audio/video multiplexed by payload type)
    quint16 rtpPort = 5004;

    // Video capture
    QString videoDevice = QStringLiteral("/dev/video0");
    int videoWidth  = 320;
    int videoHeight = 240;
    int videoFps    = 15;

    // Audio capture / playback
    QString audioDevice    = QStringLiteral("default");
    int audioSampleRate    = 16000;
    int audioChannels      = 1;
    int audioFrameMs       = 20;
};

} // namespace avcall

//Q_DECLARE_METATYPE(avcall::CallState)

#endif // AV_MODULE_TYPES_H
