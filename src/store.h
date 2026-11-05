/*
 * AuthMee for Nokia N9 - account list, PIN encryption, backups.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#ifndef AUTHMEE_STORE_H
#define AUTHMEE_STORE_H

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QSettings>
#include "otp.h"

class Store : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool locked READ locked NOTIFY changed)
    Q_PROPERTY(bool pinEnabled READ pinEnabled NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
public:
    explicit Store(QObject *parent = 0);

    bool locked() const { return m_locked; }
    bool pinEnabled() const { return !m_key.isEmpty() || m_locked; }
    int count() const { return m_accounts.size(); }

    Q_INVOKABLE bool unlock(const QString &pin);
    Q_INVOKABLE bool setPin(const QString &pin);     // empty pin = no PIN
    Q_INVOKABLE bool checkPin(const QString &pin);
    Q_INVOKABLE void lock();
    Q_INVOKABLE QString preview(const QVariantMap &m, double unixSeconds) const;

    Q_INVOKABLE QVariantMap get(int i) const;
    Q_INVOKABLE QString code(int i, double unixSeconds) const;
    // Parses a scanned or pasted text; returns a list of account maps (empty on error).
    Q_INVOKABLE QVariantList parse(const QString &text);
    Q_INVOKABLE QString lastError() const { return m_error; }
    Q_INVOKABLE bool validSecret(const QString &secret) const;
    Q_INVOKABLE bool add(const QVariantMap &m);
    Q_INVOKABLE int addAll(const QVariantList &list);
    Q_INVOKABLE bool update(int i, const QVariantMap &m);
    Q_INVOKABLE bool remove(int i);
    Q_INVOKABLE bool move(int from, int to);
    Q_INVOKABLE bool contains(const QVariantMap &m) const;

    Q_INVOKABLE QString exportBackup(const QString &password);
    Q_INVOKABLE QStringList backups() const;
    Q_INVOKABLE int importBackup(const QString &path, const QString &password);
    Q_INVOKABLE QString backupFolder() const;

    Q_INVOKABLE void copy(const QString &text);
    Q_INVOKABLE QString clipboard() const;
    Q_INVOKABLE QVariant setting(const QString &key, const QVariant &def) const;
    Q_INVOKABLE void setSetting(const QString &key, const QVariant &value);

signals:
    void changed();

private:
    bool load();
    bool save();
    QByteArray serialize() const;
    void deserialize(const QByteArray &plain);
    Account fromMap(const QVariantMap &m, bool *ok) const;

    QString m_path;
    QList<Account> m_accounts;
    QByteArray m_key;      // derived from the PIN while unlocked (empty = no PIN)
    QByteArray m_salt;
    bool m_locked;
    QString m_error;
    mutable QSettings m_settings;
};

#endif
