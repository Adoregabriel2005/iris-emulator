#include "JaguarIntroWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QFont>
#include <SDL.h>
#include <cmath>
#include <vector>

// ─── Boot sound via SDL2 ──────────────────────────────────────────────────────
// Synthesizes the Jaguar BIOS boot sound:
//   - Low thud (80 Hz, ~120 ms)
//   - Rising sweep (200→800 Hz, ~400 ms)
//   - Chord ding A4+E5 (~600 ms, fade out)

static void playJaguarBootSound()
{
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO))
        SDL_InitSubSystem(SDL_INIT_AUDIO);

    const int   SR      = 44100;
    const float TOTAL   = 1.4f;
    const int   SAMPLES = static_cast<int>(SR * TOTAL);

    std::vector<int16_t> buf(SAMPLES * 2);
    for (int i = 0; i < SAMPLES; i++) {
        float t = static_cast<float>(i) / SR;
        float v = 0.f;

        if (t < 0.12f) {
            float e = std::exp(-t * 30.f);
            v += std::sin(2.f * M_PI * 80.f * t) * e * 0.6f;
            v += std::sin(2.f * M_PI * 40.f * t) * e * 0.3f;
        }
        if (t >= 0.10f && t < 0.50f) {
            float st = (t - 0.10f) / 0.40f;
            float f  = 200.f + st * st * 600.f;
            v += std::sin(2.f * M_PI * f * t) * std::sin(st * M_PI) * 0.5f;
        }
        if (t >= 0.45f) {
            float e = std::exp(-(t - 0.45f) * 2.8f) * 0.55f;
            v += std::sin(2.f * M_PI * 440.f * t) * e;
            v += std::sin(2.f * M_PI * 659.f * t) * e * 0.7f;
            v += std::sin(2.f * M_PI * 880.f * t) * e * 0.15f;
        }

        int16_t s = static_cast<int16_t>(std::clamp(v, -1.f, 1.f) * 28000.f);
        buf[i * 2 + 0] = s;
        buf[i * 2 + 1] = s;
    }

    SDL_AudioSpec spec{};
    spec.freq     = SR;
    spec.format   = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples  = 2048;
    spec.callback = nullptr;

    static SDL_AudioDeviceID s_dev = 0;
    if (s_dev) { SDL_CloseAudioDevice(s_dev); s_dev = 0; }

    s_dev = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    if (!s_dev) return;
    SDL_QueueAudio(s_dev, buf.data(), static_cast<Uint32>(buf.size() * sizeof(int16_t)));
    SDL_PauseAudioDevice(s_dev, 0);
}

// ─── JaguarIntroWidget ────────────────────────────────────────────────────────

JaguarIntroWidget::JaguarIntroWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &JaguarIntroWidget::onTick);
}

JaguarIntroWidget::~JaguarIntroWidget() {}

void JaguarIntroWidget::play()
{
    if (m_playing) return;
    m_playing = true;
    m_elapsed.start();
    playJaguarBootSound();
    m_timer->start();
    show();
    raise();
    update();
}

void JaguarIntroWidget::onTick()
{
    if (static_cast<float>(m_elapsed.elapsed()) >= DURATION_MS) {
        m_timer->stop();
        m_playing = false;
        hide();
        emit finished();
        return;
    }
    update();
}

void JaguarIntroWidget::paintEvent(QPaintEvent*)
{
    float t = std::clamp(static_cast<float>(m_elapsed.elapsed()) / DURATION_MS, 0.f, 1.f);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    renderFrame(p, t);
}

// ─── renderFrame ─────────────────────────────────────────────────────────────
// Timeline (t = 0..1, ~3.2 s total):
//   0.00–0.08  fade in from black
//   0.08–0.35  Atari Fuji logo spins in
//   0.35–0.60  "ATARI" text fades in
//   0.60–0.80  "JAGUAR" text fades in, logo pulses
//   0.80–0.92  hold
//   0.92–1.00  fade to black

void JaguarIntroWidget::renderFrame(QPainter& p, float t)
{
    const int W = width(), H = height();
    const QPointF C(W * 0.5f, H * 0.5f);

    // Background
    float bgA = std::min(t / 0.08f, 1.f);
    if (t > 0.92f) bgA = (1.f - t) / 0.08f;
    p.fillRect(rect(), QColor(0, 0, 0));
    QRadialGradient bg(C, std::max(W, H) * 0.7f);
    bg.setColorAt(0.0, QColor(10, 8, 28, static_cast<int>(bgA * 255)));
    bg.setColorAt(1.0, QColor(0,  0,  0, static_cast<int>(bgA * 255)));
    p.fillRect(rect(), bg);
    if (t < 0.06f) return;

    // Logo
    float lT    = std::clamp((t - 0.08f) / 0.27f, 0.f, 1.f);
    float lEase = 1.f - std::pow(1.f - lT, 3.f);
    float pulse = (t > 0.60f && t < 0.92f)
        ? std::sin((t - 0.60f) / 0.32f * M_PI * 2.f) * 0.04f : 0.f;
    float lScale = lEase * (1.f + pulse);
    float lAngle = (1.f - lEase) * 180.f;
    float lAlpha = lEase;
    if (t > 0.92f) lAlpha *= (1.f - t) / 0.08f;
    float lSize  = std::min(W, H) * 0.28f * lScale;

    p.save();
    p.translate(C);
    p.rotate(lAngle);
    p.scale(lSize / 100.f, lSize / 100.f);

    // Glow
    if (lAlpha > 0.01f) {
        QRadialGradient glow(QPointF(0,0), 120);
        glow.setColorAt(0.0, QColor(180, 60, 255, static_cast<int>(lAlpha * 80)));
        glow.setColorAt(0.5, QColor(80,  20, 180, static_cast<int>(lAlpha * 40)));
        glow.setColorAt(1.0, QColor(0,    0,   0, 0));
        p.setBrush(glow); p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(0,0), 120, 120);
    }

    // Fuji mountains
    auto mountain = [&](float cx, float by, float ty, float bw, float tw, QColor col) {
        QPainterPath path;
        path.moveTo(cx - bw/2, by);
        path.lineTo(cx - tw/2, ty);
        path.arcTo(QRectF(cx - tw/2, ty - tw*0.3f, tw, tw*0.6f), 180, -180);
        path.lineTo(cx + bw/2, by);
        path.closeSubpath();
        col.setAlphaF(lAlpha);
        p.setBrush(col); p.setPen(Qt::NoPen);
        p.drawPath(path);
    };
    mountain(  0, 55, -55, 70, 38, QColor(240, 235, 255));
    mountain(-52, 55,  -5, 52, 28, QColor(200, 190, 230));
    mountain( 52, 55,  -5, 52, 28, QColor(200, 190, 230));

    for (int i = 0; i < 3; i++) {
        float bw = 90.f - i * 8.f;
        p.setBrush(QColor(220, 210, 255, static_cast<int>(lAlpha * 255)));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(-bw/2, 60.f + i*14.f, bw, 9), 3, 3);
    }
    p.restore();

    // "ATARI"
    float aA = std::clamp((t - 0.35f) / 0.20f, 0.f, 1.f);
    if (t > 0.92f) aA *= (1.f - t) / 0.08f;
    if (aA > 0.01f) {
        QFont f("Arial", 1, QFont::Bold);
        f.setPixelSize(static_cast<int>(std::min(W,H) * 0.065f));
        f.setLetterSpacing(QFont::AbsoluteSpacing, std::min(W,H) * 0.018f);
        p.setFont(f);
        p.setPen(QColor(230, 220, 255, static_cast<int>(aA * 255)));
        float y = C.y() + std::min(W,H) * 0.22f;
        p.drawText(QRectF(0, y-30, W, 60), Qt::AlignHCenter|Qt::AlignVCenter, "ATARI");
    }

    // "JAGUAR"
    float jA = std::clamp((t - 0.60f) / 0.18f, 0.f, 1.f);
    if (t > 0.92f) jA *= (1.f - t) / 0.08f;
    if (jA > 0.01f) {
        QFont f("Arial", 1, QFont::Bold);
        f.setPixelSize(static_cast<int>(std::min(W,H) * 0.048f));
        f.setLetterSpacing(QFont::AbsoluteSpacing, std::min(W,H) * 0.022f);
        p.setFont(f);
        p.setPen(QColor(255, 140, 30, static_cast<int>(jA * 255)));
        float y = C.y() + std::min(W,H) * 0.30f;
        p.drawText(QRectF(0, y-25, W, 50), Qt::AlignHCenter|Qt::AlignVCenter, "JAGUAR");
    }

    // Scanlines
    if (t > 0.08f && t < 0.92f) {
        float a = std::min((t - 0.08f) / 0.10f, 1.f) * 0.18f;
        p.setPen(QPen(QColor(0, 0, 0, static_cast<int>(a * 255)), 1));
        for (int y = 0; y < H; y += 3)
            p.drawLine(0, y, W, y);
    }
}
