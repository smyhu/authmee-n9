/*
 * AuthMee for Nokia N9 - clock correction from a time server (NTP or TIME/37).
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#include "clock.h"
#include <QtCore/QDateTime>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QTcpSocket>

static const double EPOCH_1900 = 2208988800.0;

Clock::Clock(QObject *parent)
    : QObject(parent), m_offset(0), m_busy(false), m_protocol(0), m_sent(0), m_udp(0), m_tcp(0),
      m_settings("authmee", "authmee")
{
    m_offset = m_settings.value("clock/offset", 0.0).toDouble();
    m_checkedAt = m_settings.value("clock/checkedAt", QString()).toString();
    m_timeout.setSingleShot(true);
    connect(&m_timeout, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

double Clock::wallNow()
{
    return QDateTime::currentMSecsSinceEpoch() / 1000.0;
}

double Clock::now() const
{
    return wallNow() + m_offset;
}

void Clock::reset()
{
    m_offset = 0;
    m_checkedAt.clear();
    m_settings.setValue("clock/offset", 0.0);
    m_settings.setValue("clock/checkedAt", QString());
    emit changed();
}

void Clock::sync(const QString &server, int protocol)
{
    if (m_busy)
        return;
    m_busy = true;
    emit busyChanged();
    m_protocol = protocol;
    m_timeout.start(8000);
    QHostInfo::lookupHost(server.trimmed(), this, SLOT(onHost(QHostInfo)));
}

void Clock::onHost(const QHostInfo &info)
{
    if (!qgetenv("AUTHMEE_DEBUG").isEmpty())
        qWarning("clock: host %s -> %d addresses, error %s", qPrintable(info.hostName()), info.addresses().size(), qPrintable(info.errorString()));
    if (!m_busy)
        return;
    QHostAddress addr;
    foreach (const QHostAddress &a, info.addresses())
        if (a.protocol() == QAbstractSocket::IPv4Protocol) {
            addr = a;
            break;
        }
    if (addr.isNull()) {
        done(false, 0, "dns");
        return;
    }
    if (m_protocol == 1) {
        m_tcpData.clear();
        m_tcp = new QTcpSocket(this);
        connect(m_tcp, SIGNAL(connected()), this, SLOT(onTcpConnected()));
        connect(m_tcp, SIGNAL(readyRead()), this, SLOT(onTcp()));
        m_sent = wallNow();
        m_tcp->connectToHost(addr, 37);
        return;
    }
    m_udp = new QUdpSocket(this);
    // Qt 4 sends readyRead only for a bound socket
    m_udp->bind(0);
    connect(m_udp, SIGNAL(readyRead()), this, SLOT(onUdp()));
    QByteArray packet(48, 0);
    packet[0] = 0x1b; // LI 0, version 3, client
    m_sent = wallNow();
    if (m_udp->writeDatagram(packet, addr, 123) != 48)
        done(false, 0, "network");
}

static double ntpTime(const unsigned char *p)
{
    quint32 sec = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    quint32 frac = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
    return (double)sec - EPOCH_1900 + (double)frac / 4294967296.0;
}

void Clock::onUdp()
{
    while (m_udp && m_udp->hasPendingDatagrams()) {
        QByteArray d(int(m_udp->pendingDatagramSize()), 0);
        m_udp->readDatagram(d.data(), d.size());
        double received = wallNow();
        if (d.size() < 48)
            continue;
        const unsigned char *p = (const unsigned char *)d.constData();
        if ((p[0] & 7) != 4 || p[1] == 0) // must be a server answer with a valid stratum
            continue;
        double t2 = ntpTime(p + 32), t3 = ntpTime(p + 40);
        done(true, ((t2 - m_sent) + (t3 - received)) / 2.0, QString());
        return;
    }
}

void Clock::onTcpConnected()
{
    m_sent = wallNow();
}

void Clock::onTcp()
{
    if (!m_tcp)
        return;
    m_tcpData += m_tcp->readAll();
    if (m_tcpData.size() < 4)
        return;
    double received = wallNow();
    const unsigned char *p = (const unsigned char *)m_tcpData.constData();
    quint32 sec = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    // the server sends whole seconds only: assume the middle of that second
    double server = (double)sec - EPOCH_1900 + 0.5;
    done(true, server - (m_sent + received) / 2.0, QString());
}

void Clock::onTimeout()
{
    done(false, 0, "timeout");
}

void Clock::done(bool ok, double offset, const QString &error)
{
    if (!qgetenv("AUTHMEE_DEBUG").isEmpty())
        qWarning("clock: ok=%d offset=%.3f error=%s busy=%d", ok, offset, qPrintable(error), m_busy);
    m_timeout.stop();
    if (m_udp) { m_udp->deleteLater(); m_udp = 0; }
    if (m_tcp) { m_tcp->abort(); m_tcp->deleteLater(); m_tcp = 0; }
    if (!m_busy)
        return;
    m_busy = false;
    if (ok) {
        // differences under 0.3 s do not matter for 30 s codes
        m_offset = qAbs(offset) < 0.3 ? 0.0 : offset;
        m_checkedAt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        m_settings.setValue("clock/offset", m_offset);
        m_settings.setValue("clock/checkedAt", m_checkedAt);
        emit changed();
    }
    emit busyChanged();
    emit finished(ok, error);
}
