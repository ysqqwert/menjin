#ifndef AV_MODULE_ALSA_PLAYER_H
#define AV_MODULE_ALSA_PLAYER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMutex>

namespace avcall {

class AlsaPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AlsaPlayer(QObject *parent = nullptr);
    ~AlsaPlayer() override;

    bool open(const QString &device, int sampleRate, int channels);
    void close();
    bool play(const QByteArray &pcm16le);

signals:
    void logMessage(const QString &msg);

private:
#ifdef Q_OS_LINUX
    void *m_pcm = nullptr;
#endif
    int m_channels   = 1;
    QMutex m_mutex;
};

} // namespace avcall

#endif // AV_MODULE_ALSA_PLAYER_H
