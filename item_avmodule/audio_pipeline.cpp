#include "audio_pipeline.h"
#include "alsa_capturer.h"
#include "alsa_player.h"
#include "pcmu_codec.h"
#include "rtp_session.h"

namespace avcall {

AudioPipeline::AudioPipeline(QObject *parent)
    : QObject(parent)
{
}

AudioPipeline::~AudioPipeline()
{
    stop();
}

void AudioPipeline::setRtpSession(RtpSession *rtp)
{
    m_rtp = rtp;
}

bool AudioPipeline::start(const QString &device, int sampleRate, int channels, int frameMs)
{
    if (m_cap)
        return true;

    m_sendTs = 0;

    // Player (must start before capture to avoid initial latency)
    m_player = new AlsaPlayer(this);
    connect(m_player, &AlsaPlayer::logMessage, this, &AudioPipeline::logMessage);
    if (!m_player->open(device, sampleRate, channels))
        emit logMessage("warning: ALSA player not started");

    // Capturer
    m_cap = new AlsaCapturer(this);
    connect(m_cap, &AlsaCapturer::pcmCaptured, this, &AudioPipeline::onCaptured);
    connect(m_cap, &AlsaCapturer::logMessage,   this, &AudioPipeline::logMessage);
    m_cap->startCapture(device, sampleRate, channels, frameMs);
    return true;
}

void AudioPipeline::stop()
{
    if (m_cap) {
        m_cap->stopCapture();
        delete m_cap;
        m_cap = nullptr;
    }
    if (m_player) {
        m_player->close();
        delete m_player;
        m_player = nullptr;
    }
}

void AudioPipeline::onCaptured(QByteArray pcm16le)
{
    if (!m_rtp || pcm16le.isEmpty())
        return;

    const QByteArray pcmu = PcmuCodec::encode(pcm16le);
    if (pcmu.isEmpty())
        return;

    m_sendTs += static_cast<uint32_t>(pcmu.size());
    m_rtp->sendAudio(pcmu, m_sendTs);
    qDebug()<<"send audio to remoteeeeeeeeeeeeeeeeeeeeeeeeeeeeee";

}

void AudioPipeline::onRemoteAudioPacket(QByteArray payload, uint32_t /*ts*/, bool /*marker*/, uint16_t /*seq*/)
{
    if (payload.isEmpty() || !m_player)
        return;

    const QByteArray pcm16 = PcmuCodec::decode(payload);
    if (!pcm16.isEmpty())
        m_player->play(pcm16);
    qDebug()<<"receive remote audioooooooooooooooooooooooooooooooooooooooooo";
}

} // namespace avcall
