#ifndef AV_MODULE_SIGNALING_CLIENT_H
#define AV_MODULE_SIGNALING_CLIENT_H

#include <QObject>
#include <QJsonObject>
#include <QMutex>
#include <QByteArray>

#include <atomic>
#include <thread>

namespace avcall {

class SignalingClient : public QObject
{
    Q_OBJECT
public:
    explicit SignalingClient(QObject *parent = nullptr);
    ~SignalingClient() override;

    bool connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;
    QString localIp() const;

    bool sendJson(const QJsonObject &obj);

signals:
    void connected();
    void disconnected();
    void jsonReceived(const QJsonObject &obj);
    void logMessage(const QString &msg);

private:
    void readerLoop();
    void stopReader();
    void handleLine(const QByteArray &line);

    int m_fd = -1;
    QByteArray m_buffer;
    QMutex m_sendMutex;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
};

} // namespace avcall

#endif // AV_MODULE_SIGNALING_CLIENT_H
