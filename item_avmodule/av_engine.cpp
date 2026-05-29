#include "av_engine_p.h"
#include "signaling_protocol.h"

#include <QJsonArray>

namespace avcall {

// ═══════════════════════════════════════════════════════════════
//  AvEngine::Private
// ═══════════════════════════════════════════════════════════════

AvEngine::Private::Private(AvEngine *q)
    : q_ptr(q)
{
    wireConnections();
}

AvEngine::Private::~Private()
{
    stop();
}

void AvEngine::Private::wireConnections()
{
    // Signaling message dispatch
    QObject::connect(&signaling, &SignalingClient::jsonReceived, q_ptr,
                     [this](const QJsonObject &obj) { onSignalingMessage(obj); });

    // Signaling log forwarding
    QObject::connect(&signaling, &SignalingClient::logMessage,
                     q_ptr, &AvEngine::logMessage);

    // Signaling disconnected → auto cleanup
    QObject::connect(&signaling, &SignalingClient::disconnected, q_ptr, [this]() {
        if (running) {
            stopMedia();
            sm.reset();
            running = false;
            emit q_ptr->stopped();
        }
    });

    // RTP → pipelines
    QObject::connect(&rtp, &RtpSession::videoReceived,
                     &video, &VideoPipeline::onRemoteVideoPacket);
    QObject::connect(&rtp, &RtpSession::audioReceived,
                     &audio, &AudioPipeline::onRemoteAudioPacket);
    QObject::connect(&rtp, &RtpSession::logMessage,
                     q_ptr, &AvEngine::logMessage);

    // Video pipeline → engine signals
    QObject::connect(&video, &VideoPipeline::localFrameReady,
                     q_ptr, &AvEngine::localVideoFrame);
    QObject::connect(&video, &VideoPipeline::remoteFrameReady,
                     q_ptr, &AvEngine::remoteVideoFrame);
    QObject::connect(&video, &VideoPipeline::logMessage,
                     q_ptr, &AvEngine::logMessage);

    // Audio pipeline log
    QObject::connect(&audio, &AudioPipeline::logMessage,
                     q_ptr, &AvEngine::logMessage);

    // State machine → engine signal
    QObject::connect(&sm, &CallStateMachine::stateChanged,
                     q_ptr, &AvEngine::callStateChanged);

    // Give pipelines access to RTP
    video.setRtpSession(&rtp);
    audio.setRtpSession(&rtp);
}

// ---- Lifecycle ----

bool AvEngine::Private::start()
{
    if (running)
        return true;

    if (config.userId.isEmpty()) {
        emit q_ptr->error("userId is empty");
        return false;
    }

    if (!rtp.start(config.rtpPort)) {
        emit q_ptr->error("RTP start failed");
        return false;
    }

    if (!signaling.connectToServer(config.signalingHost, config.signalingPort)) {
        rtp.stop();
        emit q_ptr->error("signaling connect failed");
        return false;
    }

    signaling.sendJson(protocol::makeRegister(config.userId));

    running = true;
    emit q_ptr->started();
    return true;
}

void AvEngine::Private::stop()
{
    if (!running)
        return;

    stopMedia();
    sm.reset();
    signaling.disconnectFromServer();
    rtp.stop();
    running = false;
    emit q_ptr->stopped();
}

// ---- Call control ----

void AvEngine::Private::call(const QString &peerId)
{
    if (!running || peerId.isEmpty())
        return;

    if (!sm.tryTransition(CallState::Calling)) {
        emit q_ptr->error("cannot call in current state");
        return;
    }

    signaling.sendJson(protocol::makeCall(peerId, config.rtpPort));
}

void AvEngine::Private::accept()
{
    if (!running)
        return;

    if (!sm.tryTransition(CallState::Connected)) {
        emit q_ptr->error("cannot accept in current state");
        return;
    }

    // Set RTP peer
    rtp.setPeer(QHostAddress(incomingPeerIp), incomingPeerRtpPort);

    // Notify caller
    signaling.sendJson(protocol::makeAccept(
        incomingPeerId, signaling.localIp(), config.rtpPort));

    startMedia();
}

void AvEngine::Private::reject(const QString &reason)
{
    if (!running || incomingPeerId.isEmpty())
        return;

    signaling.sendJson(protocol::makeReject(incomingPeerId, reason));
    sm.reset();
    incomingPeerId.clear();
}

void AvEngine::Private::hangup()
{
    if (!running)
        return;

    signaling.sendJson(protocol::makeHangup());
    stopMedia();
    sm.reset();
    emit q_ptr->callEnded();
}

void AvEngine::Private::queryPeerList()
{
    if (running)
        signaling.sendJson(protocol::makeListClients());
}

// ---- Signaling dispatch ----

void AvEngine::Private::onSignalingMessage(const QJsonObject &obj)
{
    const QString type = obj.value("type").toString();
    const QString from = obj.value("from").toString();

    if (type == protocol::kTypeCall) {
        incomingPeerId      = from;
        incomingPeerIp      = obj.value("peer_ip").toString();
        incomingPeerRtpPort = static_cast<quint16>(obj.value("rtp_port").toInt());

        if (sm.tryTransition(CallState::Ringing))
            emit q_ptr->incomingCall(from);
        return;
    }

    if (type == protocol::kTypeAccept) {
        const QString peerIp  = obj.value("peer_ip").toString();
        const quint16 peerPort = static_cast<quint16>(obj.value("rtp_port").toInt());
        rtp.setPeer(QHostAddress(peerIp), peerPort);

        if (sm.tryTransition(CallState::Connected))
            startMedia();
        return;
    }

    if (type == protocol::kTypeReject) {
        sm.reset();
        emit q_ptr->callEnded();
        return;
    }

    if (type == protocol::kTypeHangup) {
        stopMedia();
        sm.reset();
        emit q_ptr->callEnded();
        return;
    }

    if (type == protocol::kTypeListClients) {
        const QJsonArray arr = obj.value("clients").toArray();
        QStringList ids;
        ids.reserve(arr.size());
        for (const QJsonValue &v : arr)
            ids.append(v.toString());
        emit q_ptr->peerListReceived(ids);
        return;
    }

    if (type == protocol::kTypeError) {
        emit q_ptr->error(obj.value("message").toString());
        return;
    }
}

// ---- Media helpers ----

void AvEngine::Private::startMedia()
{
    video.start(config.videoDevice, config.videoWidth, config.videoHeight, config.videoFps);
    audio.start(config.audioDevice, config.audioSampleRate, config.audioChannels, config.audioFrameMs);
}

void AvEngine::Private::stopMedia()
{
    video.stop();
    audio.stop();
}

// ═══════════════════════════════════════════════════════════════
//  AvEngine (public facade)
// ═══════════════════════════════════════════════════════════════

AvEngine::AvEngine(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    qRegisterMetaType<avcall::CallState>("avcall::CallState");
}

AvEngine::~AvEngine() = default;

void AvEngine::setConfig(const AvConfig &config)
{
    d->config = config;
}

AvConfig AvEngine::config() const
{
    return d->config;
}

bool AvEngine::start()
{
    return d->start();
}

void AvEngine::stop()
{
    d->stop();
}

bool AvEngine::isRunning() const
{
    return d->running;
}

void AvEngine::call(const QString &peerId)
{
    d->call(peerId);
}

void AvEngine::accept()
{
    d->accept();
}

void AvEngine::reject(const QString &reason)
{
    d->reject(reason);
}

void AvEngine::hangup()
{
    d->hangup();
}

CallState AvEngine::callState() const
{
    return d->sm.state();
}

void AvEngine::queryPeerList()
{
    d->queryPeerList();
}

} // namespace avcall
