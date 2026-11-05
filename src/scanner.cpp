/*
 * AuthMee for Nokia N9 - QR code scanner: camera viewfinder + ZBar decoder.
 * Copyright (c) 2026 smyhu. MIT License (see LICENSE).
 * Designed and written by Claude (Anthropic) together with smyhu.
 * The approach follows MeeScan (Nikolay Popov, 2011): QCamera frames go to an
 * own video surface, a worker thread runs ZBar on the brightness channel.
 */
#include "scanner.h"
#include <QtGui/QPainter>
#include <QtMultimediaKit/qmediaservice.h>
#include <QtMultimediaKit/qvideorenderercontrol.h>
#include <QtMultimediaKit/qcamerafocus.h>
#include <zbar.h>
using namespace zbar;

// ---------- Decoder ----------

Decoder::Decoder()
{
    zbar_image_scanner_t *s = zbar_image_scanner_create();
    zbar_image_scanner_set_config(s, ZBAR_NONE, ZBAR_CFG_ENABLE, 0);
    zbar_image_scanner_set_config(s, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);
    m_scanner = s;
}

Decoder::~Decoder()
{
    zbar_image_scanner_destroy((zbar_image_scanner_t *)m_scanner);
}

void Decoder::decode(const QByteArray &grey, int width, int height)
{
    QString found;
    zbar_image_t *img = zbar_image_create();
    zbar_image_set_format(img, zbar_fourcc('Y', '8', '0', '0'));
    zbar_image_set_size(img, width, height);
    zbar_image_set_data(img, grey.constData(), width * height, 0);
    if (zbar_scan_image((zbar_image_scanner_t *)m_scanner, img) > 0) {
        const zbar_symbol_t *sym = zbar_image_first_symbol(img);
        if (sym)
            found = QString::fromUtf8(zbar_symbol_get_data(sym), zbar_symbol_get_data_length(sym));
    }
    zbar_image_destroy(img);
    emit done(found);
}

// ---------- FrameSurface ----------

QList<QVideoFrame::PixelFormat> FrameSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const
{
    QList<QVideoFrame::PixelFormat> f;
    if (type == QAbstractVideoBuffer::NoHandle)
        f << QVideoFrame::Format_UYVY << QVideoFrame::Format_YUYV << QVideoFrame::Format_NV12
          << QVideoFrame::Format_NV21 << QVideoFrame::Format_YUV420P << QVideoFrame::Format_YV12
          << QVideoFrame::Format_RGB32 << QVideoFrame::Format_ARGB32 << QVideoFrame::Format_RGB565;
    return f;
}

bool FrameSurface::present(const QVideoFrame &input)
{
    QVideoFrame frame(input);
    if (!frame.map(QAbstractVideoBuffer::ReadOnly))
        return false;
    int w = frame.width(), h = frame.height(), bpl = frame.bytesPerLine();
    const uchar *src = frame.bits();
    QByteArray grey(w * h, 0);
    uchar *dst = (uchar *)grey.data();
    switch (frame.pixelFormat()) {
    case QVideoFrame::Format_UYVY:
    case QVideoFrame::Format_YUYV: {
        int first = frame.pixelFormat() == QVideoFrame::Format_UYVY ? 1 : 0;
        for (int y = 0; y < h; ++y) {
            const uchar *s = src + y * bpl + first;
            uchar *d = dst + y * w;
            for (int x = 0; x < w; ++x)
                d[x] = s[x * 2];
        }
        break;
    }
    case QVideoFrame::Format_NV12:
    case QVideoFrame::Format_NV21:
    case QVideoFrame::Format_YUV420P:
    case QVideoFrame::Format_YV12:
        for (int y = 0; y < h; ++y)
            memcpy(dst + y * w, src + y * bpl, w);
        break;
    case QVideoFrame::Format_RGB32:
    case QVideoFrame::Format_ARGB32:
        for (int y = 0; y < h; ++y) {
            const uchar *s = src + y * bpl;
            uchar *d = dst + y * w;
            for (int x = 0; x < w; ++x, s += 4)
                d[x] = (s[0] * 29 + s[1] * 150 + s[2] * 77) >> 8;
        }
        break;
    case QVideoFrame::Format_RGB565:
        for (int y = 0; y < h; ++y) {
            const quint16 *s = (const quint16 *)(src + y * bpl);
            uchar *d = dst + y * w;
            for (int x = 0; x < w; ++x) {
                int r = (s[x] >> 8) & 0xf8, g = (s[x] >> 3) & 0xfc, b = (s[x] << 3) & 0xf8;
                d[x] = (b * 29 + g * 150 + r * 77) >> 8;
            }
        }
        break;
    default:
        frame.unmap();
        return false;
    }
    frame.unmap();
    emit this->frame(grey, w, h);
    return true;
}

// ---------- QrScanner ----------

QrScanner::QrScanner(QDeclarativeItem *parent)
    : QDeclarativeItem(parent), m_active(false), m_busy(false), m_rotation(90),
      m_camera(0), m_renderer(0), m_surface(0), m_decoder(new Decoder)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    m_decoder->moveToThread(&m_thread);
    connect(this, SIGNAL(requestDecode(QByteArray,int,int)), m_decoder, SLOT(decode(QByteArray,int,int)), Qt::QueuedConnection);
    connect(m_decoder, SIGNAL(done(QString)), this, SLOT(onDecoded(QString)), Qt::QueuedConnection);
    connect(&m_thread, SIGNAL(finished()), m_decoder, SLOT(deleteLater()));
    m_thread.start(QThread::LowPriority);
    connect(&m_fake, SIGNAL(timeout()), this, SLOT(onFakeFrame()));
}

QrScanner::~QrScanner()
{
    stopCamera();
    m_thread.quit();
    m_thread.wait(3000);
}

void QrScanner::setActive(bool on)
{
    if (on == m_active)
        return;
    m_active = on;
    if (on)
        startCamera();
    else
        stopCamera();
    emit activeChanged();
}

void QrScanner::startCamera()
{
    m_last.clear();
    // Test mode for the emulator: AUTHMEE_FAKECAM=/path/picture.png plays the picture as camera frames.
    QByteArray fake = qgetenv("AUTHMEE_FAKECAM");
    if (!fake.isEmpty()) {
        m_fakeImage = QImage(QString::fromLocal8Bit(fake)).convertToFormat(QImage::Format_RGB32);
        m_fake.start(200);
        return;
    }
    m_camera = new QCamera(this);
    connect(m_camera, SIGNAL(error(QCamera::Error)), this, SLOT(onCameraError(QCamera::Error)));
    QMediaService *service = m_camera->service();
    if (service)
        m_renderer = qobject_cast<QVideoRendererControl *>(service->requestControl(QVideoRendererControl_iid));
    if (!m_renderer) {
        emit cameraError("no viewfinder");
        return;
    }
    m_surface = new FrameSurface(this);
    connect(m_surface, SIGNAL(frame(QByteArray,int,int)), this, SLOT(onFrame(QByteArray,int,int)));
    m_renderer->setSurface(m_surface);
    m_camera->setCaptureMode(QCamera::CaptureStillImage);
    QCameraFocus *f = m_camera->focus();
    if (f && f->isFocusModeSupported(QCameraFocus::ContinuousFocus))
        f->setFocusMode(QCameraFocus::ContinuousFocus);
    m_camera->start();
}

void QrScanner::stopCamera()
{
    m_fake.stop();
    if (m_camera) {
        m_camera->stop();
        if (m_renderer)
            m_renderer->setSurface(0);
        delete m_camera;
        m_camera = 0;
    }
    m_renderer = 0;
    delete m_surface;
    m_surface = 0;
    m_preview = QImage();
    update();
}

void QrScanner::focus()
{
    if (m_camera && m_camera->supportedLocks() & QCamera::LockFocus) {
        m_camera->searchAndLock(QCamera::LockFocus);
        QTimer::singleShot(3000, this, SLOT(unlockFocus()));
    }
}

void QrScanner::unlockFocus()
{
    if (m_camera)
        m_camera->unlock(QCamera::LockFocus);
}

void QrScanner::onFakeFrame()
{
    if (m_fakeImage.isNull())
        return;
    int w = m_fakeImage.width(), h = m_fakeImage.height();
    QByteArray grey(w * h, 0);
    for (int y = 0; y < h; ++y) {
        const QRgb *s = (const QRgb *)m_fakeImage.constScanLine(y);
        for (int x = 0; x < w; ++x)
            grey[y * w + x] = char(qGray(s[x]));
    }
    onFrame(grey, w, h);
}

void QrScanner::onFrame(const QByteArray &grey, int w, int h)
{
    if (!m_active)
        return;
    // preview at half size, painted rotated to fit the portrait page
    int pw = w / 2, ph = h / 2;
    if (m_preview.size() != QSize(pw, ph))
        m_preview = QImage(pw, ph, QImage::Format_RGB32);
    const uchar *g = (const uchar *)grey.constData();
    for (int y = 0; y < ph; ++y) {
        QRgb *d = (QRgb *)m_preview.scanLine(y);
        const uchar *s = g + (y * 2) * w;
        for (int x = 0; x < pw; ++x) {
            int v = s[x * 2];
            d[x] = qRgb(v * 9 / 10, v, v * 8 / 10);
        }
    }
    update();
    if (!m_busy) {
        m_busy = true;
        emit requestDecode(grey, w, h);
    }
}

void QrScanner::onDecoded(const QString &text)
{
    m_busy = false;
    if (!m_active || text.isEmpty() || text == m_last)
        return;
    m_last = text;
    emit decoded(text);
}

void QrScanner::onCameraError(QCamera::Error)
{
    emit cameraError(m_camera ? m_camera->errorString() : QString("camera"));
}

void QrScanner::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    QRectF r = boundingRect();
    p->fillRect(r, Qt::black);
    if (m_preview.isNull())
        return;
    p->save();
    p->setClipRect(r);
    p->translate(r.center());
    p->rotate(m_rotation);
    bool turned = (m_rotation / 90) % 2 != 0;
    qreal tw = turned ? r.height() : r.width(), th = turned ? r.width() : r.height();
    qreal scale = qMax(tw / m_preview.width(), th / m_preview.height());
    qreal iw = m_preview.width() * scale, ih = m_preview.height() * scale;
    p->drawImage(QRectF(-iw / 2, -ih / 2, iw, ih), m_preview);
    p->restore();
}
