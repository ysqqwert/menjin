#ifndef AV_MODULE_ALSA_CAPTURER_H
#define AV_MODULE_ALSA_CAPTURER_H

#include <QThread>
#include <QString>
#include <QByteArray>
#include <atomic>

namespace avcall {

class AlsaCapturer : public QThread
{
    Q_OBJECT
public:
    explicit AlsaCapturer(QObject *parent = nullptr);
    ~AlsaCapturer() override;

    bool startCapture(const QString &device, int sampleRate, int channels, int frameMs);
    void stopCapture();

signals:
    void pcmCaptured(QByteArray pcm16le);
    void logMessage(const QString &msg);

protected:
    void run() override;

private:
    QString m_device = QStringLiteral("default");
    int m_sampleRate = 16000;
    int m_channels   = 1;
    int m_frameMs    = 20;
    std::atomic<bool> m_running{false};
};

} // namespace avcall

#endif // AV_MODULE_ALSA_CAPTURER_H
