#include "JaguarDebugWindow.h"
#include "../jaguar/JaguarSystem.h"

#include <QPainter>
#include <QPaintEvent>

// VJ core headers
#include "tom.h"
#include "jaguar.h"
#include "memory.h"
#include "settings.h"
#include "m68000/m68kinterface.h"

#include <QFont>
#include <QScrollBar>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>

// TOM register offsets (from tom.cpp)
#define TOM_VMODE   0x28
#define TOM_BORD1   0x2A
#define TOM_HP      0x2E
#define TOM_HBB     0x30
#define TOM_HBE     0x32
#define TOM_HDB1    0x38
#define TOM_HDB2    0x3A
#define TOM_HDE     0x3C
#define TOM_VP      0x3E
#define TOM_VBB     0x40
#define TOM_VBE     0x42
#define TOM_VS      0x44
#define TOM_VDB     0x46
#define TOM_VDE     0x48
#define TOM_VI      0x4E
#define TOM_INT1    0xE0
#define TOM_OLP     0x20
#define TOM_VC      0x06

static inline uint16_t tomReg16(int offset) {
    extern uint8_t tomRam8[];
    return (uint16_t)((tomRam8[offset] << 8) | tomRam8[offset + 1]);
}

// ─────────────────────────────────────────────────────────────────────────────

JaguarDebugWindow::JaguarDebugWindow(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    setWindowTitle(tr("Jaguar Debug — Íris"));
    resize(700, 520);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildTOMTab(),         tr("TOM Registers"));
    m_tabs->addTab(buildCPUTab(),         tr("68K CPU"));
    m_tabs->addTab(buildFramebufferTab(), tr("Framebuffer"));
    m_tabs->addTab(buildVJLogTab(),       tr("VJ Log File"));
    m_tabs->addTab(buildDiagTab(),        tr("Diagnóstico"));
    layout->addWidget(m_tabs, 1);

    auto* btnRow = new QHBoxLayout();
    auto* refreshBtn = new QPushButton(tr("Refresh Now"), this);
    auto* autoCheck  = new QCheckBox(tr("Auto (10 Hz)"), this);
    autoCheck->setChecked(true);
    btnRow->addWidget(refreshBtn);
    btnRow->addWidget(autoCheck);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    m_timer = new QTimer(this);
    m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, this, &JaguarDebugWindow::refresh);
    connect(refreshBtn, &QPushButton::clicked, this, &JaguarDebugWindow::refresh);
    connect(autoCheck, &QCheckBox::toggled, this, [this](bool on) {
        on ? m_timer->start() : m_timer->stop();
    });
    m_timer->start();
}

void JaguarDebugWindow::setCore(JaguarSystem* core)
{
    m_core = core;
    refresh();
}

QLabel* JaguarDebugWindow::makeVal(QWidget* parent)
{
    return makeLabel(parent);
}

// ─── FBCanvas ─────────────────────────────────────────────────────────────────
void FBCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (!m_img.isNull())
        p.drawImage(rect(), m_img);
    else
        p.fillRect(rect(), Qt::black);
}

QLabel* JaguarDebugWindow::makeLabel(QWidget* parent)
{
    auto* l = new QLabel("—", parent);
    QFont f("Courier New", 9);
    f.setStyleHint(QFont::Monospace);
    l->setFont(f);
    l->setMinimumWidth(120);
    return l;
}

// ─── TOM Tab ─────────────────────────────────────────────────────────────────
QWidget* JaguarDebugWindow::buildTOMTab()
{
    auto* w = new QWidget();
    auto* grid = new QGridLayout(w);
    grid->setSpacing(4);

    auto addRow = [&](int row, const QString& name, QLabel*& lbl) {
        grid->addWidget(new QLabel(name + ":", w), row, 0);
        lbl = makeLabel(w);
        grid->addWidget(lbl, row, 1);
    };

    int r = 0;
    addRow(r++, "VMODE",        m_tom_vmode);
    addRow(r++, "PWIDTH",       m_tom_pwidth);
    addRow(r++, "Mode",         m_tom_mode);
    addRow(r++, "tomWidth",     m_tom_width);
    addRow(r++, "tomHeight",    m_tom_height);
    addRow(r++, "VDB",          m_tom_vdb);
    addRow(r++, "VDE",          m_tom_vde);
    addRow(r++, "VI",           m_tom_vi);
    addRow(r++, "HDB1",         m_tom_hdb1);
    addRow(r++, "HDE",          m_tom_hde);
    addRow(r++, "VP",           m_tom_vp);
    addRow(r++, "VC",           m_tom_vc);
    addRow(r++, "INT1",         m_tom_int1);
    addRow(r++, "OLP",          m_tom_olp);
    grid->setRowStretch(r, 1);
    grid->setColumnStretch(2, 1);
    return w;
}

// ─── CPU Tab ─────────────────────────────────────────────────────────────────
QWidget* JaguarDebugWindow::buildCPUTab()
{
    auto* w = new QWidget();
    auto* grid = new QGridLayout(w);
    grid->setSpacing(4);

    int r = 0;
    auto addRow = [&](const QString& name, QLabel*& lbl) {
        grid->addWidget(new QLabel(name + ":", w), r, 0);
        lbl = makeLabel(w);
        grid->addWidget(lbl, r++, 1);
    };

    addRow("PC", m_cpu_pc);
    addRow("SR", m_cpu_sr);
    addRow("SP (A7)", m_cpu_sp);

    for (int i = 0; i < 8; i++) {
        grid->addWidget(new QLabel(QString("D%1:").arg(i), w), r, 0);
        m_cpu_d[i] = makeLabel(w);
        grid->addWidget(m_cpu_d[i], r++, 1);
    }
    for (int i = 0; i < 8; i++) {
        grid->addWidget(new QLabel(QString("A%1:").arg(i), w), r, 0);
        m_cpu_a[i] = makeLabel(w);
        grid->addWidget(m_cpu_a[i], r++, 1);
    }
    grid->setRowStretch(r, 1);
    grid->setColumnStretch(2, 1);
    return w;
}

// ─── Framebuffer Tab ─────────────────────────────────────────────────────────
QWidget* JaguarDebugWindow::buildFramebufferTab()
{
    auto* w = new QWidget();
    auto* grid = new QGridLayout(w);
    grid->setSpacing(4);

    int r = 0;
    auto addRow = [&](const QString& name, QLabel*& lbl) {
        grid->addWidget(new QLabel(name + ":", w), r, 0);
        lbl = makeLabel(w);
        grid->addWidget(lbl, r++, 1);
    };

    addRow("screenPitch",          m_fb_pitch);
    addRow("tomWidth",             m_fb_tomW);
    addRow("tomHeight",            m_fb_tomH);
    addRow("TOMGetVideoModeWidth", m_fb_getW);
    addRow("visW (copy width)",    m_fb_visW);
    addRow("visH (copy height)",   m_fb_visH);
    addRow("jaguarRunAddress",     m_fb_runAddr);
    addRow("jaguarMainROMCRC32",   m_fb_crc);

    auto* hint = new QLabel(tr(
        "<i style='color:#888'>tomWidth = pixels TOM writes per scanline<br>"
        "TOMGetVideoModeWidth = (HC_range / PWIDTH)<br>"
        "If tomWidth != TOMGetVideoModeWidth → pitch mismatch</i>"), w);
    hint->setWordWrap(true);
    grid->addWidget(hint, r++, 0, 1, 2);

    grid->setRowStretch(r, 1);
    grid->setColumnStretch(2, 1);
    return w;
}

// ─── Diag Tab ────────────────────────────────────────────────────────────────
QWidget* JaguarDebugWindow::buildDiagTab()
{
    auto* w = new QWidget();
    auto* layout = new QVBoxLayout(w);

    // Info label
    auto* info = new QLabel(
        tr("<b>Gravação de diagnóstico em tempo real.</b><br>"
           "Captura PC, SR, OLP, INT1, tomWidth e eventos de trava a cada frame.<br>"
           "Útil para identificar onde o Doom (ou qualquer jogo) trava."), w);
    info->setWordWrap(true);
    layout->addWidget(info);

    // Log display
    m_diagLog = new QPlainTextEdit(w);
    m_diagLog->setReadOnly(true);
    m_diagLog->setMaximumBlockCount(5000);
    QFont f("Courier New", 8);
    f.setStyleHint(QFont::Monospace);
    m_diagLog->setFont(f);
    m_diagLog->setStyleSheet("QPlainTextEdit { background:#0d0d0d; color:#ffcc00; }");
    layout->addWidget(m_diagLog, 1);

    // Controls
    auto* row = new QHBoxLayout();
    m_diagStart  = new QPushButton(tr("▶ Iniciar Gravação"), w);
    m_diagStop   = new QPushButton(tr("■ Parar"), w);
    m_diagSave   = new QPushButton(tr("💾 Salvar Como..."), w);
    m_diagFreeze = new QCheckBox(tr("Detectar trava (PC parado)"), w);
    m_diagFreeze->setChecked(true);
    m_diagStop->setEnabled(false);
    m_diagSave->setEnabled(false);
    row->addWidget(m_diagStart);
    row->addWidget(m_diagStop);
    row->addWidget(m_diagSave);
    row->addWidget(m_diagFreeze);
    row->addStretch();
    layout->addLayout(row);

    connect(m_diagStart, &QPushButton::clicked, this, [this]() {
        m_diagLines.clear();
        m_diagLog->clear();
        m_recording = true;
        m_diagStart->setEnabled(false);
        m_diagStop->setEnabled(true);
        m_diagSave->setEnabled(false);
        m_diagLog->appendPlainText(tr("=== Gravação iniciada: %1 ===")
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    });

    connect(m_diagStop, &QPushButton::clicked, this, [this]() {
        m_recording = false;
        m_diagStart->setEnabled(true);
        m_diagStop->setEnabled(false);
        m_diagSave->setEnabled(!m_diagLines.isEmpty());
        m_diagLog->appendPlainText(tr("=== Gravação parada. %1 linhas capturadas. ===")
            .arg(m_diagLines.size()));
    });

    connect(m_diagSave, &QPushButton::clicked, this, [this]() {
        QString defaultName = QDateTime::currentDateTime()
            .toString("'jaguar_diag_'yyyyMMdd_HHmmss'.txt'");
        QString path = QFileDialog::getSaveFileName(
            this,
            tr("Salvar Log de Diagnóstico"),
            defaultName,
            tr("Arquivos de texto (*.txt);;Todos os arquivos (*.*)"));
        if (path.isEmpty()) return;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_diagLog->appendPlainText(tr("ERRO: não foi possível salvar em %1").arg(path));
            return;
        }
        QTextStream ts(&f);
        ts << m_diagLines.join('\n');
        m_diagLog->appendPlainText(tr("Salvo em: %1").arg(path));
    });

    return w;
}

void JaguarDebugWindow::appendDiagLine(const QString& line)
{
    if (!m_recording) return;
    m_diagLines.append(line);
    m_diagPending.append(line);
}

// ─── VJ Log Tab ──────────────────────────────────────────────────────────────
QWidget* JaguarDebugWindow::buildVJLogTab()
{
    auto* w = new QWidget();
    auto* layout = new QVBoxLayout(w);

    m_vjlog = new QPlainTextEdit(w);
    m_vjlog->setReadOnly(true);
    m_vjlog->setMaximumBlockCount(2000);
    QFont f("Courier New", 8);
    f.setStyleHint(QFont::Monospace);
    m_vjlog->setFont(f);
    m_vjlog->setStyleSheet("QPlainTextEdit { background:#0a0a0a; color:#00ff88; }");
    layout->addWidget(m_vjlog, 1);

    auto* btnRow = new QHBoxLayout();
    auto* reloadBtn = new QPushButton(tr("Reload Log File"), w);
    auto* saveBtn   = new QPushButton(tr("Save…"), w);
    btnRow->addWidget(reloadBtn);
    btnRow->addWidget(saveBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(reloadBtn, &QPushButton::clicked, this, [this]() {
        // Read jaguar_iris.log from app dir
        QString logPath = QCoreApplication::applicationDirPath() + "/jaguar_iris.log";
        QFile f(logPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_vjlog->setPlainText(tr("Log file not found: %1").arg(logPath));
            return;
        }
        QString content = QString::fromLocal8Bit(f.readAll());
        // Show last 2000 lines
        QStringList lines = content.split('\n');
        if (lines.size() > 2000)
            lines = lines.mid(lines.size() - 2000);
        m_vjlog->setPlainText(lines.join('\n'));
        m_vjlog->verticalScrollBar()->setValue(m_vjlog->verticalScrollBar()->maximum());
    });

    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("Save VJ Log"),
            QDateTime::currentDateTime().toString("'vj_log_'yyyyMMdd_HHmmss'.txt'"),
            tr("Text files (*.txt)"));
        if (path.isEmpty()) return;
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
            QTextStream(&f) << m_vjlog->toPlainText();
    });

    return w;
}

// ─── Refresh ─────────────────────────────────────────────────────────────────
void JaguarDebugWindow::refresh()
{
    // ── Flush pending diag lines to display ──────────────────────────────────
    if (!m_diagPending.isEmpty()) {
        for (const QString& l : m_diagPending)
            m_diagLog->appendPlainText(l);
        m_diagPending.clear();
        m_diagLog->verticalScrollBar()->setValue(
            m_diagLog->verticalScrollBar()->maximum());
    }

    if (!m_core || !m_core->isRunning()) return;

    // ── TOM ──────────────────────────────────────────────────────────────────
    uint16_t vmode  = tomReg16(TOM_VMODE);
    uint8_t  pwidth = ((vmode >> 9) & 0x07) + 1;
    uint8_t  mode   = (vmode >> 1) & 0x03;
    const char* modeStr[] = { "CRY16", "RGB24", "DIRECT16", "RGB16" };

    m_tom_vmode ->setText(QString("$%1").arg(vmode, 4, 16, QChar('0')).toUpper());
    m_tom_pwidth->setText(QString::number(pwidth));
    m_tom_mode  ->setText(modeStr[mode & 3]);
    m_tom_width ->setText(QString::number(tomWidth));
    m_tom_height->setText(QString::number(tomHeight));
    m_tom_vdb   ->setText(QString::number(tomReg16(TOM_VDB)));
    m_tom_vde   ->setText(QString::number(tomReg16(TOM_VDE)));
    m_tom_vi    ->setText(QString::number(tomReg16(TOM_VI)));
    m_tom_hdb1  ->setText(QString::number(tomReg16(TOM_HDB1)));
    m_tom_hde   ->setText(QString::number(tomReg16(TOM_HDE)));
    m_tom_vp    ->setText(QString::number(tomReg16(TOM_VP)));
    m_tom_vc    ->setText(QString::number(tomReg16(TOM_VC)));
    m_tom_int1  ->setText(QString("$%1").arg(tomReg16(TOM_INT1), 4, 16, QChar('0')).toUpper());
    // OLP is 32-bit at offset 0x20
    {
        extern uint8_t tomRam8[];
        uint32_t olp = ((uint32_t)tomRam8[TOM_OLP] << 24) | ((uint32_t)tomRam8[TOM_OLP+1] << 16)
                     | ((uint32_t)tomRam8[TOM_OLP+2] << 8) | tomRam8[TOM_OLP+3];
        m_tom_olp->setText(QString("$%1").arg(olp, 8, 16, QChar('0')).toUpper());
    }

    // ── 68K CPU ───────────────────────────────────────────────────────────────
    m_cpu_pc->setText(QString("$%1").arg(m68k_get_reg(nullptr, M68K_REG_PC), 6, 16, QChar('0')).toUpper());
    m_cpu_sr->setText(QString("$%1").arg(m68k_get_reg(nullptr, M68K_REG_SR), 4, 16, QChar('0')).toUpper());
    m_cpu_sp->setText(QString("$%1").arg(m68k_get_reg(nullptr, M68K_REG_A7), 8, 16, QChar('0')).toUpper());
    for (int i = 0; i < 8; i++) {
        m_cpu_d[i]->setText(QString("$%1").arg(
            m68k_get_reg(nullptr, (m68k_register_t)(M68K_REG_D0 + i)), 8, 16, QChar('0')).toUpper());
        m_cpu_a[i]->setText(QString("$%1").arg(
            m68k_get_reg(nullptr, (m68k_register_t)(M68K_REG_A0 + i)), 8, 16, QChar('0')).toUpper());
    }

    // ── Framebuffer ───────────────────────────────────────────────────────────
    int calcVisW = (int)(tomWidth > 0 ? tomWidth : TOMGetVideoModeWidth());
    int calcVisH = vjs.hardwareTypeNTSC ? VIRTUAL_SCREEN_HEIGHT_NTSC : VIRTUAL_SCREEN_HEIGHT_PAL;

    m_fb_pitch  ->setText(QString::number(screenPitch));
    m_fb_tomW   ->setText(QString::number(tomWidth));
    m_fb_tomH   ->setText(QString::number(tomHeight));
    m_fb_getW   ->setText(QString::number(TOMGetVideoModeWidth()));
    m_fb_visW   ->setText(QString::number(std::clamp(calcVisW, 40, 800)));
    m_fb_visH   ->setText(QString::number(std::clamp(calcVisH, 100, 576)));
    m_fb_runAddr->setText(QString("$%1").arg(jaguarRunAddress, 8, 16, QChar('0')).toUpper());
    m_fb_crc    ->setText(QString("$%1").arg(jaguarMainROMCRC32, 8, 16, QChar('0')).toUpper());
}
