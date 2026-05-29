#include "video_pipeline.h"
#include "v4l2_capturer.h"
#include "rtp_session.h"
#include <QBuffer>
namespace avcall {

VideoPipeline::VideoPipeline(QObject *parent)
    : QObject(parent)
{
}

VideoPipeline::~VideoPipeline()
{
    stop();
}

void VideoPipeline::setRtpSession(RtpSession *rtp)
{
    m_rtp = rtp;
}

bool VideoPipeline::start(const QString &device, int w, int h, int fps)
{
    if (m_cap)
        return true;

    m_fps    = qMax(1, fps);
    m_sendTs = 0;
    m_recvTs = 0;
    m_recvBuf.clear();

    m_cap = new V4L2Capturer(this);
    connect(m_cap, &V4L2Capturer::frameCaptured, this, &VideoPipeline::onCaptured,Qt::QueuedConnection);
    connect(m_cap, &V4L2Capturer::logMessage,    this, &VideoPipeline::logMessage,Qt::QueuedConnection);
    m_cap->startCapture(device, w, h, fps);
    return true;
}

void VideoPipeline::stop()
{
    if (!m_cap)
        return;

    m_cap->stopCapture();
    delete m_cap;
    m_cap = nullptr;
    m_recvBuf.clear();
}

void VideoPipeline::onCaptured(QByteArray mjpeg)
{
    qDebug() << ">>> 本地捕获到帧，大小：" << mjpeg.size();

    if (m_rtp && !mjpeg.isEmpty()) {
        const uint32_t tick = static_cast<uint32_t>(90000 / m_fps);
        m_sendTs += tick;
        m_rtp->sendVideo(mjpeg, m_sendTs);
    }

    // Decode for local preview
    QImage img = decodeMjpeg(mjpeg);
    if (!img.isNull())
        emit localFrameReady(img);
}

void VideoPipeline::onRemoteVideoPacket(QByteArray payload, uint32_t ts, bool marker, uint16_t seq)
{
    qDebug() << "收到远程视频包: seq=" << seq << "ts=" << ts << "marker=" << marker << "size=" << payload.size();

    if (payload.isEmpty())
        return;

    // ========================
    // 正确逻辑：
    // 时间戳不同 = 直接开始新一帧
    // ========================
    // 只要缓存空，才记录时间戳
    if (m_recvBuf.isEmpty()) {
        m_recvTs = ts;
    }

    // 追加数据包
    m_recvBuf.append(payload);

    qDebug() << payload.size() << "remote video received";

    // 帧结束，进行解码
    if (marker) {
        qDebug() << "收到完整帧，总大小:" << m_recvBuf.size();
        qDebug() << "原始数据大小:" << m_recvBuf.size();
        qDebug() << "拼接后大小:" << m_recvBuf.size();

        QImage img = decodeMjpeg(m_recvBuf);
        bool ok = !img.isNull();
        qDebug() << "解码成功? " << (ok ? "true" : "false");

        if (ok) {
            emit remoteFrameReady(img);
            qDebug() << "远程帧解码成功并发送";
        } else {
            qDebug() << "远程帧解码失败!";
        }

        // 解码完成清空
        m_recvBuf.clear();
    }
}


QImage VideoPipeline::decodeMjpeg(const QByteArray &data)
{

        QByteArray jpeg;
        jpeg.append(data);

            QImage img;
            img.loadFromData((const uchar*)jpeg.constData(), jpeg.size(), "JPEG");
            qDebug() << "原始数据大小:" << data.size();
            qDebug() << "拼接后大小:" << jpeg.size();
            qDebug() << "解码成功?" << !img.isNull();
            return img;
    }
}

 // namespace avcall
