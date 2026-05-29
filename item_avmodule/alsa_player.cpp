#include "alsa_player.h"

#ifdef Q_OS_LINUX
#include <alsa/asoundlib.h>
#endif

namespace avcall {

AlsaPlayer::AlsaPlayer(QObject *parent)
    : QObject(parent)
{
}

AlsaPlayer::~AlsaPlayer()
{
    close();
}

bool AlsaPlayer::open(const QString &device, int sampleRate, int channels)
{
#ifndef Q_OS_LINUX
    Q_UNUSED(device) Q_UNUSED(sampleRate) Q_UNUSED(channels)
    emit logMessage("ALSA player is only available on Linux");
    return false;
#else
    QMutexLocker locker(&m_mutex);
    if (m_pcm)
        return true;

    m_channels = channels;

    snd_pcm_t *pcm = nullptr;
    const QByteArray dev = device.toLocal8Bit();
    int rc = snd_pcm_open(&pcm, dev.constData(), SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        emit logMessage(QString("ALSA open playback failed: %1")
                            .arg(QString::fromLocal8Bit(snd_strerror(rc))));
        return false;
    }

    rc = snd_pcm_set_params(pcm,
                            SND_PCM_FORMAT_S16_LE,
                            SND_PCM_ACCESS_RW_INTERLEAVED,
                            static_cast<unsigned int>(channels),
                            static_cast<unsigned int>(sampleRate),
                            1,
                            20000); // 20 ms latency
    if (rc < 0) {
        emit logMessage(QString("ALSA set params playback failed: %1")
                            .arg(QString::fromLocal8Bit(snd_strerror(rc))));
        snd_pcm_close(pcm);
        return false;
    }

    m_pcm = pcm;
    return true;
#endif
}

void AlsaPlayer::close()
{
#ifdef Q_OS_LINUX
    QMutexLocker locker(&m_mutex);
    if (!m_pcm)
        return;
    snd_pcm_t *pcm = static_cast<snd_pcm_t *>(m_pcm);
    snd_pcm_drop(pcm);
    snd_pcm_close(pcm);
    m_pcm = nullptr;
#endif
}

bool AlsaPlayer::play(const QByteArray &pcm16le)
{
#ifndef Q_OS_LINUX
    Q_UNUSED(pcm16le)
    return false;
#else
    QMutexLocker locker(&m_mutex);
    if (!m_pcm || pcm16le.isEmpty())
        return false;

    snd_pcm_t *pcm = static_cast<snd_pcm_t *>(m_pcm);
    const int frames = pcm16le.size() / (m_channels * 2);
    int rc = snd_pcm_writei(pcm, pcm16le.constData(), static_cast<snd_pcm_uframes_t>(frames));
    if (rc == -EPIPE) {
        snd_pcm_prepare(pcm);
        return false;
    }
    if (rc < 0) {
        emit logMessage(QString("ALSA write failed: %1")
                            .arg(QString::fromLocal8Bit(snd_strerror(rc))));
        return false;
    }
    return true;
#endif
}

} // namespace avcall
