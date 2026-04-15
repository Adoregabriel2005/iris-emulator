#pragma once

#include <QDialog>
#include <QTimer>

class IEmulatorCore;
class EmulatorCore;
class LynxSystem;
class QLabel;
class QTextEdit;
class QTabWidget;
class QTableWidget;

class DebugWindow : public QDialog
{
    Q_OBJECT
public:
    explicit DebugWindow(QWidget* parent = nullptr);

    void setCore(IEmulatorCore* core, bool isLynx);
    void refresh();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void buildUi();
    void refreshAtari();
    void refreshLynx();

    QString formatHex8(uint8_t v)  const { return QString("%1").arg(v, 2, 16, QChar('0')).toUpper(); }
    QString formatHex16(uint16_t v) const { return QString("%1").arg(v, 4, 16, QChar('0')).toUpper(); }

    IEmulatorCore* m_core  = nullptr;
    bool           m_is_lynx = false;

    QTimer*      m_refresh_timer = nullptr;
    QTabWidget*  m_tabs          = nullptr;

    // CPU tab
    QLabel* m_cpu_pc   = nullptr;
    QLabel* m_cpu_a    = nullptr;
    QLabel* m_cpu_x    = nullptr;
    QLabel* m_cpu_y    = nullptr;
    QLabel* m_cpu_sp   = nullptr;
    QLabel* m_cpu_p    = nullptr;
    QLabel* m_cpu_flags = nullptr;

    // TIA/Mikey tab
    QTextEdit* m_chip_dump = nullptr;

    // RAM tab
    QTableWidget* m_ram_table = nullptr;
};
