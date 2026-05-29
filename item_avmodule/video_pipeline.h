#ifndef AV_MODULE_VIDEO_PIPELINE_H
#define AV_MODULE_VIDEO_PIPELINE_H

#include <QObject>
#include <QImage>
#include <QByteArray>
#include <cstdint>

namespace avcall {

class RtpSession;
class V4L2Capturer;

/**
 * @brief Manages the complete video data flow:
 *   Local:  V4L2 capture → MJPEG frame → RTP send + QImage decode → localFrameReady
 *   Remote: RTP recv → reassemble → MJPEG → QImage decode → remoteFrameReady
 */
class VideoPipeline : public QObject
{
    Q_OBJECT
public:
    explicit VideoPipeline(QObject *parent = nullptr);
    ~VideoPipeline() override;

    void setRtpSession(RtpSession *rtp);
    bool start(const QString &device, int w, int h, int fps);
    void stop();

signals:
    void localFrameReady(const QImage &frame);
    void remoteFrameReady(const QImage &frame);
    void logMessage(const QString &msg);

public slots:
    void onRemoteVideoPacket(QByteArray payload, uint32_t ts, bool marker, uint16_t seq);

private slots:
    void onCaptured(QByteArray mjpeg);

private:
    static QImage decodeMjpeg(const QByteArray &data);

    RtpSession   *m_rtp = nullptr;
    V4L2Capturer *m_cap = nullptr;
    uint32_t m_sendTs = 0;
    int      m_fps    = 15;

    // Remote frame reassembly
    uint32_t   m_recvTs  = 0;
    QByteArray m_recvBuf;
};

} // namespace avcall

#endif // AV_MODULE_VIDEO_PIPELINE_H
