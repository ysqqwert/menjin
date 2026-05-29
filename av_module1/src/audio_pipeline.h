#ifndef AV_MODULE_AUDIO_PIPELINE_H
#define AV_MODULE_AUDIO_PIPELINE_H

#include <QObject>
#include <QByteArray>
#include <cstdint>

namespace avcall {

class RtpSession;
class AlsaCapturer;
class AlsaPlayer;

/**
 * @brief Manages the complete audio data flow:
 *   Local:  ALSA capture → PCMU encode → RTP send
 *   Remote: RTP recv → PCMU decode → ALSA playback
 */
class AudioPipeline : public QObject
{
    Q_OBJECT
public:
    explicit AudioPipeline(QObject *parent = nullptr);
    ~AudioPipeline() override;

    void setRtpSession(RtpSession *rtp);
    bool start(const QString &device, int sampleRate, int channels, int frameMs);
    void stop();

signals:
    void logMessage(const QString &msg);

public slots:
    void onRemoteAudioPacket(QByteArray payload, uint32_t ts, bool marker, uint16_t seq);

private slots:
    void onCaptured(QByteArray pcm16le);

private:
    RtpSession   *m_rtp    = nullptr;
    AlsaCapturer *m_cap    = nullptr;
    AlsaPlayer   *m_player = nullptr;
    uint32_t m_sendTs = 0;
};

} // namespace avcall

#endif // AV_MODULE_AUDIO_PIPELINE_H
