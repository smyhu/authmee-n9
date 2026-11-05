/*
 * AuthMee for Nokia N9 - account list, PIN encryption, backups.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 *
 * File format (accounts.dat and backups): one otpauth:// link per line.
 *   without PIN:  "AUTHMEE0\n" + lines                     (file mode 0600)
 *   with PIN:     "AUTHMEE1" | iterations (4, BE) | salt (16) | iv (16) | AES-256-CBC data | HMAC-SHA256 (32)
 *   key material = PBKDF2-HMAC-SHA1(PIN, salt, iterations, 64): 32 bytes AES key + 32 bytes HMAC key.
 */
#include "store.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <sys/stat.h>
#include <unistd.h>

static const int ITERATIONS = 20000;
static const char MAGIC_PLAIN[] = "AUTHMEE0\n";
static const char MAGIC_SEALED[] = "AUTHMEE1";

namespace {

QByteArray randomBytes(int n)
{
    QByteArray b(n, 0);
    if (RAND_bytes((unsigned char *)b.data(), n) != 1) {
        QFile f("/dev/urandom");
        if (f.open(QIODevice::ReadOnly))
            b = f.read(n);
    }
    return b;
}

QByteArray derive(const QString &pin, const QByteArray &salt, int iterations)
{
    QByteArray pw = pin.toUtf8();
    QByteArray out(64, 0);
    PKCS5_PBKDF2_HMAC_SHA1(pw.constData(), pw.size(), (const unsigned char *)salt.constData(), salt.size(),
                           iterations, 64, (unsigned char *)out.data());
    return out;
}

QByteArray mac(const QByteArray &key, const QByteArray &data)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(EVP_sha256(), key.constData() + 32, 32, (const unsigned char *)data.constData(), data.size(), md, &len);
    return QByteArray((const char *)md, len);
}

QByteArray aes(bool encrypt, const QByteArray &key, const QByteArray &iv, const QByteArray &in, bool *ok)
{
    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX_init(&ctx);
    QByteArray out(in.size() + 32, 0);
    int n1 = 0, n2 = 0;
    *ok = EVP_CipherInit_ex(&ctx, EVP_aes_256_cbc(), 0, (const unsigned char *)key.constData(),
                            (const unsigned char *)iv.constData(), encrypt ? 1 : 0) == 1
          && EVP_CipherUpdate(&ctx, (unsigned char *)out.data(), &n1, (const unsigned char *)in.constData(), in.size()) == 1
          && EVP_CipherFinal_ex(&ctx, (unsigned char *)out.data() + n1, &n2) == 1;
    EVP_CIPHER_CTX_cleanup(&ctx);
    out.resize(*ok ? n1 + n2 : 0);
    return out;
}

QByteArray seal(const QByteArray &key, const QByteArray &salt, int iterations, const QByteArray &plain)
{
    QByteArray iv = randomBytes(16);
    bool ok;
    QByteArray ct = aes(true, key, iv, plain, &ok);
    QByteArray out(MAGIC_SEALED);
    out.append(char(iterations >> 24)).append(char(iterations >> 16)).append(char(iterations >> 8)).append(char(iterations));
    out.append(salt).append(iv).append(ct);
    out.append(mac(key, out));
    return out;
}

// Returns the plain text, or a null array when the PIN/password is wrong or the file is damaged.
QByteArray unseal(const QByteArray &blob, const QString &pin, QByteArray *keyOut, QByteArray *saltOut)
{
    if (blob.size() < 8 + 4 + 16 + 16 + 16 + 32 || !blob.startsWith(MAGIC_SEALED))
        return QByteArray();
    const unsigned char *p = (const unsigned char *)blob.constData() + 8;
    int iterations = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    if (iterations < 1000 || iterations > 1000000)
        return QByteArray();
    QByteArray salt = blob.mid(12, 16);
    QByteArray iv = blob.mid(28, 16);
    QByteArray body = blob.left(blob.size() - 32);
    QByteArray key = derive(pin, salt, iterations);
    if (mac(key, body) != blob.right(32))
        return QByteArray();
    bool ok;
    QByteArray plain = aes(false, key, iv, body.mid(44), &ok);
    if (!ok)
        return QByteArray();
    if (keyOut) *keyOut = key;
    if (saltOut) *saltOut = salt;
    return plain.isNull() ? QByteArray("") : plain;
}

bool writeSecure(const QString &path, const QByteArray &data)
{
    QString tmp = path + ".new";
    QFile f(tmp);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    ::chmod(QFile::encodeName(tmp).constData(), 0600);
    bool ok = f.write(data) == data.size();
    f.flush();
    ::fsync(f.handle());
    f.close();
    if (!ok)
        return false;
    QFile::remove(path + ".old");
    QFile::rename(path, path + ".old");
    if (!QFile::rename(tmp, path))
        return false;
    QFile::remove(path + ".old");
    return true;
}

} // namespace

Store::Store(QObject *parent)
    : QObject(parent), m_locked(false), m_settings("authmee", "authmee")
{
    QString dir = QDir::homePath() + "/.local/share/authmee";
    QByteArray env = qgetenv("AUTHMEE_DATA");
    if (!env.isEmpty())
        dir = QString::fromLocal8Bit(env);
    QDir().mkpath(dir);
    ::chmod(QFile::encodeName(dir).constData(), 0700);
    m_path = dir + "/accounts.dat";
    if (!QFile::exists(m_path) && QFile::exists(m_path + ".old"))
        QFile::rename(m_path + ".old", m_path);
    load();
}

bool Store::load()
{
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray data = f.readAll();
    if (data.startsWith(MAGIC_SEALED)) {
        m_locked = true;
        return true;
    }
    if (data.startsWith(MAGIC_PLAIN))
        deserialize(data.mid(sizeof(MAGIC_PLAIN) - 1));
    return true;
}

bool Store::unlock(const QString &pin)
{
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray plain = unseal(f.readAll(), pin, &m_key, &m_salt);
    if (plain.isNull())
        return false;
    deserialize(plain);
    m_locked = false;
    emit changed();
    return true;
}

bool Store::checkPin(const QString &pin)
{
    if (m_key.isEmpty())
        return true;
    return derive(pin, m_salt, ITERATIONS) == m_key;
}

void Store::lock()
{
    if (m_key.isEmpty() || m_locked)
        return;
    m_accounts.clear();
    m_key.fill(0);
    m_key.clear();
    m_locked = true;
    emit changed();
}

QString Store::preview(const QVariantMap &m, double unixSeconds) const
{
    bool ok;
    Account a = fromMap(m, &ok);
    return ok ? Otp::code(a, (qint64)unixSeconds) : QString();
}

bool Store::setPin(const QString &pin)
{
    if (m_locked)
        return false;
    if (pin.isEmpty()) {
        m_key.clear();
        m_salt.clear();
    } else {
        m_salt = randomBytes(16);
        m_key = derive(pin, m_salt, ITERATIONS);
    }
    bool ok = save();
    emit changed();
    return ok;
}

QByteArray Store::serialize() const
{
    QByteArray out;
    foreach (const Account &a, m_accounts)
        out += Otp::toUri(a).toUtf8() + '\n';
    return out;
}

void Store::deserialize(const QByteArray &plain)
{
    m_accounts.clear();
    foreach (const QByteArray &line, plain.split('\n')) {
        if (line.trimmed().isEmpty())
            continue;
        QString err;
        QList<Account> got = Otp::parse(QString::fromUtf8(line), &err);
        if (!got.isEmpty())
            m_accounts.append(got.first());
    }
}

bool Store::save()
{
    QByteArray plain = serialize();
    QByteArray data = m_key.isEmpty() ? QByteArray(MAGIC_PLAIN) + plain : seal(m_key, m_salt, ITERATIONS, plain);
    return writeSecure(m_path, data);
}

QVariantMap Store::get(int i) const
{
    QVariantMap m;
    if (i < 0 || i >= m_accounts.size())
        return m;
    const Account &a = m_accounts.at(i);
    m["issuer"] = a.issuer;
    m["name"] = a.name;
    m["secret"] = Otp::base32Encode(a.secret);
    m["algorithm"] = a.algorithm;
    m["digits"] = a.digits;
    m["period"] = a.period;
    return m;
}

QString Store::code(int i, double unixSeconds) const
{
    if (i < 0 || i >= m_accounts.size())
        return QString();
    return Otp::code(m_accounts.at(i), (qint64)unixSeconds);
}

QVariantList Store::parse(const QString &text)
{
    QVariantList out;
    m_error.clear();
    QList<Account> got = Otp::parse(text, &m_error);
    foreach (const Account &a, got) {
        QVariantMap m;
        m["issuer"] = a.issuer;
        m["name"] = a.name;
        m["secret"] = Otp::base32Encode(a.secret);
        m["algorithm"] = a.algorithm;
        m["digits"] = a.digits;
        m["period"] = a.period;
        out.append(m);
    }
    return out;
}

bool Store::validSecret(const QString &secret) const
{
    bool ok;
    QByteArray b = Otp::base32Decode(secret, &ok);
    return ok && b.size() >= 5;
}

Account Store::fromMap(const QVariantMap &m, bool *ok) const
{
    Account a;
    a.issuer = m.value("issuer").toString().trimmed();
    a.name = m.value("name").toString().trimmed();
    a.secret = Otp::base32Decode(m.value("secret").toString(), ok);
    QString al = m.value("algorithm").toString();
    if (al == "SHA256" || al == "SHA512")
        a.algorithm = al;
    int d = m.value("digits").toInt();
    if (d >= 6 && d <= 8)
        a.digits = d;
    int p = m.value("period").toInt();
    if (p >= 10 && p <= 300)
        a.period = p;
    *ok = *ok && a.secret.size() >= 5;
    return a;
}

bool Store::contains(const QVariantMap &m) const
{
    bool ok;
    Account a = fromMap(m, &ok);
    foreach (const Account &b, m_accounts)
        if (b.secret == a.secret)
            return true;
    return false;
}

bool Store::add(const QVariantMap &m)
{
    bool ok;
    Account a = fromMap(m, &ok);
    if (!ok || m_locked)
        return false;
    m_accounts.append(a);
    bool saved = save();
    emit changed();
    return saved;
}

int Store::addAll(const QVariantList &list)
{
    int n = 0;
    foreach (const QVariant &v, list) {
        bool ok;
        Account a = fromMap(v.toMap(), &ok);
        if (ok && !contains(v.toMap())) {
            m_accounts.append(a);
            ++n;
        }
    }
    if (n) {
        save();
        emit changed();
    }
    return n;
}

bool Store::update(int i, const QVariantMap &m)
{
    if (i < 0 || i >= m_accounts.size() || m_locked)
        return false;
    bool ok;
    Account a = fromMap(m, &ok);
    if (!ok)
        return false;
    m_accounts[i] = a;
    bool saved = save();
    emit changed();
    return saved;
}

bool Store::remove(int i)
{
    if (i < 0 || i >= m_accounts.size() || m_locked)
        return false;
    m_accounts.removeAt(i);
    bool saved = save();
    emit changed();
    return saved;
}

bool Store::move(int from, int to)
{
    if (from < 0 || from >= m_accounts.size() || to < 0 || to >= m_accounts.size() || from == to || m_locked)
        return false;
    m_accounts.move(from, to);
    bool saved = save();
    emit changed();
    return saved;
}

QString Store::backupFolder() const
{
    QByteArray env = qgetenv("AUTHMEE_BACKUP");
    if (!env.isEmpty())
        return QString::fromLocal8Bit(env);
    return QDir::homePath() + "/MyDocs/AuthMee";
}

QString Store::exportBackup(const QString &password)
{
    if (m_locked || password.isEmpty())
        return QString();
    QDir().mkpath(backupFolder());
    QString path = backupFolder() + "/authmee-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmm") + ".authmee";
    QByteArray salt = randomBytes(16);
    QByteArray data = seal(derive(password, salt, ITERATIONS), salt, ITERATIONS, serialize());
    return writeSecure(path, data) ? path : QString();
}

QStringList Store::backups() const
{
    QDir d(backupFolder());
    QStringList out;
    foreach (const QString &f, d.entryList(QStringList() << "*.authmee", QDir::Files, QDir::Name | QDir::Reversed))
        out << d.filePath(f);
    return out;
}

int Store::importBackup(const QString &path, const QString &password)
{
    QFile f(path);
    if (m_locked || !f.open(QIODevice::ReadOnly))
        return -2;
    QByteArray plain = unseal(f.readAll(), password, 0, 0);
    if (plain.isNull())
        return -1;
    QVariantList list;
    foreach (const QByteArray &line, plain.split('\n'))
        if (!line.trimmed().isEmpty())
            list += parse(QString::fromUtf8(line));
    return addAll(list);
}

void Store::copy(const QString &text)
{
    QApplication::clipboard()->setText(text);
}

QString Store::clipboard() const
{
    return QApplication::clipboard()->text();
}

QVariant Store::setting(const QString &key, const QVariant &def) const
{
    return m_settings.value(key, def);
}

void Store::setSetting(const QString &key, const QVariant &value)
{
    m_settings.setValue(key, value);
}
