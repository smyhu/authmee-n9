/*
 * AuthMee for Nokia N9 - QR code scanner: camera viewfinder + ZBar decoder.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 * ZBar (LGPL 2.1) by Jeff Brown, QR decoder by Timothy B. Terriberry.
 */
#ifndef AUTHMEE_SCANNER_H
#define AUTHMEE_SCANNER_H

#include <QtDeclarative/QDeclarativeItem>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimediaKit/qabstractvideosurface.h>
#include <QtMultimediaKit/qcamera.h>

class QVideoRendererControl;

// Decodes grey frames in a worker thread.
class Decoder : public QObject
{
    Q_OBJECT
public:
    Decoder();
    ~Decoder();
public slots:
    void decode(const QByteArray &grey, int width, int height);
signals:
    void done(const QString &text);
private:
    void *m_scanner;
};

// Receives camera frames and keeps only the brightness (Y) channel.
class FrameSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit FrameSurface(QObject *parent = 0) : QAbstractVideoSurface(parent) {}
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const;
    bool present(const QVideoFrame &frame);
signals:
    void frame(const QByteArray &grey, int width, int height);
};

class QrScanner : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int frameRotation READ frameRotation WRITE setFrameRotation)
public:
    explicit QrScanner(QDeclarativeItem *parent = 0);
    ~QrScanner();
    bool active() const { return m_active; }
    void setActive(bool on);
    int frameRotation() const { return m_rotation; }
    void setFrameRotation(int r) { m_rotation = r; update(); }
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *);
    Q_INVOKABLE void focus();

signals:
    void activeChanged();
    void decoded(const QString &text);
    void cameraError(const QString &message);
    void requestDecode(const QByteArray &grey, int width, int height);

private slots:
    void onFrame(const QByteArray &grey, int width, int height);
    void onDecoded(const QString &text);
    void onCameraError(QCamera::Error);
    void onFakeFrame();
    void unlockFocus();

private:
    void startCamera();
    void stopCamera();

    bool m_active;
    bool m_busy;
    int m_rotation;
    QImage m_preview;
    QCamera *m_camera;
    QVideoRendererControl *m_renderer;
    FrameSurface *m_surface;
    QThread m_thread;
    Decoder *m_decoder;
    QTimer m_fake;
    QImage m_fakeImage;
    QString m_last;
};

#endif
