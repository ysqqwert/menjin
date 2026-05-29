#include "signaling_client.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QMutexLocker>
#include <cstring>

#ifdef Q_OS_LINUX
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace avcall {

SignalingClient::SignalingClient(QObject *parent)
    : QObject(parent)
{
}

SignalingClient::~SignalingClient()
{
    stopReader();
}

bool SignalingClient::connectToServer(const QString &host, quint16 port)
{
#ifndef Q_OS_LINUX
    Q_UNUSED(host)
    Q_UNUSED(port)
    emit logMessage("signaling client only supports Linux");
    return false;
#else
    if (m_fd >= 0 && m_running.load())
        return true;

    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        emit logMessage(QString("socket create failed: errno=%1").arg(errno));
        return false;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    const QByteArray ip = host.toLocal8Bit();
    if (::inet_pton(AF_INET, ip.constData(), &addr.sin_addr) != 1) {
        emit logMessage(QString("invalid ipv4: %1").arg(host));
        ::close(fd);
        return false;
    }

    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
        emit logMessage(QString("connect failed: errno=%1").arg(errno));
        ::close(fd);
        return false;
    }

    m_fd = fd;
    m_running.store(true);
    m_thread = std::thread(&SignalingClient::readerLoop, this);
    emit connected();
    return true;
#endif
}

void SignalingClient::disconnectFromServer()
{
    stopReader();
}

bool SignalingClient::isConnected() const
{
    return m_fd >= 0 && m_running.load();
}

QString SignalingClient::localIp() const
{
#ifndef Q_OS_LINUX
    return QString();
#else
    if (m_fd < 0)
        return QString();

    sockaddr_in local;
    socklen_t len = sizeof(local);
    memset(&local, 0, sizeof(local));
    if (::getsockname(m_fd, reinterpret_cast<sockaddr *>(&local), &len) != 0)
        return QString();
    //拿到这个 socket 绑定的本地 IP + 端口

    char buf[INET_ADDRSTRLEN] = {};
    if (!::inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf)))
        return QString();


    return QString::fromLocal8Bit(buf);
#endif
}

bool SignalingClient::sendJson(const QJsonObject &obj)
{
    if (m_fd < 0 || !m_running.load()) {
        emit logMessage("send failed: not connected");
        return false;
    }

    const QByteArray bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact) + "\n";

#ifdef Q_OS_LINUX
    QMutexLocker locker(&m_sendMutex);
    qint64 total = 0;
    while (total < bytes.size()) {
        const ssize_t n = ::send(m_fd, bytes.constData() + total,
                                 bytes.size() - total, 0);
        if (n <= 0) {
            emit logMessage(QString("send failed: errno=%1").arg(errno));
            return false;
        }
        total += n;
    }
#endif
    return true;
}

void SignalingClient::stopReader()
{
    if (!m_running.load()) {
        if (m_thread.joinable())
            m_thread.join();
#ifdef Q_OS_LINUX
        if (m_fd >= 0) { ::close(m_fd); m_fd = -1; }
#endif
        return;
    }

    m_running.store(false);
#ifdef Q_OS_LINUX
    if (m_fd >= 0)
        ::shutdown(m_fd, SHUT_RDWR);
#endif

    if (m_thread.joinable())
        m_thread.join();

#ifdef Q_OS_LINUX
    if (m_fd >= 0) { ::close(m_fd); m_fd = -1; }
#endif

    emit disconnected();
}

void SignalingClient::readerLoop()
{
#ifndef Q_OS_LINUX
    return;
#else
    char buf[4096];
    while (m_running.load()) {
        const ssize_t n = ::recv(m_fd, buf, sizeof(buf), 0);
        if (n == 0)
            break;
        if (n < 0) {
            if (!m_running.load()) break;
            if (errno == EINTR) continue;
            emit logMessage(QString("recv failed: errno=%1").arg(errno));
            break;
        }

        m_buffer.append(buf, static_cast<int>(n));

        int idx = m_buffer.indexOf('\n');
        while (idx >= 0) {
            const QByteArray line = m_buffer.left(idx).trimmed();
            m_buffer.remove(0, idx + 1);
            if (!line.isEmpty())
                handleLine(line);
            idx = m_buffer.indexOf('\n');
        }
    }
    m_running.store(false);
#endif
}

void SignalingClient::handleLine(const QByteArray &line)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject())
        emit jsonReceived(doc.object());
    else
        emit logMessage("invalid signaling JSON received");
}

} // namespace avcall
