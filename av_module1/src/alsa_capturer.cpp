#include "alsa_capturer.h"

#ifdef Q_OS_LINUX
#include <alsa/asoundlib.h>
#endif

namespace avcall {

AlsaCapturer::AlsaCapturer(QObject *parent)
    : QThread(parent)
{
}

AlsaCapturer::~AlsaCapturer()
{
    stopCapture();
}

bool AlsaCapturer::startCapture(const QString &device, int sampleRate, int channels, int frameMs)
{
    if (m_running.load())
        return true;

    m_device     = device;
    m_sampleRate = sampleRate;
    m_channels   = channels;
    m_frameMs    = frameMs;
    m_running.store(true);
    start();
    return true;
}

void AlsaCapturer::stopCapture()
{
    if (!m_running.load())
        return;
    m_running.store(false);
    wait(1000);
}

void AlsaCapturer::run()
{
#ifndef Q_OS_LINUX
    emit logMessage("ALSA capture is only available on Linux");
    m_running.store(false);
    return;
#else
    snd_pcm_t *pcm = nullptr;
    const QByteArray dev = m_device.toLocal8Bit();
    int rc = snd_pcm_open(&pcm, dev.constData(), SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        emit logMessage(QString("ALSA open capture failed: %1")
                            .arg(QString::fromLocal8Bit(snd_strerror(rc))));
        m_running.store(false);
        return;
    }

    rc = snd_pcm_set_params(pcm,
                            SND_PCM_FORMAT_S16_LE,
                            SND_PCM_ACCESS_RW_INTERLEAVED,
                            static_cast<unsigned int>(m_channels),
                            static_cast<unsigned int>(m_sampleRate),
                            1,
                            static_cast<unsigned int>(m_frameMs * 1000));
    if (rc < 0) {
        emit logMessage(QString("ALSA set params capture failed: %1")
                            .arg(QString::fromLocal8Bit(snd_strerror(rc))));
        snd_pcm_close(pcm);
        m_running.store(false);
        return;
    }

    const int frames = (m_sampleRate * m_frameMs) / 1000;
    QByteArray chunk;
    chunk.resize(frames * m_channels * 2);

    while (m_running.load()) {
        rc = snd_pcm_readi(pcm, chunk.data(), static_cast<snd_pcm_uframes_t>(frames));
        if (rc == -EPIPE) {
            snd_pcm_prepare(pcm);
            continue;
        }
        if (rc < 0) {
            emit logMessage(QString("ALSA read failed: %1")
                                .arg(QString::fromLocal8Bit(snd_strerror(rc))));
            continue;
        }
        const int bytes = rc * m_channels * 2;
        if (bytes > 0)
            emit pcmCaptured(chunk.left(bytes));
    }

    snd_pcm_drop(pcm);
    snd_pcm_close(pcm);
#endif
}

} // namespace avcall
