#include "rtp_session.h"

#ifndef ENABLE_JRTP
#error "RtpSession requires jrtplib. Define ENABLE_JRTP and link -ljrtp."
#endif

#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtptimeutilities.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtperrors.h>
#include <QThread>
#include <QApplication>
#include <QRandomGenerator>
namespace avcall {

// ---- Pimpl hiding jrtplib types from the header ----

class RtpSession::Impl
{
public:
    jrtplib::RTPSession session;
    QHostAddress peerIp;
    quint16 peerPort = 0;
};

// ---- RtpSession ----

RtpSession::RtpSession(QObject *parent)
    : QObject(parent)
    , m_impl(new Impl)
{
    connect(&m_timer, &QTimer::timeout, this, &RtpSession::poll);
}

RtpSession::~RtpSession()
{
    stop();
    delete m_impl;
}

bool RtpSession::start(quint16 localPort)
{


    //printf("dfgdfgregfhgfhgh111");
    if (m_started)
        return true;

    jrtplib::RTPUDPv4TransmissionParams trans;
    trans.SetPortbase(localPort);
    const uint32_t bindIp = QHostAddress("192.168.10.50").toIPv4Address();
    trans.SetBindIP(bindIp);
     //printf("dgergrgtrhrt");
    jrtplib::RTPSessionParams params;
    params.SetOwnTimestampUnit(1.0 / 90000.0);
    params.SetAcceptOwnPackets(false);
    params.SetResolveLocalHostname(false);
    const QString cname = QString("avmodule-%1-%2@local")
                              .arg(QCoreApplication::applicationPid())
                              .arg(localPort);
    params.SetCNAME(cname.toStdString());
    params.SetUsePredefinedSSRC(true);
    params.SetPredefinedSSRC(QRandomGenerator::global()->generate());
//    qDebug()<<"ergergryyyyy";
//    printf("dgergrgtrhrt");
    const int rc = m_impl->session.Create(params, &trans);
    if (rc < 0) {
           emit logMessage(QString("RTP create failed: %1 (%2)")
                               .arg(rc)
                               .arg(jrtplib::RTPGetErrorString(rc).c_str()));
//        qDebug()<<QString("RTP create failed: %1 (%2)")
//                  .arg(rc)
//                  .arg(jrtplib::RTPGetErrorString(rc).c_str());
           return false;
       }

    m_started = true;
    m_timer.start(5);
    return true;
}

void RtpSession::stop()
{
    if (!m_started)
        return;
    m_timer.stop();
    m_impl->session.BYEDestroy(jrtplib::RTPTime(0, 0), "avcall-stop", 11);
    m_started = false;
}

bool RtpSession::isStarted() const
{
    return m_started;
}

void RtpSession::setPeer(const QHostAddress &ip, quint16 port)
{
    m_impl->peerIp   = ip;
    m_impl->peerPort = port;

    qDebug()<<ip.toString()<<port;
    m_impl->session.ClearDestinations();
    const uint32_t ipv4 = ip.toIPv4Address();
    jrtplib::RTPIPv4Address addr(ipv4, port);
    m_impl->session.AddDestination(addr);
}

bool RtpSession::sendVideo(const QByteArray &mjpeg, uint32_t ts)
{
    return sendPacket(26, mjpeg, ts, false);

}

bool RtpSession::sendAudio(const QByteArray &pcmu, uint32_t ts)
{
    return sendPacket(0, pcmu, ts, false);
}



bool RtpSession::sendPacket(int pt, const QByteArray &data, uint32_t tsInc, bool marker)
{
    //qDebug()<<"arm                        send";
    if (!m_started || m_impl->peerPort == 0 || data.isEmpty())
        return false;

    static const int kMaxChunk = 1200;
    int offset = 0;
    int packetCount = 0;

    while (offset < data.size()) {
        const int chunk = qMin(kMaxChunk, data.size() - offset);
        const bool m    = (offset + chunk >= data.size());
        const auto *ptr = (const uint8_t *)data.constData() + offset;

        // ========================
        // ✅ 修复：所有分片时间戳相同！
        // ========================
        uint32_t ts = tsInc;  // <-- 就改这一行！

        int rc = m_impl->session.SendPacket(ptr, chunk, pt, m, ts);
        if (rc < 0) {
            emit logMessage(QString("RTP send failed: %1").arg(rc));
            return false;
        }

        offset += chunk;
        packetCount++;

        // 删掉这个延迟！会导致时间戳错乱！
        //QThread::usleep(5000);

    }

    if (pt == 26) {
        //qDebug() << "发送视频帧: 总大小=" << data.size() << "分片数=" << packetCount;
    }

    return true;
}

void RtpSession::poll()
{

    m_impl->session.Poll();
    m_impl->session.BeginDataAccess();
    if (m_impl->session.GotoFirstSourceWithData()) {
        do {
            //qDebug()<<"nnnnnnnnnnn";
            jrtplib::RTPPacket *pkt = nullptr;
            while ((pkt = m_impl->session.GetNextPacket()) != nullptr) {
                const QByteArray payload(
                    reinterpret_cast<const char *>(pkt->GetPayloadData()),
                    static_cast<int>(pkt->GetPayloadLength()));
                const uint32_t ts  = pkt->GetTimestamp();
                const bool     mk  = pkt->HasMarker();
                const uint16_t seq = pkt->GetSequenceNumber();
                const int      pt  = pkt->GetPayloadType();

                //qDebug() << "RTP pkt pt=" << pt
                //         << "seq=" << seq
                //         << "len=" << payload.size()
                 //        << "ts=" << ts
                  //       << "mk=" << mk
                  //       << "ssrc=" << pkt->GetSSRC();
                if (pt == 26){

                    emit videoReceived(payload, ts, mk, seq);
                    //qDebug()<<payload.size()<<"remote video received";
                }
                else if (pt == 0){
                    emit audioReceived(payload, ts, mk, seq);
                    //qDebug()<<payload.size()<<"remote audio received";
                }

                m_impl->session.DeletePacket(pkt);
            }
        } while (m_impl->session.GotoNextSourceWithData());
    }
    m_impl->session.EndDataAccess();
}

} // namespace avcall
