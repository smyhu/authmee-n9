/*
 * AuthMee for Nokia N9 (Harmattan) - two-step verification codes (TOTP).
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 */
#include <QtGui/QApplication>
#include <QtCore/QLocale>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/qdeclarative.h>
#include "store.h"
#include "clock.h"
#include "scanner.h"
#include "ring.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("authmee");
    app.setOrganizationName("authmee");
    qmlRegisterType<QrScanner>("AuthMee", 1, 0, "QrScanner");
    qmlRegisterType<Ring>("AuthMee", 1, 0, "Ring");

    QString base = "/opt/authmee-n9/qml/";
    QByteArray dir = qgetenv("AUTHMEE_QML");
    if (!dir.isEmpty()) base = QString::fromLocal8Bit(dir);
    QString lang = QLocale::system().name().left(2);
    QByteArray l = qgetenv("AUTHMEE_LANG");
    if (!l.isEmpty()) lang = QString::fromLatin1(l);

    Store store;
    Clock clock;
    QDeclarativeView view;
    view.rootContext()->setContextProperty("appVersion", QString(AUTHMEE_VERSION));
    view.rootContext()->setContextProperty("uiLang", lang);
    view.rootContext()->setContextProperty("store", &store);
    view.rootContext()->setContextProperty("clock", &clock);
    QObject::connect(view.engine(), SIGNAL(quit()), &app, SLOT(quit()));
    view.setSource(QUrl::fromLocalFile(base + "main.qml"));
    view.showFullScreen();
    return app.exec();
}
