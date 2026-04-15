#pragma once

#include <QtWidgets/QWidget>
#include <QtGui/QImage>
#include <QtGui/QPaintEvent>
#include <QtGui/QKeyEvent>

class SDLInput;

class DisplayWidget : public QWidget
{
    Q_OBJECT

public:
    enum class ScreenFilter {
        None,
        Scanlines,   // CRT scanlines classicas
        CRT,         // CRT completo: scanlines + vignette + curvatura + phosphor
        Smooth,      // Bilinear suave
        LCD,         // Grid LCD — especifico pro Lynx
        LCDBlur      // LCD com ghosting/blur — simula tela original do Lynx
    };

    explicit DisplayWidget(QWidget* parent = nullptr);
    ~DisplayWidget();

    void updateFrame(const QImage& frame);
    void clearFrame();

    bool hasFrame() const { return !m_current_frame.isNull(); }

    void setScreenFilter(ScreenFilter filter);
    ScreenFilter screenFilter() const { return m_filter; }
    void setScanlineIntensity(int percent);
    void setLCDGhostingStrength(int percent);

    void setInput(SDLInput* input) { m_input = input; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

Q_SIGNALS:
    void sizeChanged(const QSize& newSize);

private:
    QRect calculateDisplayRect() const;
    void drawScanlines(QPainter& painter, const QRect& rect);
    void drawCRT(QPainter& painter, const QRect& rect);
    void drawLCD(QPainter& painter, const QRect& rect);
    QImage applyLCDBlur(const QImage& src) const;

    QImage m_current_frame;
    QImage m_prev_frame;          // para ghosting do LCD
    QImage m_scaled_frame;
    QRect m_scaled_frame_rect;
    quint64 m_frame_cache_key = 0;
    ScreenFilter m_filter = ScreenFilter::None;
    int m_scanline_intensity = 50;
    int m_lcd_ghosting = 40;      // 0-100
    SDLInput* m_input = nullptr;
};
