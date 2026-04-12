#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QtGui/QImage>
#include <QtGui/QPaintEvent>
#include <QtGui/QKeyEvent>
#include <memory>

class SDLInput;

class DisplayWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum class ScreenFilter {
        None,
        Scanlines,
        CRT,
        Smooth,
        LCD,
        LCDBlur,
        RFJaguar,    // RF cable simulation (Jaguar exclusive)
        SVideoJaguar // S-Video simulation (Jaguar exclusive)
    };

    enum class AspectRatio {
        Auto,    // keep source aspect
        FourThree,
        SixteenNine
    };

    enum class JagUpscale {
        Native,  // original resolution
        P480,    // 480p
        P720,    // 720p
        P1080    // 1080p
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

    // Jaguar-specific
    void setAspectRatio(AspectRatio ar);
    void setJagUpscale(JagUpscale up);
    AspectRatio aspectRatio() const { return m_aspect; }
    JagUpscale  jagUpscale()  const { return m_upscale; }
    void setIsJaguar(bool jag) { m_is_jaguar = jag; }

    void setInput(SDLInput* input) { m_input = input; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

Q_SIGNALS:
    void sizeChanged(const QSize& newSize);

private:
    QRect calculateDisplayRect() const;
    QImage applyLCDBlur(const QImage& src) const;

    void uploadTexture(const QImage& img);

    QImage m_current_frame;
    QImage m_prev_frame;
    ScreenFilter m_filter = ScreenFilter::None;
    int m_scanline_intensity = 50;
    int m_lcd_ghosting = 40;
    AspectRatio m_aspect   = AspectRatio::Auto;
    JagUpscale  m_upscale  = JagUpscale::Native;
    bool        m_is_jaguar = false;
    int         m_frame_counter = 0;
    SDLInput* m_input = nullptr;

    // OpenGL resources
    std::unique_ptr<QOpenGLShaderProgram> m_shader;
    std::unique_ptr<QOpenGLTexture>       m_texture;
    QOpenGLBuffer                         m_vbo;
    QOpenGLVertexArrayObject              m_vao;
    bool m_gl_initialized = false;
};
