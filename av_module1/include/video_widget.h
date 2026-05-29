#ifndef AV_MODULE_VIDEO_WIDGET_H
#define AV_MODULE_VIDEO_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QPainter>

namespace avcall {

/**
 * @brief A simple QWidget that displays QImage frames.
 *
 * Connect AvEngine::localVideoFrame / remoteVideoFrame to setFrame().
 */
class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_OpaquePaintEvent);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

public slots:
    void setFrame(const QImage &frame)
    {
        m_frame = frame;

        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        if (m_frame.isNull()) {
            p.fillRect(rect(), Qt::black);
            return;
        }
        QImage scaled = m_frame.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int x = (width() - scaled.width()) / 2;
        int y = (height() - scaled.height()) / 2;
        p.fillRect(rect(), Qt::black);
        p.drawImage(x, y, scaled);
    }

private:
    QImage m_frame;
};

} // namespace avcall

#endif // AV_MODULE_VIDEO_WIDGET_H
