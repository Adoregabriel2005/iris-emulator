#include "DisplayWidget.h"
#include "../SDLInput.h"

#include <QtCore/QSettings>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QApplication>
#include <algorithm>
#include <cmath>

DisplayWidget::DisplayWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(160, 120);

    QSettings settings;
    QString filterStr = settings.value("Video/Filter", "none").toString();
    if      (filterStr == "scanlines") m_filter = ScreenFilter::Scanlines;
    else if (filterStr == "crt")       m_filter = ScreenFilter::CRT;
    else if (filterStr == "smooth")    m_filter = ScreenFilter::Smooth;
    else if (filterStr == "lcd")       m_filter = ScreenFilter::LCD;
    else if (filterStr == "lcdblur")   m_filter = ScreenFilter::LCDBlur;
    else if (filterStr == "av")        m_filter = ScreenFilter::AV;
    else if (filterStr == "svideo")    m_filter = ScreenFilter::SVideo;
    else if (filterStr == "vga")       m_filter = ScreenFilter::VGA;
    else if (filterStr == "rgb")       m_filter = ScreenFilter::RGB_RGB;
    else if (filterStr == "crispy")    m_filter = ScreenFilter::Crispy;
    else                               m_filter = ScreenFilter::None;

    m_scanline_intensity = settings.value("Video/ScanlineIntensity", 50).toInt();
    m_lcd_ghosting       = settings.value("Video/LCDGhosting", 40).toInt();
}

DisplayWidget::~DisplayWidget() {}

void DisplayWidget::updateFrame(const QImage& frame)
{
    if (m_filter == ScreenFilter::LCDBlur && !m_current_frame.isNull())
        m_prev_frame = m_current_frame;

    m_current_frame = frame;
    m_scaled_frame = QImage();
    m_scaled_frame_rect = QRect();
    m_frame_cache_key = 0;
    update();
}

void DisplayWidget::clearFrame()
{
    m_current_frame = QImage();
    m_prev_frame    = QImage();
    m_scaled_frame  = QImage();
    m_scaled_frame_rect = QRect();
    m_frame_cache_key = 0;
    update();
}

QRect DisplayWidget::calculateDisplayRect() const
{
    if (m_current_frame.isNull())
        return rect();

    const float display_aspect = (m_current_frame.height() <= 102)
        ? static_cast<float>(m_current_frame.width()) / static_cast<float>(m_current_frame.height())
        : 4.0f / 3.0f;

    const float widget_aspect = static_cast<float>(width()) / static_cast<float>(height());
    int display_w, display_h;

    if (widget_aspect > display_aspect) {
        display_h = height();
        display_w = static_cast<int>(display_h * display_aspect);
    } else {
        display_w = width();
        display_h = static_cast<int>(display_w / display_aspect);
    }

    return QRect((width() - display_w) / 2, (height() - display_h) / 2, display_w, display_h);
}

void DisplayWidget::setScreenFilter(ScreenFilter filter)
{
    m_filter = filter;
    m_scaled_frame = QImage();
    m_scaled_frame_rect = QRect();
    m_frame_cache_key = 0;
    update();
}

void DisplayWidget::setScanlineIntensity(int percent)
{
    m_scanline_intensity = std::clamp(percent, 0, 100);
    update();
}

void DisplayWidget::setLCDGhostingStrength(int percent)
{
    m_lcd_ghosting = std::clamp(percent, 0, 100);
    update();
}

// ─────────────────────────────────────────────────────────────────────────────
// paintEvent
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (m_current_frame.isNull())
        return;

    QRect dr = calculateDisplayRect();

    QImage frameToDraw;
    if (m_filter == ScreenFilter::LCDBlur) {
        if (!m_prev_frame.isNull() && m_prev_frame.size() == m_current_frame.size()) {
            frameToDraw = applyLCDBlur(m_current_frame);
        } else {
            frameToDraw = m_current_frame;
        }
    } else {
        const quint64 cacheKey = m_current_frame.cacheKey();
        if (m_scaled_frame_rect.size() != dr.size() || m_frame_cache_key != cacheKey) {
            Qt::TransformationMode mode = Qt::FastTransformation;
            if (m_filter == ScreenFilter::Smooth || m_filter == ScreenFilter::CRT)
                mode = Qt::SmoothTransformation;
            m_scaled_frame = m_current_frame.scaled(dr.size(), Qt::IgnoreAspectRatio, mode);
            m_scaled_frame_rect = dr;
            m_frame_cache_key = cacheKey;
        }
        frameToDraw = m_scaled_frame;
    }

    switch (m_filter) {
    case ScreenFilter::None:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dr, frameToDraw);
        break;

    case ScreenFilter::Smooth:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dr, frameToDraw);
        break;

    case ScreenFilter::Scanlines:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dr, frameToDraw);
        drawScanlines(painter, dr);
        break;

    case ScreenFilter::CRT:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dr, frameToDraw);
        drawBloom(painter, dr);
        drawScanlines(painter, dr);
        drawCRT(painter, dr);
        break;

    case ScreenFilter::LCD:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dr, frameToDraw);
        drawLCD(painter, dr);
        break;

    case ScreenFilter::LCDBlur:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dr, frameToDraw);
        drawLCD(painter, dr);
        break;

    case ScreenFilter::AV:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(dr, frameToDraw);
        drawBloom(painter, dr);
        drawAVArtifacts(painter, dr);
        drawScanlines(painter, dr);
        break;

    case ScreenFilter::SVideo:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(dr, frameToDraw);
        drawSVideoArtifacts(painter, dr);
        drawScanlines(painter, dr);
        break;

    case ScreenFilter::VGA:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(dr, frameToDraw);
        drawVGAScanlines(painter, dr);
        break;

    case ScreenFilter::RGB_RGB:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(dr, frameToDraw);
        drawRGBMask(painter, dr);
        drawScanlines(painter, dr);
        break;

    case ScreenFilter::Crispy:
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(dr, frameToDraw);
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Scanlines — alternância real de brilho entre linhas pares/ímpares
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawScanlines(QPainter& painter, const QRect& rect)
{
    if (m_scanline_intensity <= 0)
        return;

    // Quantos pixels de tela por linha de source
    const int srcH   = m_current_frame.height();
    const float scale = static_cast<float>(rect.height()) / srcH;
    const int lineH  = std::max(1, static_cast<int>(std::round(scale)));

    // Escurece linhas ímpares (gap entre scanlines)
    const int darkAlpha  = m_scanline_intensity * 220 / 100;
    // Leve brilho nas linhas pares (phosphor glow)
    const int glowAlpha  = m_scanline_intensity * 18 / 100;

    painter.setPen(Qt::NoPen);

    for (int srcY = 0; srcY < srcH; srcY++) {
        int screenY = rect.top() + static_cast<int>(srcY * scale);

        // Linha escura (gap do CRT)
        int gapY = screenY + std::max(1, lineH - 1);
        if (gapY < rect.bottom()) {
            painter.setBrush(QColor(0, 0, 0, darkAlpha));
            painter.drawRect(rect.left(), gapY, rect.width(), lineH > 2 ? lineH / 2 : 1);
        }

        // Phosphor glow sutil na linha ativa
        if (glowAlpha > 0) {
            painter.setBrush(QColor(255, 255, 220, glowAlpha));
            painter.drawRect(rect.left(), screenY, rect.width(), std::max(1, lineH - 1));
        }
    }

    // Linha horizontal de separação entre scanlines (mais escura)
    const int sepAlpha = m_scanline_intensity * 160 / 100;
    painter.setBrush(QColor(0, 0, 0, sepAlpha));
    for (int srcY = 0; srcY < srcH; srcY += 2) {
        int screenY = rect.top() + static_cast<int>(srcY * scale) + std::max(1, lineH - 1);
        painter.drawRect(rect.left(), screenY, rect.width(), 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CRT — vignette forte + curvatura + phosphor RGB mask + bloom
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawCRT(QPainter& painter, const QRect& rect)
{
    // ── Phosphor RGB mask (colunas R/G/B alternadas) ──
    const int maskAlpha = m_scanline_intensity * 25 / 100;
    if (maskAlpha > 0) {
        painter.setPen(Qt::NoPen);
        for (int x = rect.left(); x < rect.right(); x += 3) {
            painter.setBrush(QColor(255, 0, 0, maskAlpha));
            painter.drawRect(x,     rect.top(), 1, rect.height());
            painter.setBrush(QColor(0, 255, 0, maskAlpha / 2));
            painter.drawRect(x + 1, rect.top(), 1, rect.height());
            painter.setBrush(QColor(0, 0, 255, maskAlpha));
            painter.drawRect(x + 2, rect.top(), 1, rect.height());
        }
    }

    // ── Vignette forte nas bordas ──
    QRadialGradient vignette(rect.center(), rect.width() * 0.62f);
    vignette.setColorAt(0.0, QColor(0, 0, 0, 0));
    vignette.setColorAt(0.55, QColor(0, 0, 0, 0));
    vignette.setColorAt(0.80, QColor(0, 0, 0, 60));
    vignette.setColorAt(1.0,  QColor(0, 0, 0, 180));
    painter.fillRect(rect, vignette);

    // ── Curvatura: cantos muito escuros ──
    int cs = rect.width() / 6;
    auto drawCorner = [&](QPoint center) {
        QRadialGradient g(center, cs);
        g.setColorAt(0.0, QColor(0, 0, 0, 140));
        g.setColorAt(1.0, QColor(0, 0, 0, 0));
        painter.fillRect(QRect(center - QPoint(cs, cs), QSize(cs * 2, cs * 2)), g);
    };
    drawCorner(rect.topLeft());
    drawCorner(rect.topRight());
    drawCorner(rect.bottomLeft());
    drawCorner(rect.bottomRight());

    // ── Reflexo de tela (linha horizontal de brilho no topo) ──
    QLinearGradient glare(rect.topLeft(), QPoint(rect.left(), rect.top() + rect.height() / 4));
    glare.setColorAt(0.0, QColor(255, 255, 255, 18));
    glare.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillRect(rect, glare);

    // ── Borda do tubo (frame escuro ao redor) ──
    painter.setPen(QPen(QColor(0, 0, 0, 200), 4));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect.adjusted(1, 1, -1, -1));
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD — grid de pixels com gap entre eles (tela do Lynx)
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawLCD(QPainter& painter, const QRect& rect)
{
    const int srcW = m_current_frame.width();
    const int srcH = m_current_frame.height();
    const float scaleX = static_cast<float>(rect.width())  / srcW;
    const float scaleY = static_cast<float>(rect.height()) / srcH;

    // Só desenha o grid se tiver escala suficiente (pelo menos 2px por pixel)
    if (scaleX < 2.0f || scaleY < 2.0f)
        return;

    const int gapAlpha = m_scanline_intensity * 180 / 100;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, gapAlpha));

    // Linhas horizontais (gap entre linhas de pixels)
    for (int y = 0; y < srcH; y++) {
        int screenY = rect.top() + static_cast<int>(y * scaleY) + static_cast<int>(scaleY) - 1;
        painter.drawRect(rect.left(), screenY, rect.width(), 1);
    }

    // Linhas verticais (gap entre colunas de pixels)
    for (int x = 0; x < srcW; x++) {
        int screenX = rect.left() + static_cast<int>(x * scaleX) + static_cast<int>(scaleX) - 1;
        painter.drawRect(screenX, rect.top(), 1, rect.height());
    }

    // Leve tint esverdeado — o LCD do Lynx original tinha um tint verde
    painter.setBrush(QColor(20, 40, 10, 25));
    painter.drawRect(rect);
}

// ─────────────────────────────────────────────────────────────────────────────
// LCD Blur — blend com frame anterior para simular ghosting do Lynx original
// ─────────────────────────────────────────────────────────────────────────────
QImage DisplayWidget::applyLCDBlur(const QImage& src) const
{
    if (m_prev_frame.isNull() || m_prev_frame.size() != src.size())
        return src;

    QImage result = src.convertToFormat(QImage::Format_ARGB32);
    const QImage prev = m_prev_frame.convertToFormat(QImage::Format_ARGB32);

    const float ghostStrength = m_lcd_ghosting / 100.0f;
    const float curStrength   = 1.0f - ghostStrength * 0.6f;

    for (int y = 0; y < result.height(); y++) {
        const QRgb* srcLine  = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        const QRgb* prevLine = reinterpret_cast<const QRgb*>(prev.constScanLine(y));
        QRgb*       dstLine  = reinterpret_cast<QRgb*>(result.scanLine(y));

        for (int x = 0; x < result.width(); x++) {
            int r = static_cast<int>(qRed(srcLine[x])   * curStrength + qRed(prevLine[x])   * ghostStrength * 0.6f);
            int g = static_cast<int>(qGreen(srcLine[x]) * curStrength + qGreen(prevLine[x]) * ghostStrength * 0.6f);
            int b = static_cast<int>(qBlue(srcLine[x])  * curStrength + qBlue(prevLine[x])  * ghostStrength * 0.6f);
            dstLine[x] = qRgb(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// AV (Composite) — Blur horizontal + Bleeding + Noise
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawAVArtifacts(QPainter& painter, const QRect& rect)
{
    // Simula sinal composto com bleed de cores
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.setOpacity(0.15);
    painter.drawImage(rect.adjusted(2, 0, 2, 0), m_scaled_frame); // Red shift
    painter.drawImage(rect.adjusted(-2, 0, -2, 0), m_scaled_frame); // Blue shift
    painter.setOpacity(1.0);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // Leve noise
    painter.setBrush(QColor(255, 255, 255, 5));
    for (int i = 0; i < 500; i++) {
        int x = rect.left() + rand() % rect.width();
        int y = rect.top() + rand() % rect.height();
        painter.drawPoint(x, y);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// S-Video — Sharpness melhorada + Luma/Chroma separadas
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawSVideoArtifacts(QPainter& painter, const QRect& rect)
{
    // S-Video eh bem limpo, mas tem um "glow" sutil nas bordas de contraste
    painter.setOpacity(0.1);
    painter.drawImage(rect.adjusted(1, 0, 1, 0), m_scaled_frame);
    painter.setOpacity(1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// VGA — 31kHz fine scanlines
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawVGAScanlines(QPainter& painter, const QRect& rect)
{
    painter.setPen(QColor(0, 0, 0, 40));
    for (int y = rect.top(); y < rect.bottom(); y += 2) {
        painter.drawLine(rect.left(), y, rect.right(), y);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// RGB Mask — Aperture Grill / Shadow Mask
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawRGBMask(QPainter& painter, const QRect& rect)
{
    painter.setPen(Qt::NoPen);
    for (int x = rect.left(); x < rect.right(); x += 3) {
        painter.setBrush(QColor(255, 0, 0, 30));
        painter.drawRect(x, rect.top(), 1, rect.height());
        painter.setBrush(QColor(0, 255, 0, 30));
        painter.drawRect(x + 1, rect.top(), 1, rect.height());
        painter.setBrush(QColor(0, 0, 255, 30));
        painter.drawRect(x + 2, rect.top(), 1, rect.height());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Bloom — Simula o vazamento de luz de cores brilhantes (Phosphor Bleed)
// ─────────────────────────────────────────────────────────────────────────────
void DisplayWidget::drawBloom(QPainter& painter, const QRect& rect)
{
    if (m_scaled_frame.isNull()) return;

    // Reduz o frame e blura para criar o mapa de bloom
    QImage bloom = m_scaled_frame.scaled(rect.width() / 4, rect.height() / 4, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // Filtro de brilho: mantém só o que é bem brilhante
    for (int y = 0; y < bloom.height(); y++) {
        QRgb* line = reinterpret_cast<QRgb*>(bloom.scanLine(y));
        for (int x = 0; x < bloom.width(); x++) {
            int gray = qGray(line[x]);
            if (gray < 150) line[x] = qRgba(0, 0, 0, 0);
            else {
                // Aumenta a saturação do bloom
                int r = std::min(255, qRed(line[x]) * 2);
                int g = std::min(255, qGreen(line[x]) * 2);
                int b = std::min(255, qBlue(line[x]) * 2);
                line[x] = qRgba(r, g, b, 120);
            }
        }
    }

    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.setOpacity(0.3);
    painter.drawImage(rect, bloom);
    painter.setOpacity(1.0);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
}

void DisplayWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_scaled_frame = QImage();
    m_scaled_frame_rect = QRect();
    emit sizeChanged(event->size());
    update();
}

void DisplayWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_input && !event->isAutoRepeat())
        m_input->setQtKeyState(event->key(), true);
    QWidget::keyPressEvent(event);
}

void DisplayWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (m_input && !event->isAutoRepeat())
        m_input->setQtKeyState(event->key(), false);
    QWidget::keyReleaseEvent(event);
}
