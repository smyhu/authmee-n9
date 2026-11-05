/*
 * AuthMee for Nokia N9 - one-time codes (RFC 4226 / RFC 6238) and otpauth links.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#ifndef AUTHMEE_OTP_H
#define AUTHMEE_OTP_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QList>

struct Account {
    QString issuer;     // service name shown in the list (user can rename it)
    QString name;       // account / login
    QByteArray secret;  // raw key bytes
    QString algorithm;  // SHA1, SHA256, SHA512
    int digits;         // 6..8
    int period;         // seconds, usually 30
    Account() : algorithm("SHA1"), digits(6), period(30) {}
};

namespace Otp {
QByteArray base32Decode(const QString &text, bool *ok);
QString base32Encode(const QByteArray &data);
QString code(const Account &a, qint64 unixSeconds);
QString toUri(const Account &a);
// Parses one otpauth://totp link or an otpauth-migration link (Google Authenticator export).
// Returns the accounts found; error gets a short English reason when nothing was found.
QList<Account> parse(const QString &text, QString *error);
}

#endif
