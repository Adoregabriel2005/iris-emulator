#include "DisplayWidget.h"
#include "../SDLInput.h"

#include <QtCore/QSettings>
#include <QtGui/QResizeEvent>
#include <algorithm>
#include <cmath>

// ─── Vertex shader ───────────────────────────────────────────────────────────
static const char* VS_SRC = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

// ─── Fragment shader — all filters in one ────────────────────────────────────
static const char* FS_SRC = R"(
#version 330 core
in vec2 vUV;
out vec4 fragColor;

uniform sampler2D uTex;
uniform int   uFilter;       // 0=None 1=Scanlines 2=CRT 3=Smooth 4=LCD 5=LCDBlur 6=RF 7=SVideo
uniform float uIntensity;    // 0..1
uniform vec2  uResolution;   // widget size in pixels
uniform vec2  uTexSize;      // texture (frame) size in pixels
uniform float uTime;         // frame counter for RF noise

// ── Scanline ──
float scanline(vec2 uv) {
    float line = floor(uv.y * uTexSize.y);
    return mod(line, 2.0) < 1.0 ? 1.0 : (1.0 - uIntensity * 0.7);
}

// ── Phosphor RGB mask ──
vec3 rgbMask(vec2 uv) {
    int col = int(mod(uv.x * uResolution.x, 3.0));
    if (col == 0) return vec3(1.0 + uIntensity * 0.15, 0.85, 0.85);
    if (col == 1) return vec3(0.85, 1.0 + uIntensity * 0.10, 0.85);
    return vec3(0.85, 0.85, 1.0 + uIntensity * 0.15);
}

// ── Vignette ──
float vignette(vec2 uv) {
    vec2 d = uv - 0.5;
    return 1.0 - dot(d, d) * uIntensity * 2.5;
}

// ── LCD grid ──
float lcdGrid(vec2 uv) {
    vec2 pixel = uv * uTexSize;
    vec2 frac  = fract(pixel);
    float gapX = step(0.88, frac.x);
    float gapY = step(0.88, frac.y);
    return 1.0 - max(gapX, gapY) * uIntensity * 0.85;
}

// ── Pseudo-random noise (cheap hash) ──
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// ── RF Cable simulation ──
// Simulates: composite chroma bleed, horizontal smear, dot crawl,
// luminance noise, color desaturation, slight vertical roll jitter
vec4 rfFilter(vec2 uv) {
    // Horizontal chroma bleed — sample color slightly offset
    float bleed = uIntensity * 0.012;
    vec4 cR = texture(uTex, uv + vec2( bleed, 0.0));
    vec4 cG = texture(uTex, uv);
    vec4 cB = texture(uTex, uv - vec2( bleed, 0.0));
    vec3 col = vec3(cR.r, cG.g, cB.b);

    // Luma/chroma separation smear (horizontal blur)
    float smear = uIntensity * 0.006;
    vec3 smearCol = vec3(0.0);
    for (int i = -2; i <= 2; i++) {
        smearCol += texture(uTex, uv + vec2(float(i) * smear, 0.0)).rgb;
    }
    smearCol /= 5.0;
    col = mix(col, smearCol, uIntensity * 0.45);

    // Dot crawl — diagonal interference pattern
    float crawl = sin((uv.x + uv.y) * uTexSize.x * 0.5 + uTime * 0.3) * 0.5 + 0.5;
    col += crawl * uIntensity * 0.04;

    // Luminance noise
    float noise = (hash(uv + vec2(uTime * 0.01, 0.0)) - 0.5) * uIntensity * 0.12;
    col += noise;

    // Scanlines (RF TVs always had them)
    float sl = scanline(uv);
    col *= sl;

    // Color desaturation (RF degrades chroma)
    float luma = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(col, vec3(luma), uIntensity * 0.30);

    // Slight brightness drop + warm tint
    col *= 0.88 + uIntensity * 0.05;
    col.r *= 1.0 + uIntensity * 0.06;
    col.b *= 1.0 - uIntensity * 0.04;

    // Vignette
    col *= vignette(uv);

    return vec4(clamp(col, 0.0, 1.0), 1.0);
}

// ── S-Video simulation ──
// S-Video separates luma (Y) and chroma (C), so:
// - No chroma bleed (sharp edges)
// - Slight luma softening (Y bandwidth ~5 MHz)
// - Chroma is slightly softer than luma
// - Mild scanlines (S-Video on a CRT)
// - Subtle color boost (S-Video had better saturation than RF/composite)
vec4 svideoFilter(vec2 uv) {
    // Luma channel: slight horizontal softening
    float lumaBlur = 1.0 / uTexSize.x * uIntensity * 0.8;
    vec3 c0 = texture(uTex, uv).rgb;
    vec3 cL = texture(uTex, uv - vec2(lumaBlur, 0.0)).rgb;
    vec3 cR2 = texture(uTex, uv + vec2(lumaBlur, 0.0)).rgb;
    vec3 col = (c0 * 2.0 + cL + cR2) / 4.0;

    // Chroma softening (wider blur on color channels)
    float chromaBlur = 1.0 / uTexSize.x * uIntensity * 2.0;
    vec3 chromaL = texture(uTex, uv - vec2(chromaBlur, 0.0)).rgb;
    vec3 chromaR = texture(uTex, uv + vec2(chromaBlur, 0.0)).rgb;
    vec3 chroma = (chromaL + col + chromaR) / 3.0;

    // Reconstruct: keep luma sharp, soften chroma
    float luma = dot(col, vec3(0.299, 0.587, 0.114));
    vec3 lumaVec = vec3(luma);
    col = lumaVec + (chroma - lumaVec) * (1.0 - uIntensity * 0.25);

    // Mild scanlines (CRT with S-Video)
    float sl = 1.0 - (1.0 - scanline(uv)) * uIntensity * 0.5;
    col *= sl;

    // Color boost — S-Video had richer saturation vs RF
    float lumaFinal = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(lumaFinal), col, 1.0 + uIntensity * 0.18);

    // Subtle phosphor warmth
    col.r *= 1.0 + uIntensity * 0.03;
    col.g *= 1.0 + uIntensity * 0.01;

    // Light vignette
    col *= 1.0 - dot(uv - 0.5, uv - 0.5) * uIntensity * 0.6;

    return vec4(clamp(col, 0.0, 1.0), 1.0);
}

void main() {
    vec4 col = texture(uTex, vUV);

    if (uFilter == 0) { // None
        fragColor = col;
    }
    else if (uFilter == 1) { // Scanlines
        float sl = scanline(vUV);
        fragColor = vec4(col.rgb * sl, col.a);
    }
    else if (uFilter == 2) { // CRT
        float sl  = scanline(vUV);
        vec3  rgb = col.rgb * sl * rgbMask(vUV);
        float vig = vignette(vUV);
        fragColor = vec4(rgb * vig, col.a);
    }
    else if (uFilter == 3) { // Smooth
        fragColor = col;
    }
    else if (uFilter == 4) { // LCD
        float grid = lcdGrid(vUV);
        fragColor = vec4(col.rgb * grid, col.a);
    }
    else if (uFilter == 5) { // LCDBlur
        float grid = lcdGrid(vUV);
        fragColor = vec4(col.rgb * grid, col.a);
    }
    else if (uFilter == 6) { // RF Jaguar
        fragColor = rfFilter(vUV);
    }
    else { // S-Video Jaguar
        fragColor = svideoFilter(vUV);
    }
}
)";

// ─────────────────────────────────────────────────────────────────────────────

DisplayWidget::DisplayWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(160, 120);

    QSettings settings;
    QString filterStr = settings.value("Video/Filter", "none").toString();
    if      (filterStr == "scanlines") m_filter = ScreenFilter::Scanlines;
    else if (filterStr == "crt")       m_filter = ScreenFilter::CRT;
    else if (filterStr == "smooth")    m_filter = ScreenFilter::Smooth;
    else if (filterStr == "lcd")       m_filter = ScreenFilter::LCD;
    else if (filterStr == "lcdblur")   m_filter = ScreenFilter::LCDBlur;
    else if (filterStr == "rf")        m_filter = ScreenFilter::RFJaguar;
    else if (filterStr == "svideo")    m_filter = ScreenFilter::SVideoJaguar;
    else                               m_filter = ScreenFilter::None;

    m_scanline_intensity = settings.value("Video/ScanlineIntensity", 50).toInt();
    m_lcd_ghosting       = settings.value("Video/LCDGhosting", 40).toInt();

    // Request OpenGL 3.3 Core
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSwapInterval(1); // VSync
    setFormat(fmt);
}

DisplayWidget::~DisplayWidget()
{
    makeCurrent();
    m_shader.reset();
    m_texture.reset();
    m_vbo.destroy();
    m_vao.destroy();
    doneCurrent();
}

void DisplayWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Compile shaders
    m_shader = std::make_unique<QOpenGLShaderProgram>();
    m_shader->addShaderFromSourceCode(QOpenGLShader::Vertex,   VS_SRC);
    m_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, FS_SRC);
    m_shader->link();

    // Full-screen quad: pos(x,y) + uv(u,v)
    // NDC: (-1,-1) bottom-left, (1,1) top-right
    // UV:  (0,0) top-left, (1,1) bottom-right  → flip Y
    float verts[] = {
        -1.f, -1.f,  0.f, 1.f,
         1.f, -1.f,  1.f, 1.f,
        -1.f,  1.f,  0.f, 0.f,
         1.f,  1.f,  1.f, 0.f,
    };

    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(verts, sizeof(verts));

    m_shader->bind();
    m_shader->setAttributeBuffer(0, GL_FLOAT, 0,                2, 4 * sizeof(float));
    m_shader->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float), 2, 4 * sizeof(float));
    m_shader->enableAttributeArray(0);
    m_shader->enableAttributeArray(1);
    m_shader->release();

    m_vao.release();
    m_vbo.release();

    m_gl_initialized = true;
}

void DisplayWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    emit sizeChanged(QSize(w, h));
}

void DisplayWidget::uploadTexture(const QImage& img)
{
    if (img.isNull()) return;

    QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);

    if (!m_texture || m_texture->width() != rgba.width() || m_texture->height() != rgba.height()) {
        m_texture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
        m_texture->setSize(rgba.width(), rgba.height());
        m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
        m_texture->allocateStorage();
    }

    bool smooth = (m_filter == ScreenFilter::Smooth || m_filter == ScreenFilter::CRT
                 || m_filter == ScreenFilter::SVideoJaguar);
    // RF uses nearest for the authentic pixelated look
    bool nearest = (m_filter == ScreenFilter::RFJaguar || m_filter == ScreenFilter::None
                  || m_filter == ScreenFilter::LCD || m_filter == ScreenFilter::LCDBlur);
    m_texture->setMinificationFilter(smooth  ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest);
    m_texture->setMagnificationFilter(smooth  ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest);
    m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);

    m_texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, rgba.constBits());
}

void DisplayWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (m_current_frame.isNull() || !m_gl_initialized || !m_shader)
        return;

    // For LCDBlur: blend on CPU before upload
    QImage frameToDraw = m_current_frame;
    if (m_filter == ScreenFilter::LCDBlur && !m_prev_frame.isNull()
        && m_prev_frame.size() == m_current_frame.size())
        frameToDraw = applyLCDBlur(m_current_frame);

    // Jaguar upscaling: scale frame to target resolution before upload
    if (m_is_jaguar && m_upscale != JagUpscale::Native && !frameToDraw.isNull()) {
        int targetH = frameToDraw.height();
        switch (m_upscale) {
            case JagUpscale::P480:  targetH = 480;  break;
            case JagUpscale::P720:  targetH = 720;  break;
            case JagUpscale::P1080: targetH = 1080; break;
            default: break;
        }
        if (targetH != frameToDraw.height()) {
            // Maintain source aspect for the upscale
            int targetW = frameToDraw.width() * targetH / frameToDraw.height();
            frameToDraw = frameToDraw.scaled(targetW, targetH,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
    }

    uploadTexture(frameToDraw);
    if (!m_texture) return;

    // Compute letterbox rect in NDC
    QRect dr = calculateDisplayRect();
    float x0 = (2.f * dr.left()   / width())  - 1.f;
    float x1 = (2.f * dr.right()  / width())  - 1.f;
    float y0 = 1.f - (2.f * dr.bottom() / height());
    float y1 = 1.f - (2.f * dr.top()    / height());

    float verts[] = {
        x0, y0,  0.f, 1.f,
        x1, y0,  1.f, 1.f,
        x0, y1,  0.f, 0.f,
        x1, y1,  1.f, 0.f,
    };

    m_vbo.bind();
    m_vbo.write(0, verts, sizeof(verts));
    m_vbo.release();

    m_shader->bind();
    m_texture->bind(0);
    m_shader->setUniformValue("uTex",        0);
    m_shader->setUniformValue("uFilter",     static_cast<int>(m_filter));
    m_shader->setUniformValue("uIntensity",  m_scanline_intensity / 100.f);
    m_shader->setUniformValue("uResolution", QVector2D(width(), height()));
    m_shader->setUniformValue("uTexSize",    QVector2D(frameToDraw.width(), frameToDraw.height()));
    m_shader->setUniformValue("uTime",       static_cast<float>(m_frame_counter));

    m_vao.bind();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_vao.release();

    m_texture->release();
    m_shader->release();
    m_frame_counter++;
}

QRect DisplayWidget::calculateDisplayRect() const
{
    if (m_current_frame.isNull())
        return rect();

    float display_aspect;

    if (m_is_jaguar) {
        switch (m_aspect) {
            case AspectRatio::FourThree:    display_aspect = 4.f / 3.f; break;
            case AspectRatio::SixteenNine:  display_aspect = 16.f / 9.f; break;
            default: // Auto: use source pixel aspect
                display_aspect = static_cast<float>(m_current_frame.width())
                               / static_cast<float>(m_current_frame.height());
                break;
        }
    } else if (m_current_frame.height() <= 102) {
        // Lynx: use pixel aspect
        display_aspect = static_cast<float>(m_current_frame.width())
                       / static_cast<float>(m_current_frame.height());
    } else {
        // 2600: always 4:3
        display_aspect = 4.f / 3.f;
    }

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

void DisplayWidget::updateFrame(const QImage& frame)
{
    if (m_filter == ScreenFilter::LCDBlur && !m_current_frame.isNull())
        m_prev_frame = m_current_frame;

    m_current_frame = frame;
    update();
}

void DisplayWidget::clearFrame()
{
    m_current_frame = QImage();
    m_prev_frame    = QImage();
    update();
}

void DisplayWidget::setScreenFilter(ScreenFilter filter)
{
    m_filter = filter;
    m_texture.reset(); // force re-upload with new filter params
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

void DisplayWidget::setAspectRatio(AspectRatio ar)
{
    m_aspect = ar;
    update();
}

void DisplayWidget::setJagUpscale(JagUpscale up)
{
    m_upscale = up;
    m_texture.reset();
    update();
}

QImage DisplayWidget::applyLCDBlur(const QImage& src) const
{
    if (m_prev_frame.isNull() || m_prev_frame.size() != src.size())
        return src;

    QImage result = src.convertToFormat(QImage::Format_ARGB32);
    const QImage prev = m_prev_frame.convertToFormat(QImage::Format_ARGB32);

    const float ghostStrength = m_lcd_ghosting / 100.f;
    const float curStrength   = 1.f - ghostStrength * 0.6f;

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

void DisplayWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_input && !event->isAutoRepeat())
        m_input->setQtKeyState(event->key(), true);
    QOpenGLWidget::keyPressEvent(event);
}

void DisplayWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (m_input && !event->isAutoRepeat())
        m_input->setQtKeyState(event->key(), false);
    QOpenGLWidget::keyReleaseEvent(event);
}
