/*
 * AuthMee for Nokia N9 - one-time codes (RFC 4226 / RFC 6238) and otpauth links.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#include "otp.h"
#include <QtCore/QUrl>
#include <QtCore/QStringList>
#include <openssl/hmac.h>
#include <openssl/evp.h>

static const char B32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

QByteArray Otp::base32Decode(const QString &text, bool *ok)
{
    QByteArray out;
    unsigned int buffer = 0;
    int bits = 0;
    *ok = false;
    for (int i = 0; i < text.length(); ++i) {
        QChar c = text.at(i).toUpper();
        if (c == ' ' || c == '-' || c == '=')
            continue;
        const char *p = 0;
        if (c.unicode() < 128)
            p = strchr(B32, c.toLatin1());
        if (!p || !*p) {
            // common typing mistakes: 0 -> O, 1 -> L, 8 -> B
            if (c == '0') p = strchr(B32, 'O');
            else if (c == '1') p = strchr(B32, 'L');
            else if (c == '8') p = strchr(B32, 'B');
            else return QByteArray();
        }
        buffer = (buffer << 5) | (unsigned int)(p - B32);
        bits += 5;
        if (bits >= 8) {
            out.append(char((buffer >> (bits - 8)) & 0xff));
            bits -= 8;
        }
    }
    *ok = !out.isEmpty();
    return out;
}

QString Otp::base32Encode(const QByteArray &data)
{
    QString out;
    unsigned int buffer = 0;
    int bits = 0;
    for (int i = 0; i < data.size(); ++i) {
        buffer = (buffer << 8) | (unsigned char)data.at(i);
        bits += 8;
        while (bits >= 5) {
            out.append(QLatin1Char(B32[(buffer >> (bits - 5)) & 31]));
            bits -= 5;
        }
    }
    if (bits > 0)
        out.append(QLatin1Char(B32[(buffer << (5 - bits)) & 31]));
    return out;
}

static const EVP_MD *digestFor(const QString &algorithm)
{
    if (algorithm == "SHA256") return EVP_sha256();
    if (algorithm == "SHA512") return EVP_sha512();
    return EVP_sha1();
}

QString Otp::code(const Account &a, qint64 unixSeconds)
{
    if (a.secret.isEmpty() || a.period <= 0)
        return QString();
    quint64 counter = (quint64)(unixSeconds / a.period);
    unsigned char msg[8];
    for (int i = 7; i >= 0; --i) {
        msg[i] = counter & 0xff;
        counter >>= 8;
    }
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(digestFor(a.algorithm), a.secret.constData(), a.secret.size(), msg, 8, md, &len);
    if (len < 20)
        return QString();
    int offset = md[len - 1] & 0x0f;
    quint32 bin = ((md[offset] & 0x7f) << 24) | (md[offset + 1] << 16) | (md[offset + 2] << 8) | md[offset + 3];
    quint32 mod = 1;
    for (int i = 0; i < a.digits; ++i)
        mod *= 10;
    return QString::number(bin % mod).rightJustified(a.digits, '0');
}

QString Otp::toUri(const Account &a)
{
    QString label = a.issuer.isEmpty() ? a.name : a.issuer + ":" + a.name;
    QString uri = "otpauth://totp/" + QString::fromLatin1(QUrl::toPercentEncoding(label, ":@"));
    uri += "?secret=" + base32Encode(a.secret);
    if (!a.issuer.isEmpty())
        uri += "&issuer=" + QString::fromLatin1(QUrl::toPercentEncoding(a.issuer));
    if (a.algorithm != "SHA1")
        uri += "&algorithm=" + a.algorithm;
    if (a.digits != 6)
        uri += "&digits=" + QString::number(a.digits);
    if (a.period != 30)
        uri += "&period=" + QString::number(a.period);
    return uri;
}

static QString pct(const QString &s)
{
    QByteArray b = s.toLatin1();
    b.replace('+', "%20");
    return QUrl::fromPercentEncoding(b);
}

static bool parseTotp(const QString &text, Account *a, QString *error)
{
    // otpauth://totp/Issuer:name?secret=...&issuer=...
    int q = text.indexOf('?');
    QString head = text.left(q < 0 ? text.length() : q);
    QString query = q < 0 ? QString() : text.mid(q + 1);
    QString type = head.mid(10).section('/', 0, 0).toLower();
    if (type == "hotp") {
        *error = "hotp";
        return false;
    }
    if (type != "totp") {
        *error = "format";
        return false;
    }
    QString label = pct(head.mid(10).section('/', 1));
    int colon = label.indexOf(':');
    if (colon >= 0) {
        a->issuer = label.left(colon).trimmed();
        a->name = label.mid(colon + 1).trimmed();
    } else {
        a->name = label.trimmed();
    }
    bool haveSecret = false;
    foreach (const QString &pair, query.split('&', QString::SkipEmptyParts)) {
        QString key = pair.section('=', 0, 0).toLower();
        QString val = pct(pair.section('=', 1));
        if (key == "secret") {
            bool ok;
            a->secret = Otp::base32Decode(val, &ok);
            haveSecret = ok;
        } else if (key == "issuer") {
            if (!val.trimmed().isEmpty())
                a->issuer = val.trimmed();
        } else if (key == "algorithm") {
            QString al = val.toUpper().remove('-');
            if (al == "SHA1" || al == "SHA256" || al == "SHA512")
                a->algorithm = al;
        } else if (key == "digits") {
            int d = val.toInt();
            if (d >= 6 && d <= 8)
                a->digits = d;
        } else if (key == "period") {
            int p = val.toInt();
            if (p >= 10 && p <= 300)
                a->period = p;
        }
    }
    if (!haveSecret) {
        *error = "secret";
        return false;
    }
    return true;
}

// --- Google Authenticator export (protobuf, written by hand: no library needed) ---

static bool readVarint(const QByteArray &b, int *pos, quint64 *v)
{
    *v = 0;
    for (int shift = 0; shift < 64 && *pos < b.size(); shift += 7) {
        unsigned char c = b.at((*pos)++);
        *v |= (quint64)(c & 0x7f) << shift;
        if (!(c & 0x80))
            return true;
    }
    return false;
}

// Calls back for every field of one message: number, wire type, varint value or bytes.
struct Field { int number; int wire; quint64 value; QByteArray bytes; };

static QList<Field> fields(const QByteArray &b)
{
    QList<Field> out;
    int pos = 0;
    while (pos < b.size()) {
        quint64 key;
        if (!readVarint(b, &pos, &key))
            break;
        Field f;
        f.number = int(key >> 3);
        f.wire = int(key & 7);
        f.value = 0;
        if (f.wire == 0) {
            if (!readVarint(b, &pos, &f.value)) break;
        } else if (f.wire == 2) {
            quint64 len;
            if (!readVarint(b, &pos, &len) || pos + (int)len > b.size()) break;
            f.bytes = b.mid(pos, (int)len);
            pos += (int)len;
        } else if (f.wire == 5) {
            pos += 4;
        } else if (f.wire == 1) {
            pos += 8;
        } else {
            break;
        }
        out.append(f);
    }
    return out;
}

static QList<Account> parseMigration(const QString &text, QString *error)
{
    QList<Account> out;
    int i = text.indexOf("data=");
    if (i < 0) {
        *error = "format";
        return out;
    }
    QString data = pct(text.mid(i + 5).section('&', 0, 0));
    QByteArray payload = QByteArray::fromBase64(data.toLatin1());
    int skipped = 0;
    foreach (const Field &f, fields(payload)) {
        if (f.number != 1 || f.wire != 2)
            continue;
        Account a;
        int type = 2;
        foreach (const Field &g, fields(f.bytes)) {
            switch (g.number) {
            case 1: a.secret = g.bytes; break;
            case 2: a.name = QString::fromUtf8(g.bytes); break;
            case 3: a.issuer = QString::fromUtf8(g.bytes); break;
            case 4: a.algorithm = g.value == 2 ? "SHA256" : g.value == 3 ? "SHA512" : "SHA1"; break;
            case 5: a.digits = g.value == 2 ? 8 : 6; break;
            case 6: type = int(g.value); break;
            }
        }
        if (type != 2 || a.secret.isEmpty()) {
            ++skipped;
            continue;
        }
        int colon = a.name.indexOf(':');
        if (colon >= 0) {
            if (a.issuer.isEmpty())
                a.issuer = a.name.left(colon).trimmed();
            a.name = a.name.mid(colon + 1).trimmed();
        }
        out.append(a);
    }
    if (out.isEmpty())
        *error = skipped ? "hotp" : "format";
    return out;
}

QList<Account> Otp::parse(const QString &input, QString *error)
{
    QList<Account> out;
    QString text = input.trimmed();
    if (text.startsWith("otpauth-migration://", Qt::CaseInsensitive))
        return parseMigration(text, error);
    if (text.startsWith("otpauth://", Qt::CaseInsensitive)) {
        Account a;
        if (parseTotp(text, &a, error))
            out.append(a);
        return out;
    }
    // a bare key typed or pasted by hand
    bool ok;
    Account a;
    a.secret = base32Decode(text, &ok);
    if (ok && text.length() >= 16) {
        out.append(a);
        return out;
    }
    *error = "format";
    return out;
}
