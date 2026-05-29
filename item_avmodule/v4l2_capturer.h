#ifndef AV_MODULE_V4L2_CAPTURER_H
#define AV_MODULE_V4L2_CAPTURER_H

#include <QThread>
#include <QString>
#include <QByteArray>
#include <atomic>

namespace avcall {

class V4L2Capturer : public QThread
{
    Q_OBJECT
public:
    explicit V4L2Capturer(QObject *parent = nullptr);
    ~V4L2Capturer() override;

    bool startCapture(const QString &device, int width, int height, int fps);
    void stopCapture();

signals:
    void frameCaptured(QByteArray mjpeg);
    void logMessage(const QString &msg);

protected:
    void run() override;

private:
    QString m_device;
    int m_width  = 640;
    int m_height = 480;
    int m_fps    = 15;
    std::atomic<bool> m_running{false};
};

} // namespace avcall

#endif // AV_MODULE_V4L2_CAPTURER_H
