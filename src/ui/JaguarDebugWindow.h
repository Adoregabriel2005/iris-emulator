#pragma once
#include <QDialog>
#include <QTimer>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QScrollArea>
#include <QStringList>
#include <QImage>
#include <cstdint>

class JaguarSystem;

// ─── Framebuffer canvas ──────────────────────────────────────────────────────
class FBCanvas : public QWidget {
    Q_OBJECT
public:
    explicit FBCanvas(QWidget* p = nullptr) : QWidget(p) {}
    void setImage(const QImage& img) { m_img = img; update(); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QImage m_img;
};

// ─── Main debug window ───────────────────────────────────────────────────────
class JaguarDebugWindow : public QDialog
{
    Q_OBJECT
public:
    explicit JaguarDebugWindow(QWidget* parent = nullptr);
    void setCore(JaguarSystem* core);

    // Called by JaguarSystem::step() — zero cost when window is closed
    void appendDiagLine(const QString& line);

    // Memory watchpoint hit — called from write hooks
    void onMemoryWrite(uint32_t addr, uint32_t val, int who);

    // Blitter operation started — called from blitter hook
    void onBlitterOp(uint32_t dst, uint32_t src, uint32_t w, uint32_t h,
                     uint32_t cmd, uint32_t dstFlags, uint32_t srcFlags);

private Q_SLOTS:
    void refresh();

private:
    // Tab builders
    QWidget* buildTOMTab();
    QWidget* buildCPUTab();
    QWidget* buildGPUTab();
    QWidget* buildDSPTab();
    QWidget* buildBlitterTab();
    QWidget* buildFramebufferTab();
    QWidget* buildMemWatchTab();
    QWidget* buildDiagTab();
    QWidget* buildVJLogTab();

    QLabel* makeLabel(QWidget* p);
    QLabel* makeVal(QWidget* p);
    void    makeRegRow(QGridLayout* g, int row, const QString& name, QLabel*& lbl, QWidget* p);

    JaguarSystem* m_core  = nullptr;
    QTimer*       m_timer = nullptr;
    QTabWidget*   m_tabs  = nullptr;

    // ── TOM Registers tab ────────────────────────────────────────────────────
    QLabel* m_tom_vmode  = nullptr, *m_tom_pwidth = nullptr, *m_tom_mode   = nullptr;
    QLabel* m_tom_width  = nullptr, *m_tom_height = nullptr;
    QLabel* m_tom_vdb    = nullptr, *m_tom_vde    = nullptr, *m_tom_vi     = nullptr;
    QLabel* m_tom_hdb1   = nullptr, *m_tom_hde    = nullptr;
    QLabel* m_tom_vp     = nullptr, *m_tom_vc     = nullptr;
    QLabel* m_tom_int1   = nullptr, *m_tom_olp    = nullptr;

    // ── CPU tab ──────────────────────────────────────────────────────────────
    QLabel* m_cpu_pc = nullptr, *m_cpu_sr = nullptr, *m_cpu_sp = nullptr;
    QLabel* m_cpu_d[8]{}, *m_cpu_a[8]{};

    // ── GPU tab ──────────────────────────────────────────────────────────────
    QLabel* m_gpu_pc      = nullptr;
    QLabel* m_gpu_ctrl    = nullptr;
    QLabel* m_gpu_running = nullptr;
    QLabel* m_gpu_r[32]{};

    // ── DSP tab ──────────────────────────────────────────────────────────────
    QLabel* m_dsp_pc      = nullptr;
    QLabel* m_dsp_ctrl    = nullptr;
    QLabel* m_dsp_running = nullptr;
    QLabel* m_dsp_r[32]{};

    // ── Blitter tab ──────────────────────────────────────────────────────────
    QLabel* m_blt_a1base  = nullptr, *m_blt_a2base  = nullptr;
    QLabel* m_blt_a1flags = nullptr, *m_blt_a2flags = nullptr;
    QLabel* m_blt_a1pixel = nullptr, *m_blt_a2pixel = nullptr;
    QLabel* m_blt_cmd     = nullptr, *m_blt_count   = nullptr;
    QLabel* m_blt_working = nullptr;
    QPlainTextEdit* m_blt_log = nullptr;
    QStringList     m_blt_pending;

    // ── Framebuffer tab ──────────────────────────────────────────────────────
    FBCanvas*  m_fb_canvas  = nullptr;
    QLabel*    m_fb_info    = nullptr;
    QCheckBox* m_fb_live    = nullptr;
    QPushButton* m_fb_dump  = nullptr;
    QLabel*    m_fb_tomW    = nullptr, *m_fb_tomH    = nullptr;
    QLabel*    m_fb_pitch   = nullptr, *m_fb_mode    = nullptr;
    QLabel*    m_fb_getW    = nullptr, *m_fb_visW    = nullptr;
    QLabel*    m_fb_visH    = nullptr, *m_fb_runAddr = nullptr;
    QLabel*    m_fb_crc     = nullptr;

    // ── Memory Watch tab ─────────────────────────────────────────────────────
    QLineEdit*      m_mw_addr    = nullptr;
    QLineEdit*      m_mw_size    = nullptr;
    QPushButton*    m_mw_add     = nullptr;
    QPushButton*    m_mw_clear   = nullptr;
    QPlainTextEdit* m_mw_log     = nullptr;
    QStringList     m_mw_pending;

    struct WatchPoint { uint32_t base; uint32_t size; };
    QList<WatchPoint> m_watchpoints;

    // ── Diag tab ─────────────────────────────────────────────────────────────
    QPlainTextEdit* m_diagLog    = nullptr;
    QPushButton*    m_diagStart  = nullptr;
    QPushButton*    m_diagStop   = nullptr;
    QPushButton*    m_diagSave   = nullptr;
    QCheckBox*      m_diagFreeze = nullptr;
    bool            m_recording  = false;
    QStringList     m_diagLines;
    QStringList     m_diagPending;

    // ── VJ Log tab ───────────────────────────────────────────────────────────
    QPlainTextEdit* m_vjlog = nullptr;
};
