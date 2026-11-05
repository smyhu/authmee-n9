/*
 * AuthMee for Nokia N9 - clock correction from a time server (NTP or TIME/37).
 * The system clock is never changed: the app only remembers the difference.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#ifndef AUTHMEE_CLOCK_H
#define AUTHMEE_CLOCK_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtNetwork/QHostInfo>

class QUdpSocket;
class QTcpSocket;

class Clock : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double offset READ offset NOTIFY changed)
    Q_PROPERTY(QString checkedAt READ checkedAt NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
public:
    explicit Clock(QObject *parent = 0);
    double offset() const { return m_offset; }
    QString checkedAt() const { return m_checkedAt; }
    bool busy() const { return m_busy; }

    Q_INVOKABLE double now() const;
    // protocol: 0 = NTP (UDP 123), 1 = TIME (TCP 37)
    Q_INVOKABLE void sync(const QString &server, int protocol);
    Q_INVOKABLE void reset();

signals:
    void changed();
    void busyChanged();
    void finished(bool ok, const QString &error);

private slots:
    void onHost(const QHostInfo &info);
    void onUdp();
    void onTcpConnected();
    void onTcp();
    void onTimeout();

private:
    void done(bool ok, double offset, const QString &error);
    static double wallNow();

    double m_offset;
    QString m_checkedAt;
    bool m_busy;
    int m_protocol;
    double m_sent;
    QByteArray m_tcpData;
    QUdpSocket *m_udp;
    QTcpSocket *m_tcp;
    QTimer m_timeout;
    QSettings m_settings;
};

#endif
