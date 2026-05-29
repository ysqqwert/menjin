#ifndef AV_MODULE_ENGINE_PRIVATE_H
#define AV_MODULE_ENGINE_PRIVATE_H

#include "av_engine.h"
#include "call_state_machine.h"
#include "signaling_client.h"
#include "rtp_session.h"
#include "video_pipeline.h"
#include "audio_pipeline.h"

namespace avcall {

/**
 * @brief Private implementation of AvEngine (Pimpl).
 *
 * NOT a QObject — all signal connections use lambdas with q_ptr as context.
 */
class AvEngine::Private
{
public:
    explicit Private(AvEngine *q);
    ~Private();

    bool start();
    void stop();

    void call(const QString &peerId);
    void accept();
    void reject(const QString &reason);
    void hangup();

    void queryPeerList();

    // --- Members ---
    AvConfig          config;
    CallStateMachine  sm;
    SignalingClient    signaling;
    RtpSession        rtp;
    VideoPipeline     video;
    AudioPipeline     audio;

    bool    running = false;
    AvEngine *q_ptr;

    // Pending incoming call
    QString  incomingPeerId;
    QString  incomingPeerIp;
    quint16  incomingPeerRtpPort = 0;

private:
    void onSignalingMessage(const QJsonObject &obj);
    void startMedia();
    void stopMedia();
    void wireConnections();
};

} // namespace avcall

#endif // AV_MODULE_ENGINE_PRIVATE_H
