#ifndef AV_MODULE_ENGINE_H
#define AV_MODULE_ENGINE_H

#include <QObject>
#include <QImage>
#include <QScopedPointer>
#include <QStringList>

#include "av_types.h"

namespace avcall {

/**
 * @brief The single public entry point for audio/video calling.
 *
 * Usage:
 *   avcall::AvEngine engine;
 *   engine.setConfig({...});
 *   engine.start();
 *   engine.call("bob");
 *
 * All internal wiring (signaling, RTP, capture, playback, codec) is hidden.
 */
class AvEngine : public QObject
{
    Q_OBJECT
public:
    explicit AvEngine(QObject *parent = nullptr);
    ~AvEngine() override;

    // ---- Configuration (set before start) ----
    void    setConfig(const AvConfig &config);
    AvConfig config() const;

    // ---- Lifecycle ----
    bool start();
    void stop();
    bool isRunning() const;

    // ---- Call control ----
    void call(const QString &peerId);
    void accept();
    void reject(const QString &reason = QStringLiteral("busy"));
    void hangup();
    CallState callState() const;

    // ---- Peer discovery ----
    void queryPeerList();

signals:
    // Lifecycle
    void started();
    void stopped();
    void error(const QString &message);

    // Call
    void incomingCall(const QString &peerId);
    void callStateChanged(avcall::CallState state);
    void callEnded();

    // Video frames (connect to VideoWidget::setFrame or custom renderer)
    void localVideoFrame(const QImage &frame);
    void remoteVideoFrame(const QImage &frame);

    // Peer list query result
    void peerListReceived(const QStringList &peerIds);

    // Diagnostic log
    void logMessage(const QString &msg);

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace avcall

#endif // AV_MODULE_ENGINE_H
