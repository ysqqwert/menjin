#ifndef AV_MODULE_SIGNALING_PROTOCOL_H
#define AV_MODULE_SIGNALING_PROTOCOL_H

#include <QString>
#include <QJsonObject>

namespace avcall {
namespace protocol {

// Message type constants
static const char *kTypeRegister    = "register";
static const char *kTypeListClients = "list_clients";
static const char *kTypeCall        = "call";
static const char *kTypeAccept      = "accept";
static const char *kTypeReject      = "reject";
static const char *kTypeHangup      = "hangup";
static const char *kTypeError       = "error";
static const char *kTypeAck         = "ack";

inline QJsonObject makeRegister(const QString &userId)
{
    QJsonObject obj;
    obj["type"]      = kTypeRegister;
    obj["client_id"] = userId;
    return obj;
}

inline QJsonObject makeListClients()
{
    QJsonObject obj;
    obj["type"] = kTypeListClients;
    return obj;
}

inline QJsonObject makeCall(const QString &toId, int rtpPort)
{
    QJsonObject obj;
    obj["type"]     = kTypeCall;
    obj["to"]       = toId;
    obj["rtp_port"] = rtpPort;
    return obj;
}

inline QJsonObject makeAccept(const QString &toId, const QString &localIp, int rtpPort)
{
    QJsonObject obj;
    obj["type"]     = kTypeAccept;
    obj["to"]       = toId;
    obj["peer_ip"]  = localIp;
    obj["rtp_port"] = rtpPort;
    return obj;
}

inline QJsonObject makeReject(const QString &toId, const QString &reason)
{
    QJsonObject obj;
    obj["type"]   = kTypeReject;
    obj["to"]     = toId;
    obj["reason"] = reason;
    return obj;
}

inline QJsonObject makeHangup()
{
    QJsonObject obj;
    obj["type"] = kTypeHangup;
    return obj;
}

} // namespace protocol
} // namespace avcall

#endif // AV_MODULE_SIGNALING_PROTOCOL_H
