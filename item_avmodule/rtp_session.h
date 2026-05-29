#ifndef AV_MODULE_RTP_SESSION_H
#define AV_MODULE_RTP_SESSION_H

#include <QObject>
#include <QHostAddress>
#include <QTimer>
#include <QByteArray>
#include <cstdint>

namespace avcall {

/**
 * @brief jrtplib wrapper — one session, audio/video multiplexed by payload type.
 *
 * Video: PT 26 (JPEG), 90 kHz clock.
 * Audio: PT 0  (PCMU), 8 kHz clock.
 */
class RtpSession : public QObject
{
    Q_OBJECT
public:
    explicit RtpSession(QObject *parent = nullptr);
    ~RtpSession() override;

    bool start(quint16 localPort);
    void stop();
    bool isStarted() const;

    void setPeer(const QHostAddress &ip, quint16 port);

    bool sendVideo(const QByteArray &mjpeg, uint32_t ts);
    bool sendAudio(const QByteArray &pcmu,  uint32_t ts);
    bool sendJpegPacket(const QByteArray &jpeg, uint32_t ts);

signals:
    void videoReceived(QByteArray payload, uint32_t ts, bool marker, uint16_t seq);
    void audioReceived(QByteArray payload, uint32_t ts, bool marker, uint16_t seq);
    void logMessage(const QString &msg);

private slots:
    void poll();

private:
    bool sendPacket(int pt, const QByteArray &data, uint32_t ts, bool marker);

    class Impl;
    Impl *m_impl;
    QTimer m_timer;
    bool m_started = false;
};

} // namespace avcall

#endif // AV_MODULE_RTP_SESSION_H
