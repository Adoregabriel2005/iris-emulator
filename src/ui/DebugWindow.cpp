#include "DebugWindow.h"
#include "IEmulatorCore.h"
#include "EmulatorCore.h"
#include "LynxSystem.h"
#include "CPU6507.h"
#include "TIA.h"
#include "RIOT.h"

#include <QBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QFont>
#include <QTimer>

DebugWindow::DebugWindow(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    setWindowTitle(tr("Debug — Íris Emulator"));
    setMinimumSize(520, 400);
    resize(600, 460);

    m_refresh_timer = new QTimer(this);
    m_refresh_timer->setInterval(100); // 10 Hz refresh
    connect(m_refresh_timer, &QTimer::timeout, this, &DebugWindow::refresh);

    buildUi();
}

void DebugWindow::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    m_tabs = new QTabWidget(this);

    // ── CPU Tab ──
    auto* cpuTab = new QWidget();
    auto* cpuLayout = new QVBoxLayout(cpuTab);

    auto* cpuGroup = new QGroupBox(tr("CPU Registers"), cpuTab);
    auto* cpuForm  = new QFormLayout(cpuGroup);
    cpuForm->setHorizontalSpacing(16);

    QFont mono("Courier New", 10);
    mono.setStyleHint(QFont::Monospace);

    auto makeReg = [&](const QString& label) -> QLabel* {
        auto* l = new QLabel("--", cpuGroup);
        l->setFont(mono);
        l->setMinimumWidth(80);
        cpuForm->addRow(label, l);
        return l;
    };

    m_cpu_pc    = makeReg("PC:");
    m_cpu_a     = makeReg("A:");
    m_cpu_x     = makeReg("X:");
    m_cpu_y     = makeReg("Y:");
    m_cpu_sp    = makeReg("SP:");
    m_cpu_p     = makeReg("P:");
    m_cpu_flags = makeReg("Flags:");

    cpuLayout->addWidget(cpuGroup);
    cpuLayout->addStretch();
    m_tabs->addTab(cpuTab, tr("CPU"));

    // ── Chip Tab (TIA / Mikey) ──
    auto* chipTab = new QWidget();
    auto* chipLayout = new QVBoxLayout(chipTab);
    m_chip_dump = new QTextEdit(chipTab);
    m_chip_dump->setReadOnly(true);
    m_chip_dump->setFont(mono);
    m_chip_dump->setStyleSheet("background: #111; color: #0f0;");
    chipLayout->addWidget(m_chip_dump);
    m_tabs->addTab(chipTab, tr("TIA / Chip"));

    // ── RAM Tab ──
    auto* ramTab = new QWidget();
    auto* ramLayout = new QVBoxLayout(ramTab);
    m_ram_table = new QTableWidget(8, 16, ramTab);
    m_ram_table->setFont(mono);
    m_ram_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_ram_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_ram_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ram_table->setSelectionMode(QAbstractItemView::SingleSelection);

    // Column headers: 0-F
    for (int c = 0; c < 16; c++)
        m_ram_table->setHorizontalHeaderItem(c, new QTableWidgetItem(QString::number(c, 16).toUpper()));
    // Row headers: $80, $90, ...
    for (int r = 0; r < 8; r++)
        m_ram_table->setVerticalHeaderItem(r, new QTableWidgetItem(QString("$%1").arg(0x80 + r * 16, 2, 16, QChar('0')).toUpper()));

    m_ram_table->horizontalHeader()->setDefaultSectionSize(28);
    m_ram_table->verticalHeader()->setDefaultSectionSize(22);

    ramLayout->addWidget(m_ram_table);
    m_tabs->addTab(ramTab, tr("RAM"));

    layout->addWidget(m_tabs);

    // Refresh rate label
    auto* footer = new QLabel(tr("Auto-refreshes at 10 Hz while visible."), this);
    footer->setStyleSheet("color: #666; font-size: 10px;");
    layout->addWidget(footer);
}

void DebugWindow::setCore(IEmulatorCore* core, bool isLynx)
{
    m_core    = core;
    m_is_lynx = isLynx;
    m_tabs->setTabText(1, isLynx ? tr("Mikey") : tr("TIA"));
}

void DebugWindow::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    m_refresh_timer->start();
    refresh();
}

void DebugWindow::hideEvent(QHideEvent* event)
{
    QDialog::hideEvent(event);
    m_refresh_timer->stop();
}

void DebugWindow::refresh()
{
    if (!m_core || !m_core->isRunning())
        return;

    if (m_is_lynx)
        refreshLynx();
    else
        refreshAtari();
}

void DebugWindow::refreshAtari()
{
    auto* emu = qobject_cast<EmulatorCore*>(m_core);
    if (!emu) return;

    // Access CPU via friend or public accessor — we expose minimal info via EmulatorCore
    // For now show what we can from the public interface
    m_cpu_pc->setText("(running)");
    m_cpu_a->setText("--");
    m_cpu_x->setText("--");
    m_cpu_y->setText("--");
    m_cpu_sp->setText("--");
    m_cpu_p->setText("--");
    m_cpu_flags->setText("NV-BDIZC");

    m_chip_dump->setPlainText(
        "TIA debug output requires CPU step hooks.\n"
        "Enable in a future build.");
}

void DebugWindow::refreshLynx()
{
    auto* lynx = qobject_cast<LynxSystem*>(m_core);
    if (!lynx) return;

    // Get debug info from LynxSystem
    LynxSystem::DebugInfo info = lynx->getDebugInfo();

    if (info.isRunning && info.totalCycles > 0) {
        // CPU is running - display real register values
        m_cpu_pc->setText(QString("$%1").arg(info.pc, 4, 16, QChar('0')).toUpper());
        m_cpu_a->setText(QString("$%1").arg(info.a, 2, 16, QChar('0')).toUpper());
        m_cpu_x->setText(QString("$%1").arg(info.x, 2, 16, QChar('0')).toUpper());
        m_cpu_y->setText(QString("$%1").arg(info.y, 2, 16, QChar('0')).toUpper());
        m_cpu_sp->setText(QString("$%1").arg(info.sp, 2, 16, QChar('0')).toUpper());
        m_cpu_p->setText(QString("$%1").arg(info.p, 2, 16, QChar('0')).toUpper());

        // Format flags
        QString flags;
        flags += (info.p & 0x80) ? 'N' : '-';
        flags += (info.p & 0x40) ? 'V' : '-';
        flags += '-'; // unused
        flags += (info.p & 0x08) ? 'D' : '-';
        flags += (info.p & 0x10) ? 'B' : '-';
        flags += (info.p & 0x04) ? 'I' : '-';
        flags += (info.p & 0x02) ? 'Z' : '-';
        flags += (info.p & 0x01) ? 'C' : '-';
        m_cpu_flags->setText(flags);

        // Chip dump - show some runtime info
        QString chipInfo;
        chipInfo += QString("Lynx CPU: WDC 65C02 @ 4 MHz\n");
        chipInfo += QString("Total cycles: %1\n").arg(info.totalCycles);
        chipInfo += QString("PC: $%1\n\n").arg(info.pc, 4, 16, QChar('0')).toUpper();
        chipInfo += "Mikey/Suzy registers:\n";
        chipInfo += "  Mikey: $FD00-$FDFF (timers, audio, display, palette)\n";
        chipInfo += "  Suzy:  $FC00-$FCFF (sprite engine, math, input)\n\n";
        chipInfo += "Display: 160x102, 4bpp, ~75 Hz\n";
        chipInfo += "Audio: 4-channel LFSR, stereo (Lynx II)";

        m_chip_dump->setPlainText(chipInfo);
    } else {
        // Not running yet
        m_cpu_pc->setText("$----");
        m_cpu_a->setText("--");
        m_cpu_x->setText("--");
        m_cpu_y->setText("--");
        m_cpu_sp->setText("--");
        m_cpu_p->setText("--");
        m_cpu_flags->setText("NV-BDIZC");

        m_chip_dump->setPlainText(
            "Lynx system not running yet.\n"
            "Load and start a ROM to see debug info.\n\n"
            "CPU: WDC 65C02 @ 4 MHz\n"
            "RAM: 64 KB\n"
            "Display: 160x102, 4bpp, ~75 Hz\n"
            "Audio: 4-channel LFSR, stereo");
    }
}
