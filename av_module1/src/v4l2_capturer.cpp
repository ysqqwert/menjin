#include "v4l2_capturer.h"

#include <QVector>
#include <cstring>
#include <QDebug>
#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev2.h>
#endif

namespace avcall {

V4L2Capturer::V4L2Capturer(QObject *parent)
    : QThread(parent)
{
}

V4L2Capturer::~V4L2Capturer()
{
    stopCapture();
}

bool V4L2Capturer::startCapture(const QString &device, int width, int height, int fps)
{
    if (m_running.load())
        return true;

    m_device = device;
    m_width  = width;
    m_height = height;
    m_fps    = fps;
    m_running.store(true);
    start();
    return true;
}

void V4L2Capturer::stopCapture()
{
    if (!m_running.load())
        return;
    m_running.store(false);
    wait(1500);
}

void V4L2Capturer::run()
{
#ifndef Q_OS_LINUX
    emit logMessage("V4L2 capture is only available on Linux");
    m_running.store(false);
    return;
#else
    struct MmapBuf { void *start = nullptr; size_t length = 0; };

    const QByteArray dev = m_device.toLocal8Bit();
    const int fd = ::open(dev.constData(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        emit logMessage(QString("V4L2 open failed: %1").arg(m_device));
        m_running.store(false);
        return;
    }

    // Set MJPEG format
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = static_cast<quint32>(m_width);
    fmt.fmt.pix.height      = static_cast<quint32>(m_height);
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        emit logMessage("V4L2 set format failed");
        ::close(fd);
        m_running.store(false);
        return;
    }

    // Set frame rate
    v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator   = 1;
    parm.parm.capture.timeperframe.denominator = static_cast<quint32>(qMax(1, m_fps));
    ioctl(fd, VIDIOC_S_PARM, &parm);

    // Request buffers
    v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 2) {
        emit logMessage("V4L2 request buffers failed");
        ::close(fd);
        m_running.store(false);
        return;
    }

    QVector<MmapBuf> buffers(static_cast<int>(req.count));

    for (quint32 i = 0; i < req.count; ++i) {
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            emit logMessage("V4L2 query buffer failed");
            ::close(fd);
            m_running.store(false);
            return;
        }

        buffers[static_cast<int>(i)].length = buf.length;
        buffers[static_cast<int>(i)].start  =
            mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if (buffers[static_cast<int>(i)].start == MAP_FAILED) {
            emit logMessage("V4L2 mmap failed");
            ::close(fd);
            m_running.store(false);
            return;
        }
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            emit logMessage("V4L2 queue buffer failed");
            ::close(fd);
            m_running.store(false);
            return;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        emit logMessage("V4L2 stream on failed");
        for (const auto &b : buffers) {
            if (b.start && b.start != MAP_FAILED) munmap(b.start, b.length);
        }
        ::close(fd);
        m_running.store(false);
        return;
    }

    while (m_running.load()) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        timeval tv = { 1, 0 };

        if (select(fd + 1, &fds, nullptr, nullptr, &tv) <= 0)
            continue;

        v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
            continue;

        if (buf.index < req.count && buf.bytesused > 0) {
            const char *src = static_cast<const char *>(buffers[static_cast<int>(buf.index)].start);
            emit frameCaptured(QByteArray(src, static_cast<int>(buf.bytesused)));
            //qDebug()<<"video capture local"<<buf.bytesused;
            msleep(20);
        }
        ioctl(fd, VIDIOC_QBUF, &buf);
        //qDebug()<<"VIDIOC_QBUF";

    }

    ioctl(fd, VIDIOC_STREAMOFF, &type);
    for (const auto &b : buffers) {
        if (b.start && b.start != MAP_FAILED) munmap(b.start, b.length);
    }
    ::close(fd);
#endif
}

} // namespace avcall
